//------------------------------------------------------------------------------------------------
//! Cursor data
class SCR_MapCursorInfo
{
	static bool isGamepad;			// is using gamepad
	bool isFixedMode = true;		// determines whether cursor is fixed to the screen center 
	
	static int x, y; 				// by default, all screen position are DPI Unscaled, when scaled, these will return a range from 0 to window resolution in pixels
	static int startPos[2];			// state start pos pan
	static int startPosMultiSel[2];	// state start pos multi select
	int lastX, lastY;				// last pos of x & y
	EMapCursorEdgePos edgeFlags;	// cursor screen edge flags
	
	static WorkspaceWidget m_WorkspaceWidget;

	//------------------------------------------------------------------------------------------------
	//! DPI scale of values
	static int Scale(int pos)
	{
		if (!m_WorkspaceWidget)
			m_WorkspaceWidget = GetGame().GetWorkspace();
		
		return m_WorkspaceWidget.DPIScale(pos);
	}
	
	//------------------------------------------------------------------------------------------------
	void ~SCR_MapCursorInfo()
	{
		x = 0;
		y = 0;
		startPos = {0, 0};
		startPosMultiSel = {0, 0};
	}
};

//------------------------------------------------------------------------------------------------
//! Map cursor behavior and mode setup
[BaseContainerProps()]
class SCR_MapCursorModule: SCR_MapModuleBase
{	
	[Attribute(defvalue: "1.5", uiwidget: UIWidgets.EditBox, desc: "Panning: multiplier of keyboard panning speed", params: "0.1 10")]
	float m_fPanKBMMultiplier;
	
	[Attribute(defvalue: "1.5", uiwidget: UIWidgets.EditBox, desc: "Panning: multiplier of thubmstick panning speed", params: "0.1 10")]
	float m_fPanStickMultiplier;
	
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Enables panning by moving cursor to the screen edges")]
	bool m_bEnableCursorEdgePan;
	
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Center KBM cursor on map open")]
	bool m_bIsCursorCenteredOnOpen;
				
	[Attribute(defvalue: "0.1", uiwidget: UIWidgets.EditBox, desc: "Time it takes to perform zooming of a single step", params: "0.01 10")]
	float m_fZoomAnimTime;
	
	[Attribute(defvalue: "2", uiwidget: UIWidgets.EditBox, desc: "Each zoom step is increased/decreased by the currentZoom/thisVariable, lower number means faster steps", params: "0.1 10")]
	float m_fZoomStrength;
	
	[Attribute(defvalue: "100", uiwidget: UIWidgets.EditBox, desc: "This is multiplier of average frame time resulting in a rotation degree value", params: "100 1000")]
	int m_iRotateSpeedMouse;
	
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Enables custom map crosshair visuals")]
	bool m_bEnableMapCrosshairVisuals;
	
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Enables crosshair grid coordinate display")]
	bool m_bEnableCrosshairCoords;
	
	[Attribute("0 0 0 1", UIWidgets.ColorPicker, desc: "Cursor grid/guide lines color")]
	ref Color m_GuidelineColor;
	
	[Attribute("", UIWidgets.Object, "Cursor state configuration")]
	protected ref array<ref SCR_CursorVisualState> m_aCursorStatesConfig;
	
	protected ref map<EMapCursorState, SCR_CursorVisualState> m_aCursorStatesMap = new map<EMapCursorState, SCR_CursorVisualState>;	// used to quickly fetch a desired cursor state from config
	
	//static
	protected static ref array<Widget> s_aTracedWidgets = {};
	
	// const
	const int CURSOR_CAPTURE_OFFSET = 10;		// hardcoded(code) screen edges in pixels for cursor capture
	const int GUILDING_LINE_WIDTH = 16;
	const float PAN_DEFAULT_COUNTDOWN = 0.1;
	const float SINGLE_SELECTION_RANGE = 50.0;	// range in world pos
	const float CIRCLE_SELECTION_RANGE = 500.0;	// range in world pos
	const float FREE_CURSOR_RESET = 3.0;		// seconds, time before free cursor resets to locked mode on controller
	
	const EMapCursorState CUSTOM_CURSOR_LOCKED = EMapCursorState.CS_ROTATE;
	const EMapCursorState STATE_PAN_RESTRICTED	= EMapCursorState.CS_DRAG | EMapCursorState.CS_MODIFIER | EMapCursorState.CS_DRAW | EMapCursorState.CS_CONTEXTUAL_MENU;
	const EMapCursorState STATE_ZOOM_RESTRICTED = EMapCursorState.CS_DRAG | EMapCursorState.CS_MODIFIER | EMapCursorState.CS_DRAW | EMapCursorState.CS_CONTEXTUAL_MENU;
	const EMapCursorState STATE_HOVER_RESTRICTED = EMapCursorState.CS_PAN | EMapCursorState.CS_ZOOM | EMapCursorState.CS_MULTI_SELECTION 
												 | EMapCursorState.CS_DRAG | EMapCursorState.CS_DRAW | EMapCursorState.CS_CONTEXTUAL_MENU;
	const EMapCursorState STATE_SELECT_RESTRICTED = EMapCursorState.CS_MULTI_SELECTION | EMapCursorState.CS_CONTEXTUAL_MENU | EMapCursorState.CS_DRAG | EMapCursorState.CS_DRAW;
	const EMapCursorState STATE_MULTISELECT_RESTRICTED = EMapCursorState.CS_DRAG | EMapCursorState.CS_DRAW | EMapCursorState.CS_CONTEXTUAL_MENU | EMapCursorState.CS_MODIFIER;
	const EMapCursorState STATE_DRAG_RESTRICTED	= EMapCursorState.CS_CONTEXTUAL_MENU | EMapCursorState.CS_MULTI_SELECTION | EMapCursorState.CS_ROTATE | EMapCursorState.CS_DRAW;
	const EMapCursorState STATE_ROTATE_RESTRICTED = EMapCursorState.CS_PAN | EMapCursorState.CS_ZOOM | EMapCursorState.CS_CONTEXTUAL_MENU;
	const EMapCursorState STATE_DRAW_RESTRICTED = EMapCursorState.CS_PAN | EMapCursorState.CS_ZOOM | EMapCursorState.CS_CONTEXTUAL_MENU;
	const EMapCursorState STATE_CTXMENU_RESTRICTED = EMapCursorState.CS_DRAG | EMapCursorState.CS_DRAW;
	
	// timers
	protected float m_fPanCountdown;		// used to stop panning cursor state and refresh start position for next drag panning
	protected float m_fModifActionDelay;	// delay between modifier action activation 
	protected float m_fZoomHoldTime;		// zoom hold time for adjusting speed of zoom
	protected float m_fHoverTime;			// hover activation
	protected float m_fFreeCursorTime;		// free cursor deactivation
	protected float m_fSelectHoldTime;		// determines activation of multiselect, which is not using input filters so it doesnt create initial click pos delay
					
	// enums
	EMapCursorSelectType m_eMultiSelType;									// multiselect type
	protected EMapCursorState m_CursorState = EMapCursorState.CS_DEFAULT;	// keeps current cursor state
		
	protected bool m_bIsInit;				// is module initiated
	protected bool m_bIsDisabled;			// temporary module disable
	protected bool m_bIsDraggingAvailable;
	protected bool m_bIsSelectionAvailable;
	protected bool m_bIsModifierActive;
	protected int m_iRotationDirVal;
	protected float m_fZoomMultiplierWheel = 1;
	protected InputManager m_InputManager;
	protected ref SCR_MapCursorInfo m_CursorInfo;
	protected ref SCR_CursorCustom m_CustomCursor;
	protected CanvasWidget m_MapWidget;
	protected SCR_MapSelectionModule m_SelectionModule;
	
	// positioning & sizing
	protected ImageWidget m_wCrossLTop;	// top left fill image, width and heigth determined by the cursor pos
	protected ImageWidget m_wCrossRTop;	// top right fill image, width by (screen reso - cursor pos),  height determined by cursos pos
	protected ImageWidget m_wCrossLBot;	// bot left fill image, width set by top left fill, height by (screen reso - cursor pos)
	protected ImageWidget m_wGLLeft;	// guiding line sizing
	protected ImageWidget m_wGLTop;		// guiding line sizing
	protected Widget m_wCrossMCenter;	// used for centering
	
	// Rest of guide lines for coloring
	protected ImageWidget m_wGLRight;
	protected ImageWidget m_wGLBot;
	protected ImageWidget m_wGLFadeTop;
	protected ImageWidget m_wGLFadeLeft;
	protected ImageWidget m_wGLFadeRight;
	protected ImageWidget m_wGLFadeBot;
	protected TextWidget m_wCoordText;
	
	//------------------------------------------------------------------------------------------------
	// GETTERS / SETTERS
	//------------------------------------------------------------------------------------------------
	//! Get cursor data class
	SCR_MapCursorInfo GetCursorInfo() { return m_CursorInfo; }
	
	//------------------------------------------------------------------------------------------------
	//! Get cursor state
	EMapCursorState GetCursorState() { return m_CursorState; }
		
	//------------------------------------------------------------------------------------------------
	//! Get widgets under cursor
	static array<Widget> GetMapWidgetsUnderCursor()
	{	
		TraceMapWidgets();
		
		return s_aTracedWidgets;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set cursor state
	//! \param state is the added state
	protected void SetCursorState(EMapCursorState state) 
	{ 
		if (!m_bIsInit)		// some map components may call this when the module already disabled map cursor
			return;
		
		m_CursorState |= state; 
		SetCursorType(m_CursorState);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Unset cursor state
	//! \param state is the removed state
	protected void UnsetCursorState(EMapCursorState state) 
	{ 
		m_CursorState &= ~state; 
		SetCursorType(m_CursorState);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets cursor type - visual style
	protected void SetCursorType(EMapCursorState type)
	{
		if (!m_bEnableMapCrosshairVisuals)
			return;
		
		if (type == EMapCursorState.CS_DISABLE || m_bIsDisabled)
			m_CustomCursor.SetCursorVisual(null);
		else
			m_CustomCursor.SetCursorVisual(GetCursorStateCfg());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets active device - KB+M or controller
	protected void OnInputDeviceIsGamepad(bool isGamepad) 
	{ 
		m_CursorInfo.isGamepad = isGamepad;
		if (m_CursorInfo.isGamepad)
			ForceCenterCursor();
		else if (m_fFreeCursorTime != 0)	// if scheme switched during fade
			m_CustomCursor.SetOpacity(1);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets zoom strength multiplier for mouse wheel zoom
	void SetWheelZoomMultiplier(float value)
	{
		m_fZoomMultiplierWheel = value;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Force cursor to screen center
	//! Only use when necessarry as some methods rely on current cursor position
	void ForceCenterCursor()
	{
		float screenX, screenY;
		m_MapWidget.GetScreenSize(screenX, screenY);
		
		m_InputManager.SetCursorPosition(screenX/2, screenY/2);
		m_CursorInfo.x = screenX/2;
		m_CursorInfo.y = screenY/2;
		
		if (!m_CursorInfo.isFixedMode)
			m_fFreeCursorTime = FREE_CURSOR_RESET;
	}
		
	//------------------------------------------------------------------------------------------------
	// CURSOR STATE HANDLERS
	//------------------------------------------------------------------------------------------------
	//! Handle move state
	protected void HandleMove()
	{	
		//begin move
		if (m_CursorInfo.lastX != m_CursorInfo.x || m_CursorInfo.lastY != m_CursorInfo.y)
		{
			if ( ~m_CursorState & EMapCursorState.CS_MOVE )
			{
				SetCursorState(EMapCursorState.CS_MOVE);
			}
		}
		//end move
		else if (m_CursorState & EMapCursorState.CS_MOVE)
			UnsetCursorState(EMapCursorState.CS_MOVE);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle pan 
	protected void HandlePan(float timeSlice)
	{ 
		// pan disabled
		if (m_CursorState & STATE_PAN_RESTRICTED)
		{
			if (m_CursorState & EMapCursorState.CS_PAN)
				m_fPanCountdown = 0;
			else
				return;
		}
		
		// Start panning if cursor is on the screen edges
		if ( m_CursorInfo.edgeFlags > 0 && (m_CursorState & EMapCursorState.CS_PAN) == 0 )	// dont allow edge panning while panning using another method
		{
			if (m_fPanCountdown == 0)
				m_fPanCountdown = PAN_DEFAULT_COUNTDOWN;
			
			float frameTime = System.GetFrameTimeS();
			int px = (int)(frameTime * 1000) * m_fPanKBMMultiplier;
			
			if (m_CursorInfo.edgeFlags & EMapCursorEdgePos.LEFT)
				m_MapEntity.Pan(EMapPanMode.HORIZONTAL, px);
			else if (m_CursorInfo.edgeFlags & EMapCursorEdgePos.RIGHT)
				m_MapEntity.Pan(EMapPanMode.HORIZONTAL, -px);
			
			if (m_CursorInfo.edgeFlags & EMapCursorEdgePos.TOP)
				m_MapEntity.Pan(EMapPanMode.VERTICAL, px);
			else if (m_CursorInfo.edgeFlags & EMapCursorEdgePos.BOTTOM)
				m_MapEntity.Pan(EMapPanMode.VERTICAL, -px);
		}
		
		// stop pan state
		if (m_fPanCountdown <= 0)
		{						
			if (m_CursorState & EMapCursorState.CS_PAN)
			{
				UnsetCursorState(EMapCursorState.CS_PAN);
				m_CursorInfo.startPos = {0, 0};
			}
			
			return;
		}
		// begin pan state
		if ( ~m_CursorState & EMapCursorState.CS_PAN )
			SetCursorState(EMapCursorState.CS_PAN);
		
		m_fPanCountdown -= timeSlice;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle zoom 
	protected void HandleZoom()
	{		
		// stop zoom state
		if (!m_MapEntity.IsZooming())
		{
			if (m_CursorState & EMapCursorState.CS_ZOOM)
			{
				UnsetCursorState(EMapCursorState.CS_ZOOM);
			}
			
			return;
		}
		
		//! zoom disabled
		if (m_CursorState & STATE_ZOOM_RESTRICTED)
			return;
		
		// begin zoom state
		if ( ~m_CursorState & EMapCursorState.CS_ZOOM )
		{
			SetCursorState(EMapCursorState.CS_ZOOM);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle hover state displaying details over target
	protected void HandleHover(float timeSlice)
	{			
		// begin hover
		if (m_CursorState == EMapCursorState.CS_DEFAULT)
		{
			m_fHoverTime += timeSlice;
			
			if (m_fHoverTime >= 0.25)
			{
				float worldX, worldY;
				m_MapEntity.ScreenToWorldNoFlip(m_CursorInfo.Scale(m_CursorInfo.x), m_CursorInfo.Scale(m_CursorInfo.y), worldX, worldY);
				vector curPos = Vector(worldX, 0, worldY);
				
				MapItem closeItem = m_MapEntity.GetClose(curPos, SINGLE_SELECTION_RANGE / m_MapEntity.GetCurrentZoom());
				if (closeItem)
				{
					SetCursorState(EMapCursorState.CS_HOVER);
					m_MapEntity.HoverItem(closeItem);
				}

				return;
			}
		}
		// end hover
		else if (m_CursorState & EMapCursorState.CS_HOVER)
		{
			float worldX, worldY;
			m_MapEntity.ScreenToWorldNoFlip(m_CursorInfo.Scale(m_CursorInfo.x), m_CursorInfo.Scale(m_CursorInfo.y), worldX, worldY);
			vector curPos = Vector(worldX, 0, worldY);
			MapItem closeItem = m_MapEntity.GetClose(curPos, SINGLE_SELECTION_RANGE / m_MapEntity.GetCurrentZoom());
			
			if ( (m_CursorState & STATE_HOVER_RESTRICTED) 
				|| !closeItem
				|| (closeItem && closeItem != m_MapEntity.GetHoveredItem())
				)
			{
				m_MapEntity.ClearHover();
				m_fHoverTime = 0; 
				UnsetCursorState(EMapCursorState.CS_HOVER);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle select state
	protected void HandleSelect()
	{
		//! select disabled
		if (m_CursorState & STATE_SELECT_RESTRICTED)
			return;
		
		vector vScreenPos = Vector(m_CursorInfo.Scale(m_CursorInfo.x), 0, m_CursorInfo.Scale(m_CursorInfo.y));
		m_MapEntity.InvokeOnSelect(vScreenPos);
		
		// start select
		if ( ~m_CursorState & EMapCursorState.CS_SELECT )
		{
			SetCursorState(EMapCursorState.CS_SELECT);

			float worldX, worldY;
			m_MapEntity.ScreenToWorldNoFlip(m_CursorInfo.Scale(m_CursorInfo.x), m_CursorInfo.Scale(m_CursorInfo.y), worldX, worldY);
			vector curPos = Vector(worldX, 0, worldY);
			
			MapItem selected = m_MapEntity.GetClose(curPos, SINGLE_SELECTION_RANGE / m_MapEntity.GetCurrentZoom() );
			autoptr array<MapItem> selectedItems = new array<MapItem>;
			m_MapEntity.GetSelected(selectedItems);
			
			// only select if different item or multiple selected
			if (selected && (selectedItems.Count() != 1 || selectedItems[0] != selected) )
			{
				m_MapEntity.ClearSelection();
				m_MapEntity.SelectItem(selected);
			}
			else 
				m_MapEntity.ClearSelection();

		}
		
		// Selection state end instantly after a click
		UnsetCursorState(EMapCursorState.CS_SELECT);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle multi select - rectangular selection
	protected void HandleMultiSelect(bool activate)
	{		
		// multiselect state end or disabled
		if ( (m_CursorState & STATE_MULTISELECT_RESTRICTED) || !activate)
		{
			// never started
			if ( ~m_CursorState & EMapCursorState.CS_MULTI_SELECTION )
				return;
						
			// select items		
			m_MapEntity.ClearSelection();	
			autoptr array<MapItem> selectedItems = new array<MapItem>;
			GetCursorSelection(selectedItems);
			int count = selectedItems.Count();
			
			for ( int i = 0; i < count; i++ )
			{
				m_MapEntity.SelectItem(selectedItems[i]);
			}
			
			// multiselect end
			UnsetCursorState(EMapCursorState.CS_MULTI_SELECTION);
			m_CursorInfo.startPosMultiSel = {0, 0};
		}
		else if (activate)
		{			
			// multiselect begin
			if ( ~m_CursorState & EMapCursorState.CS_MULTI_SELECTION )
				SetCursorState(EMapCursorState.CS_MULTI_SELECTION);
			
			// highlight for items within selection
			m_MapEntity.ResetHighlighted();
			autoptr array<MapItem> selectedItems = new array<MapItem>;
			GetCursorSelection(selectedItems);
			int count = selectedItems.Count();
							
			for ( int i = 0; i < count; i++ )
			{
				selectedItems[i].SetHighlighted(true);
			}
		}
	}
		
	//------------------------------------------------------------------------------------------------
	//! Handle drag state
	//! \param startDrag determines whether dragging should begin or stop
	protected void HandleDrag(bool startDrag)
	{		
		// begin drag
		if (startDrag)
		{
			// disable drag state
			if (m_CursorState & STATE_DRAG_RESTRICTED)
				return;
			
			if ( ~m_CursorState & EMapCursorState.CS_DRAG )
			{
				if (SCR_MapToolInteractionUI.StartDrag())
				{
					SetCursorState(EMapCursorState.CS_DRAG);
				}
			}
		}
		// end drag
		else 
		{
			if (m_CursorState & EMapCursorState.CS_DRAG)
			{
				UnsetCursorState(EMapCursorState.CS_DRAG);
				SCR_MapToolInteractionUI.EndDrag();
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle rotate map tool
	protected void HandleRotateTool(bool startRotate)
	{		
		if (m_CursorState & STATE_ROTATE_RESTRICTED)
			return;
		
		if (startRotate)
		{
			if ( ~m_CursorState & EMapCursorState.CS_ROTATE )
			{
				if (SCR_MapToolInteractionUI.StartRotate())
					SetCursorState(EMapCursorState.CS_ROTATE);
			}
		}
		else 
		{
			if (m_CursorState & EMapCursorState.CS_ROTATE)
			{
				UnsetCursorState(EMapCursorState.CS_ROTATE);
				SCR_MapToolInteractionUI.EndRotate();
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle draw state
	bool HandleDraw(bool active)
	{
		// begin draw state
		if (active && (~m_CursorState & STATE_DRAW_RESTRICTED))
		{
			if ( ~m_CursorState & EMapCursorState.CS_DRAW )
			{
				SetCursorState(EMapCursorState.CS_DRAW);
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_START);
				return true;
			}
		}
		// end draw state
		else if (m_CursorState & EMapCursorState.CS_DRAW)
		{
			UnsetCursorState(EMapCursorState.CS_DRAW);
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_STOP);
		}
		
		return false;
	}
		
	//------------------------------------------------------------------------------------------------
	//! Handle contextual menu
	//! \param doClose determines whether the context menu should close
	//! \return state of the menu where false = close / true = open
	bool HandleContextualMenu(bool doClose = false)
	{
		//! context menu disabled
		if ((m_CursorState & STATE_CTXMENU_RESTRICTED) && !doClose)
			return false;
		
		SCR_MapRadialUI radialMenu = SCR_MapRadialUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapRadialUI));
		if (!radialMenu)
			return false;

		if (doClose)	// close 
		{
			if (m_CursorState & EMapCursorState.CS_CONTEXTUAL_MENU)
			{
				UnsetCursorState(EMapCursorState.CS_CONTEXTUAL_MENU);
				return false;
			}
		}
		else if (~m_CursorState & EMapCursorState.CS_CONTEXTUAL_MENU) // open
		{
			SetCursorState(EMapCursorState.CS_CONTEXTUAL_MENU);
			return true;
		}
		
		return false;
	}
		
	//------------------------------------------------------------------------------------------------
	// SUPPORT METHODS
	//------------------------------------------------------------------------------------------------
	//! Acquires array of map items for multi selection based on selection mode
	//! \param mapItems are map items within selection
	protected void GetCursorSelection(out array<MapItem> mapItems)
	{
			//! screen to world conversion
			float startWorldX, startWorldY, worldX, worldY;
			m_MapEntity.ScreenToWorldNoFlip(m_CursorInfo.Scale(m_CursorInfo.startPosMultiSel[0]), m_CursorInfo.Scale(m_CursorInfo.startPosMultiSel[1]), startWorldX, startWorldY);
			m_MapEntity.ScreenToWorldNoFlip(m_CursorInfo.Scale(m_CursorInfo.x), m_CursorInfo.Scale(m_CursorInfo.y), worldX, worldY);
			vector startPos = Vector(startWorldX, 0, startWorldY);
			vector curPos = Vector(worldX, 0, worldY);

			//! get entities inside the selection rectangle
			if (m_eMultiSelType == EMapCursorSelectType.RECTANGLE)
				m_MapEntity.GetInsideRect(mapItems, startPos, curPos);
			else if (m_eMultiSelType == EMapCursorSelectType.CIRCLE)
			{			
				float circleRadius = m_SelectionModule.GetSelCircleSize() / 2; // size is in pixels while we want radius
				m_MapEntity.GetInsideCircle(mapItems, curPos, circleRadius / m_MapEntity.GetCurrentZoom());
			}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Actual cursor position screen (DPI Unscaled)
	//! \param [out] unscaled cursor x
	//! \param [out] unscaled cursor y
	protected void GetCursorPosition(out int x, out int y)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		
		if (!m_MapWidget)
			m_MapWidget = m_MapEntity.GetMapWidget();
		
		float screenX, screenY, offX, offY;
		m_MapWidget.GetScreenSize(screenX, screenY);
		m_MapWidget.GetScreenPos(offX, offY);
		
		WidgetManager.GetMousePos(x, y);
		
		// gamepad cursor
		if (m_CursorInfo.isGamepad)
		{
			// free mode
			if (!m_CursorInfo.isFixedMode)
			{
				m_fFreeCursorTime += System.GetFrameTimeS();
				
				if (m_fFreeCursorTime > FREE_CURSOR_RESET - 1.0)
				{
					float alpha = Math.InverseLerp(FREE_CURSOR_RESET, FREE_CURSOR_RESET - 1.0, m_fFreeCursorTime);
					m_CustomCursor.SetOpacity(alpha);
				}
				
				if (m_fFreeCursorTime >= FREE_CURSOR_RESET)
				{
					m_CursorInfo.isFixedMode = true;
					m_fFreeCursorTime = 0;
					m_CustomCursor.SetOpacity(1);
					
					ForceCenterCursor();
					WidgetManager.GetMousePos(x, y);
				}
			}
		}
		
		// If the widget is not fullscreen, cursor position needs to be offset to match it
		x -= offX;
		y -= offY;
		
		// Screen edge pan
		EMapEntityMode mode = m_MapEntity.GetMapConfig().MapEntityMode;
		if (m_bEnableCursorEdgePan)
			TestEdgePan(x, y, screenX, screenY);
				
		// unscale the result
		x = workspace.DPIUnscale(x);
		y = workspace.DPIUnscale(y);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Window edge pan
	//! \param x is scaled cursor pos x
	//! \param y is scaled cursor pos y
	//! \param screenX is screen size x
	//! \param screenY is screen size y
	protected void TestEdgePan(inout float x, inout float y, int screenX, int screenY)
	{
		float windowWidth, windowHeight;
		m_MapWidget.GetScreenSize(windowWidth, windowHeight);

		if (x == 0)	// hack so the game doesnt autoscroll to top left when cursor is in default pos (f.e. during window unfocus)
			return;
		
		if (x <= CURSOR_CAPTURE_OFFSET)
			m_CursorInfo.edgeFlags |= EMapCursorEdgePos.LEFT; 
		else  
			m_CursorInfo.edgeFlags &= ~EMapCursorEdgePos.LEFT;
		
		if (y <= CURSOR_CAPTURE_OFFSET) 
			m_CursorInfo.edgeFlags |= EMapCursorEdgePos.TOP;
		else 
			m_CursorInfo.edgeFlags &= ~EMapCursorEdgePos.TOP;
		
		if (x >= (int)windowWidth - CURSOR_CAPTURE_OFFSET - 1) 
			m_CursorInfo.edgeFlags |= EMapCursorEdgePos.RIGHT;
		else  
			m_CursorInfo.edgeFlags &= ~EMapCursorEdgePos.RIGHT;
		
		if (y >= (int)windowHeight - CURSOR_CAPTURE_OFFSET - 1) 
			m_CursorInfo.edgeFlags |= EMapCursorEdgePos.BOTTOM;
		else  
			m_CursorInfo.edgeFlags &= ~EMapCursorEdgePos.BOTTOM;
	}
		
	//------------------------------------------------------------------------------------------------
	// INPUTS
	//------------------------------------------------------------------------------------------------
	//! Digital pan input for activating drag mode
	protected void OnInputPanDrag( float value, EActionTrigger reason )
	{
		// pan disabled
		if (m_CursorState & STATE_PAN_RESTRICTED)
			return;
		
		m_fPanCountdown = PAN_DEFAULT_COUNTDOWN;
		m_MapEntity.Pan(EMapPanMode.DRAG);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital pan KBM
	protected void OnInputPanH( float value, EActionTrigger reason )
	{				
		CalculatePan(value, EMapPanMode.HORIZONTAL, m_fPanKBMMultiplier);	// value determines direction for digital inputs
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital pan vertical
	protected void OnInputPanV( float value, EActionTrigger reason )
	{	
		CalculatePan(value, EMapPanMode.VERTICAL, m_fPanKBMMultiplier);		// value determines direction for digital inputs
	}
	
	//------------------------------------------------------------------------------------------------
	//! Analog pan horizontal
	protected void OnInputPanHGamepad( float value, EActionTrigger reason )
	{
		if (value == 0)
			return;
		
		CalculatePan(1, EMapPanMode.HORIZONTAL, value * m_fPanStickMultiplier);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Analog pan vertical
	protected void OnInputPanVGamepad( float value, EActionTrigger reason )
	{
		if (value == 0)
			return;

		CalculatePan(1, EMapPanMode.VERTICAL, value * m_fPanStickMultiplier);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Process pan 
	//! \param direction determines direction of digital inputs for the horizontal/vertical axis
	//! \param panMode is mode of panning
	//! \param multiPlier is configurable speed adjustment
	protected void CalculatePan(float direction, EMapPanMode panMode, float multiPlier)
	{
		if (m_CursorState & STATE_PAN_RESTRICTED)
			return;
		
		m_fPanCountdown = PAN_DEFAULT_COUNTDOWN;	
		int px = (System.GetFrameTimeS() * 1000);	// speed of pan based on time elapsed to avoid slowdown during lower fps
		
		if ((int)direction == 1)
			px = px * multiPlier;
		else if ((int)direction == 2)
			px = -px * multiPlier;
				
		m_MapEntity.Pan(panMode, px);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Cursor motion horizontal
	protected void OnInputGamepadCursorH( float value, EActionTrigger reason )
	{
		if (!m_CursorInfo.isGamepad)
			return;
				
		if (value != 0)
		{
			m_fFreeCursorTime = 0;
			m_CustomCursor.SetOpacity(1);
			m_CursorInfo.isFixedMode = false;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Cursor motion vertical
	protected void OnInputGamepadCursorV( float value, EActionTrigger reason )
	{
		if (!m_CursorInfo.isGamepad)
			return;
				
		if (value != 0)
		{
			m_fFreeCursorTime = 0;
			m_CustomCursor.SetOpacity(1);
			m_CursorInfo.isFixedMode = false;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital zoom
	protected void OnInputZoom( float value, EActionTrigger reason )
	{
		//! zoom disabled
		if (m_CursorState & STATE_ZOOM_RESTRICTED)
			return;
				
		m_fZoomHoldTime += System.GetFrameTimeS();
		if ( (m_CursorState & EMapCursorState.CS_ZOOM) && m_fZoomHoldTime < 0.05)	// if pressed, call once every 0.05 second
			return;
		
		float zoomPPU = m_MapEntity.GetCurrentZoom();
		if (value == 1)
				m_MapEntity.ZoomSmooth(zoomPPU + zoomPPU/m_fZoomStrength, m_fZoomAnimTime, false);
			else
				m_MapEntity.ZoomSmooth(zoomPPU - zoomPPU/m_fZoomStrength, m_fZoomAnimTime, false);
		
		m_fZoomHoldTime = 0;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital zoom mouse wheel
	protected void OnInputZoomWheelUp( float value, EActionTrigger reason )
	{		
		//! zoom disabled
		if (m_CursorState & STATE_ZOOM_RESTRICTED)
			return;
					
		float targetPPU = m_MapEntity.GetTargetZoomPPU();
		value = value * m_fZoomMultiplierWheel;
		m_MapEntity.ZoomSmooth(targetPPU + targetPPU * (value * 0.001), m_fZoomAnimTime, false);	// the const here is adjusting the value to match the input with zoom range
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital zoom mouse wheel
	protected void OnInputZoomWheelDown( float value, EActionTrigger reason )
	{		
		//! zoom disabled
		if (m_CursorState & STATE_ZOOM_RESTRICTED)
			return;
		
		float targetPPU = m_MapEntity.GetTargetZoomPPU();
		value = value * m_fZoomMultiplierWheel;
		m_MapEntity.ZoomSmooth(targetPPU - targetPPU/2 * (value * 0.001), m_fZoomAnimTime, false);	// the const here is adjusting the value to match the input with zoom range
	}
		
	//------------------------------------------------------------------------------------------------
	//! Digital drag
	protected void OnInputDrag( float value, EActionTrigger reason )
	{		
		if (m_CursorState & EMapCursorState.CS_MODIFIER)
		{
			if (reason == EActionTrigger.PRESSED)
			{
				if (m_CursorState & EMapCursorState.CS_DRAG)
					HandleDrag(false);
				
				HandleRotateTool(true);
			}
			else 
				HandleRotateTool(false);
				
			return;
		}
		
		if (reason == EActionTrigger.PRESSED)
		{
			if (m_bIsDraggingAvailable)
				HandleDrag(true); 
		}
		else 
		{
			if (m_bIsDraggingAvailable)
				HandleDrag(false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital drag toggle
	protected void OnInputDragGamepad( float value, EActionTrigger reason )
	{
		// End
		if (m_CursorState & EMapCursorState.CS_DRAG)
		{
			if (m_bIsDraggingAvailable)
				HandleDrag(false);
			
			return;
		}
		
		// Start
		if (m_bIsDraggingAvailable)
			HandleDrag(true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital modifier key
	protected void OnInputModifier( float value, EActionTrigger reason )
	{
		if (reason == EActionTrigger.DOWN)
			SetCursorState(EMapCursorState.CS_MODIFIER);
		else 
		{
			UnsetCursorState(EMapCursorState.CS_MODIFIER);
			if (SCR_MapToolInteractionUI.s_bIsRotating)
				HandleRotateTool(false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Analog modifier action UP
	protected void OnInputModifRotGamepadUp( float value, EActionTrigger reason )
	{
		if (~m_CursorState & EMapCursorState.CS_MODIFIER || !m_CursorInfo.isGamepad)
			return;
		
		OnInputModifRotGamepad(value, reason, true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Analog modifier action DOWN
	protected void OnInputModifRotGamepadDown( float value, EActionTrigger reason )
	{
		if (~m_CursorState & EMapCursorState.CS_MODIFIER || !m_CursorInfo.isGamepad)
			return;
		
		OnInputModifRotGamepad(value, reason, false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Called by OnInputModifierGamepad methods
	protected void OnInputModifRotGamepad(float value, EActionTrigger reason, bool direction)
	{				
		if (m_fModifActionDelay < 0.05)	// this const servers to disable rotation state, since classic actio ntrigger cant be used here due to it being a VALUE trigger
		{
			m_fModifActionDelay += System.GetFrameTimeS();
			
			if (m_fModifActionDelay < 0.025 || value == 0)	// this const is delay between rottion calls
				return;
		}
		else if (value == 0)
		{
			if (m_CursorState & EMapCursorState.CS_ROTATE)
				HandleRotateTool(false);
			
			return;
		}
					
		m_fModifActionDelay = 0;
		
		if (m_CursorState & STATE_ROTATE_RESTRICTED)
			return;
		
		if (!SCR_MapToolInteractionUI.s_bIsRotating)
		{
			HandleRotateTool(true);
			
			if (!SCR_MapToolInteractionUI.s_bIsRotating)	// didnt find rotatable widget under cursor
				return;
		}
		
		int rotateBy = Math.Round(value * 4);	// (0-1 * 4) gives variety of movement between 1 - 4 degrees
		if (rotateBy == 0)
			rotateBy == 1;
		
		SCR_MapToolInteractionUI.RotateWidget(direction,  Math.Round(value * 4));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Motion mouse input
	protected void OnInputModifRotate( float value, EActionTrigger reason )
	{	
		if (!SCR_MapToolInteractionUI.s_bIsRotating || value == m_iRotationDirVal)
			return;
		
		OnInputModifRot(value, reason, true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Motion mouse input
	protected void OnInputModifRotateBack( float value, EActionTrigger reason )
	{			
		if (!SCR_MapToolInteractionUI.s_bIsRotating || value == m_iRotationDirVal)
			return;
		
		OnInputModifRot(value, reason, false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Called by OnInputModifRotate methods
	protected void OnInputModifRot( float value, EActionTrigger reason, bool actionDir )
	{
		bool dir;
				
		if (actionDir)
		{
			if (value < m_iRotationDirVal)
				dir = true;
		}
		else 
		{
			if (value > m_iRotationDirVal)
				dir = true;
		}
		
		m_iRotationDirVal = value;
		
		if (m_CursorState & STATE_ROTATE_RESTRICTED)
			return;
		
		SCR_MapToolInteractionUI.RotateWidget(dir,  m_iRotateSpeedMouse * System.GetFrameTimeS());
		ForceCenterCursor();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital click
	protected void OnInputModifClick( float value, EActionTrigger reason )
	{
		SCR_MapToolInteractionUI.ActivateAction();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital multiselect mouse
	protected void OnInputMultiSel( float value, EActionTrigger reason )
	{
		if (reason == EActionTrigger.PRESSED)
		{
			if (m_fSelectHoldTime == 0)	// first call
				m_CursorInfo.startPosMultiSel = {m_CursorInfo.x, m_CursorInfo.y};
			
			m_fSelectHoldTime += System.GetFrameTimeS();
			if (m_fSelectHoldTime < 0.2)
				return;

			HandleMultiSelect(true);
			m_eMultiSelType = EMapCursorSelectType.RECTANGLE;

		}
		else 
		{
			HandleMultiSelect(false);
			m_fSelectHoldTime = 0;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital multiselect gamepad
	protected void OnInputMultiSelGamepad( float value, EActionTrigger reason )
	{
		if (reason == EActionTrigger.PRESSED)
		{
			m_CursorInfo.startPosMultiSel = {m_CursorInfo.x, m_CursorInfo.y};
			HandleMultiSelect(true);
			m_eMultiSelType = EMapCursorSelectType.CIRCLE;
		}
		else 
			HandleMultiSelect(false);
		
	}
		
	//------------------------------------------------------------------------------------------------
	//! PauseMenuUI event
	protected void OnPauseMenuOpened()
	{ 
		m_bIsDisabled = true;
		m_CursorState = EMapCursorState.CS_DEFAULT;
		SetCursorType(EMapCursorState.CS_DISABLE);
	}
	
	//------------------------------------------------------------------------------------------------
	//! PauseMenuUI event
	protected void OnPauseMenuClosed()
	{
		m_bIsDisabled = false;
		SetCursorType(m_CursorState);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get cursor visual state config
	//! \return highest configured cursor state config relative to current cursor state
	protected SCR_CursorVisualState GetCursorStateCfg()
	{		
		EMapCursorState state = EMapCursorState.CS_LAST;
		EMapCursorState stateMin = EMapCursorState.CS_DEFAULT;
		SCR_CursorVisualState cfg;
		while (state != stateMin)
		{
			if (m_CursorState & state)
			{
				cfg = m_aCursorStatesMap.Get(state);
				if (cfg)
					return cfg;
			}
			
			state = state >> 1;
		}
						
		return m_aCursorStatesMap.Get(EMapCursorState.CS_DEFAULT);
	}
		
	//------------------------------------------------------------------------------------------------
	//! Update crosshair UI
	protected void UpdateCrosshairUI()
	{		
		float sizeX, sizeY, cursorX, cursorY;
		m_wCrossMCenter.GetScreenSize(sizeX, sizeY);		
		
		cursorX = m_CursorInfo.x;
		cursorY = m_CursorInfo.y;
				
		// calculate and set new size of top & left fill imageWidgets
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		cursorX = cursorX - workspace.DPIUnscale(sizeX)/2;
		cursorY = cursorY - workspace.DPIUnscale(sizeY)/2;
		
		m_CustomCursor.Update(m_CursorInfo.x, m_CursorInfo.y);
		
		m_MapWidget.GetScreenSize(sizeX, sizeY);	
		
		m_wCrossLTop.SetSize(cursorX, cursorY);
		m_wCrossRTop.SetSize(workspace.DPIUnscale(sizeX) - cursorX, cursorY);
		m_wCrossLBot.SetSize(0, workspace.DPIUnscale(sizeY) - cursorY);
		m_wGLTop.SetSize(GUILDING_LINE_WIDTH, cursorY);
		m_wGLLeft.SetSize(cursorX, GUILDING_LINE_WIDTH);
		
		if (m_bEnableCrosshairCoords)
		{
			float wX, wY;
			m_MapEntity.ScreenToWorld(m_CursorInfo.Scale(m_CursorInfo.x), m_CursorInfo.Scale(m_CursorInfo.y), wX, wY);
			m_wCoordText.SetText(Math.Round(wX).ToString() + ", " + Math.Round(wY).ToString());
		}

	}
	
	//------------------------------------------------------------------------------------------------
	//! Trace widgets under cursor
	protected static void TraceMapWidgets()
	{
		array<ref Widget> widgets = {};
		WidgetManager.TraceWidgets(SCR_MapCursorInfo.Scale(SCR_MapCursorInfo.x), SCR_MapCursorInfo.Scale(SCR_MapCursorInfo.y), SCR_MapEntity.GetMapInstance().GetMapMenuRoot(), widgets);
		
		s_aTracedWidgets.Clear();
		foreach(Widget w: widgets)
		{
			s_aTracedWidgets.Insert(w);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize widgets
	protected void InitWidgets(Widget root)
	{
		m_MapWidget = m_MapEntity.GetMapWidget();
		
		Widget crossGrid = root.FindAnyWidget("CursorCrosshair");
		if (m_bEnableMapCrosshairVisuals)
		{
			crossGrid.SetVisible(true);
			// positioning widgets
			m_wCrossMCenter = root.FindAnyWidget("CrossMCenter");	
			m_wCrossRTop = ImageWidget.Cast(root.FindAnyWidget("CrossRTop"));	
			m_wCrossLTop = ImageWidget.Cast(root.FindAnyWidget("CrossLTop"));
			m_wCrossLBot = ImageWidget.Cast(root.FindAnyWidget("CrossLBot"));
			m_wGLTop = ImageWidget.Cast(root.FindAnyWidget("GLTop"));
			m_wGLLeft = ImageWidget.Cast(root.FindAnyWidget("GLLeft"));
			// guidelines	
			m_wGLFadeLeft = ImageWidget.Cast(root.FindAnyWidget("GLFadeLeft"));
			m_wGLRight = ImageWidget.Cast(root.FindAnyWidget("GLRight"));
			m_wGLFadeRight = ImageWidget.Cast(root.FindAnyWidget("GLFadeRight"));
			m_wGLBot = ImageWidget.Cast(root.FindAnyWidget("GLBot"));
			m_wGLFadeBot = ImageWidget.Cast(root.FindAnyWidget("GLFadeBot"));
			m_wGLFadeTop = ImageWidget.Cast(root.FindAnyWidget("GLFadeTop"));
			m_wCoordText = TextWidget.Cast(root.FindAnyWidget("CoordText"));
			
			// TODO configuration
			// Colors
			m_wGLTop.SetColor(m_GuidelineColor);
			m_wGLLeft.SetColor(m_GuidelineColor);
			m_wGLRight.SetColor(m_GuidelineColor);
			m_wGLBot.SetColor(m_GuidelineColor);
			m_wGLFadeTop.SetColor(m_GuidelineColor);
			m_wGLFadeLeft.SetColor(m_GuidelineColor);
			m_wGLFadeRight.SetColor(m_GuidelineColor);
			m_wGLFadeBot.SetColor(m_GuidelineColor);
			m_wCoordText.SetColor(m_GuidelineColor);
			
			m_wCoordText.SetOpacity(0); // text
			
			if (m_bEnableCrosshairCoords)
				m_wCoordText.SetOpacity(1);	
		}
		else 
			crossGrid.SetVisible(false);
		
		// cursor 
		if (!m_CustomCursor)
			m_CustomCursor = new SCR_CursorCustom();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize inputs
	protected void InitInputs()
	{
		m_InputManager = g_Game.GetInputManager();
		
		// controller detection 
		OnInputDeviceIsGamepad(!GetGame().GetInputManager().IsUsingMouseAndKeyboard());
		GetGame().OnInputDeviceIsGamepadInvoker().Insert(OnInputDeviceIsGamepad);
		
		// pause menu 
		PauseMenuUI.m_OnPauseMenuOpened.Insert(OnPauseMenuOpened);
		PauseMenuUI.m_OnPauseMenuClosed.Insert(OnPauseMenuClosed);
		
		m_InputManager.AddActionListener("MapPanDrag", EActionTrigger.PRESSED, OnInputPanDrag);
		m_InputManager.AddActionListener("MapPanH", EActionTrigger.PRESSED, OnInputPanH);
		m_InputManager.AddActionListener("MapPanV", EActionTrigger.PRESSED, OnInputPanV);
		m_InputManager.AddActionListener("MapPanHGamepad", EActionTrigger.VALUE, OnInputPanHGamepad);	// requires trigger by value since thumbstick val ranges from -1 to 1
		m_InputManager.AddActionListener("MapPanVGamepad", EActionTrigger.VALUE, OnInputPanVGamepad);
		m_InputManager.AddActionListener("MapGamepadCursorX", EActionTrigger.VALUE, OnInputGamepadCursorH);
		m_InputManager.AddActionListener("MapGamepadCursorY", EActionTrigger.VALUE, OnInputGamepadCursorV);
		m_InputManager.AddActionListener("MapZoom", EActionTrigger.PRESSED, OnInputZoom);
		m_InputManager.AddActionListener("MapWheelUp", EActionTrigger.PRESSED, OnInputZoomWheelUp);
		m_InputManager.AddActionListener("MapWheelDown", EActionTrigger.PRESSED, OnInputZoomWheelDown);
		m_InputManager.AddActionListener("MapSelect", EActionTrigger.UP, HandleSelect);
				
		// multi selection
		m_SelectionModule = SCR_MapSelectionModule.Cast(m_MapEntity.GetMapModule(SCR_MapSelectionModule));
		if ( m_SelectionModule )
		{
			m_InputManager.AddActionListener("MapMultiSelect", EActionTrigger.PRESSED, OnInputMultiSel);
			m_InputManager.AddActionListener("MapMultiSelect", EActionTrigger.UP, OnInputMultiSel);	
			m_InputManager.AddActionListener("MapMultiSelectGamepad", EActionTrigger.PRESSED, OnInputMultiSelGamepad);
			m_InputManager.AddActionListener("MapMultiSelectGamepad", EActionTrigger.UP, OnInputMultiSelGamepad);
		}
		
		// tool interaction UI
		if ( SCR_MapToolInteractionUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapToolInteractionUI)) )
		{
			m_bIsDraggingAvailable = true;
			
			m_InputManager.AddActionListener("MapModifierKey", EActionTrigger.DOWN, OnInputModifier);
			m_InputManager.AddActionListener("MapModifierKey", EActionTrigger.UP, OnInputModifier);
			m_InputManager.AddActionListener("MapModifRotGamepadUp", EActionTrigger.VALUE, OnInputModifRotGamepadUp);
			m_InputManager.AddActionListener("MapModifRotGamepadDown", EActionTrigger.VALUE, OnInputModifRotGamepadDown);
			m_InputManager.AddActionListener("MapModifRotate", EActionTrigger.PRESSED, OnInputModifRotate);
			m_InputManager.AddActionListener("MapModifRotateBack", EActionTrigger.PRESSED, OnInputModifRotateBack);
			m_InputManager.AddActionListener("MapModifClick", EActionTrigger.DOWN, OnInputModifClick);
			m_InputManager.AddActionListener("MapDrag", EActionTrigger.PRESSED, OnInputDrag);
			m_InputManager.AddActionListener("MapDrag", EActionTrigger.UP, OnInputDrag);
			m_InputManager.AddActionListener("MapDragGamepad", EActionTrigger.PRESSED, OnInputDragGamepad);
		}
		else 
			m_bIsDraggingAvailable = false;
	
	}
	
	//------------------------------------------------------------------------------------------------
	//! Cleanup inputs
	protected void CleanupInputs()
	{
		if (!m_InputManager)
			m_InputManager = g_Game.GetInputManager();
		
		// controller detection 
		GetGame().OnInputDeviceIsGamepadInvoker().Remove(OnInputDeviceIsGamepad);
		
		// pause menu
		PauseMenuUI.m_OnPauseMenuOpened.Remove(OnPauseMenuOpened);
		PauseMenuUI.m_OnPauseMenuClosed.Remove(OnPauseMenuClosed);
		
		m_InputManager.RemoveActionListener("MapPanDrag", EActionTrigger.PRESSED, OnInputPanDrag);
		m_InputManager.RemoveActionListener("MapPanH", EActionTrigger.PRESSED, OnInputPanH);
		m_InputManager.RemoveActionListener("MapPanV", EActionTrigger.PRESSED, OnInputPanV);
		m_InputManager.RemoveActionListener("MapPanHGamepad", EActionTrigger.VALUE, OnInputPanHGamepad);
		m_InputManager.RemoveActionListener("MapPanVGamepad", EActionTrigger.VALUE, OnInputPanVGamepad);
		m_InputManager.RemoveActionListener("MapGamepadCursorX", EActionTrigger.VALUE, OnInputGamepadCursorH);
		m_InputManager.RemoveActionListener("MapGamepadCursorY", EActionTrigger.VALUE, OnInputGamepadCursorV);
		m_InputManager.RemoveActionListener("MapZoom", EActionTrigger.PRESSED, OnInputZoom);
		m_InputManager.RemoveActionListener("MapWheelUp", EActionTrigger.PRESSED, OnInputZoomWheelUp);
		m_InputManager.RemoveActionListener("MapWheelDown", EActionTrigger.PRESSED, OnInputZoomWheelDown);
		m_InputManager.RemoveActionListener("MapSelect", EActionTrigger.UP, HandleSelect);
		
		m_InputManager.RemoveActionListener("MapModifierKey", EActionTrigger.DOWN, OnInputModifier);
		m_InputManager.RemoveActionListener("MapModifierKey", EActionTrigger.UP, OnInputModifier);
		m_InputManager.RemoveActionListener("MapModifRotGamepadUp", EActionTrigger.VALUE, OnInputModifRotGamepadUp);
		m_InputManager.RemoveActionListener("MapModifRotGamepadDown", EActionTrigger.VALUE, OnInputModifRotGamepadDown);
		m_InputManager.RemoveActionListener("MapModifRotate", EActionTrigger.PRESSED, OnInputModifRotate);
		m_InputManager.RemoveActionListener("MapModifRotateBack", EActionTrigger.PRESSED, OnInputModifRotateBack);
		m_InputManager.RemoveActionListener("MapModifClick", EActionTrigger.DOWN, OnInputModifClick);
		m_InputManager.RemoveActionListener("MapDrag", EActionTrigger.PRESSED, OnInputDrag);
		m_InputManager.RemoveActionListener("MapDrag", EActionTrigger.UP, OnInputDrag);
		m_InputManager.RemoveActionListener("MapDragGamepad", EActionTrigger.PRESSED, OnInputDragGamepad);
		
		m_InputManager.RemoveActionListener("MapMultiSelect", EActionTrigger.PRESSED, OnInputMultiSel);
		m_InputManager.RemoveActionListener("MapMultiSelect", EActionTrigger.UP, OnInputMultiSel);
		m_InputManager.RemoveActionListener("MapMultiSelectGamepad", EActionTrigger.PRESSED, OnInputMultiSelGamepad);
		m_InputManager.RemoveActionListener("MapMultiSelectGamepad", EActionTrigger.UP, OnInputMultiSelGamepad);
	}
	
	//------------------------------------------------------------------------------------------------
	// OVERRIDES
	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{		
		if (!m_CursorInfo)
			m_CursorInfo = new SCR_MapCursorInfo();
		
		m_bIsDisabled = false;
		InitWidgets(config.RootWidgetRef);
		InitInputs();
		m_bIsInit = true;
			
		SetCursorType(m_CursorState);
		
		if (m_bIsCursorCenteredOnOpen || m_CursorInfo.isGamepad)
			ForceCenterCursor();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{
		m_CursorState = EMapCursorState.CS_DEFAULT;
		SetCursorType(EMapCursorState.CS_DISABLE);
		CleanupInputs();
		
		m_bIsInit = false;
	}

	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		if (m_bIsDisabled)
			return;
		
		// update last pos
		m_CursorInfo.lastX = m_CursorInfo.x;
		m_CursorInfo.lastY = m_CursorInfo.y;
		
		// update current pos
		GetCursorPosition(m_CursorInfo.x, m_CursorInfo.y);

		// frame handlers
		HandleMove();
		HandleHover(timeSlice);
		HandlePan(timeSlice);
		HandleZoom();
		
		// crosshair grid lines
		if (m_bEnableMapCrosshairVisuals && (m_CursorState & CUSTOM_CURSOR_LOCKED) == 0)
			UpdateCrosshairUI();
		
		if (m_CursorState & EMapCursorState.CS_MODIFIER)
		{
			m_InputManager.ActivateContext("MapModifierContext");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SCR_MapCursorModule()
	{
		foreach (SCR_CursorVisualState cursorState : m_aCursorStatesConfig)
		{
			EMapCursorState stateEnum = cursorState.m_eCursorState;
			if (stateEnum)
				m_aCursorStatesMap.Insert(stateEnum, cursorState);
		}
	}
};

//------------------------------------------------------------------------------------------------
//! Cursor visual state config
[BaseContainerProps(), SCR_CursorStateTitle()]
class SCR_CursorVisualState
{
	[Attribute("1", UIWidgets.ComboBox, "Configure selected cursor state", "", ParamEnumArray.FromEnum(EMapCursorState))]
	int m_eCursorState;
		
	[Attribute("{E75FB4134580A496}UI/Textures/Cursor/cursors.imageset", UIWidgets.ResourceNamePicker, desc: "Imageset selection", params: "imageset")]
	ResourceName m_sCursorIconsImageset;
	
	[Attribute("default", UIWidgets.EditBox, desc: "imageset quad")]
	string m_sImageQuad;
	
	[Attribute("", UIWidgets.EditBox, desc: "imageset quad when controller is active instead of KBM \nIf this is not defined, attibute from above is used for both cases")]
	string m_sImageQuadController;
		
	[Attribute("0", UIWidgets.Slider, desc: "Padding from top, in pixels", "-32 32 1")]
	float m_fPaddingTop;
	
	[Attribute("0", UIWidgets.Slider, desc: "Padding from left, in pixels", "-32 32 1")]
	float m_fPaddingLeft;
	
	[Attribute("0.76 0.38 0.08 1", UIWidgets.ColorPicker, desc: "Cursor color")]
	ref Color m_Color;
};

//------------------------------------------------------------------------------------------------
//! Custom names for cursor visual states
class SCR_CursorStateTitle: BaseContainerCustomTitle
{
	//------------------------------------------------------------------------------------------------
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{		
		int type;
		source.Get("m_eCursorState", type);
		title = typename.EnumToString(EMapCursorState, type);
				
		return true;
	}
};