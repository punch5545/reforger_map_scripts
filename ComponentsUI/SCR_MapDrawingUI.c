//------------------------------------------------------------------------------------------------
//! Map line
class MapLine
{
	const ref Color BUTTON_RED = Color.FromSRGBA(197, 75, 75, 255);
	
	bool m_bIsLineDrawn;		// if line is drawn when closing the map, redraw it on reopen
	bool m_bIsDrawMode;
	float m_fStartPointX, m_fStartPointY;
	float m_fEndPointX, m_fEndPointY;
	Widget m_wRootW;
	Widget m_wLine;
	ImageWidget m_wLineImage;
	Widget m_wDeleteButton;
	SCR_ButtonImageComponent m_DeleteButtonComp;
	SCR_MapEntity m_MapEntity;
	SCR_MapDrawingUI m_OwnerComponent;
	
	//------------------------------------------------------------------------------------------------
	void CreateLine(notnull Widget rootW, bool drawStart = false)
	{
		m_wRootW = rootW;
		
		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame)
			return;
		
		if (drawStart)
			m_MapEntity.GetMapCursorWorldPosition(m_fStartPointX, m_fStartPointY);
		else 
			m_bIsLineDrawn = true;
		
		m_wLine = GetGame().GetWorkspace().CreateWidgets("{E8850FCD9219C411}UI/layouts/Map/MapDrawLine.layout", mapFrame);
		m_wLineImage = ImageWidget.Cast(m_wLine.FindAnyWidget("DrawLineImage"));
			
		m_wDeleteButton = GetGame().GetWorkspace().CreateWidgets("{F486FAEEA00A5218}UI/layouts/Map/MapLineDeleteButton.layout", mapFrame);
		m_DeleteButtonComp = SCR_ButtonImageComponent.Cast(m_wDeleteButton.FindAnyWidget("DelButton").FindHandler(SCR_ButtonImageComponent));
		m_DeleteButtonComp.m_OnClicked.Insert(OnButtonClick);
		m_DeleteButtonComp.m_OnFocus.Insert(OnButtonFocus);
		m_DeleteButtonComp.m_OnFocusLost.Insert(OnButtonFocusLost);
		m_DeleteButtonComp.m_OnMouseEnter.Insert(OnMouseEnter);
		m_DeleteButtonComp.m_OnMouseLeave.Insert(OnMouseLeave);
		m_DeleteButtonComp.GetImageWidget().SetColor(BUTTON_RED);
		
		m_OwnerComponent.m_iLinesDrawn++;
		m_OwnerComponent.UpdateLineCount();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnButtonClick()
	{
		if (m_OwnerComponent.m_bIsLineBeingDrawn)
			return;
		
		DestroyLine(false);
		m_OwnerComponent.m_bActivationThrottle = true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnButtonFocus()
	{
		m_DeleteButtonComp.GetImageWidget().SetColor(UIColors.CONTRAST_COLOR);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnButtonFocusLost(Widget w)
	{
		m_DeleteButtonComp.GetImageWidget().SetColor(BUTTON_RED);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnMouseEnter(Widget w)
	{
		GetGame().GetWorkspace().SetFocusedWidget(w);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnMouseLeave(Widget w)
	{
		GetGame().GetWorkspace().SetFocusedWidget(null);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetButtonVisible(bool target)
	{
		if (target)
		{
			m_wDeleteButton.SetEnabled(true);
			m_wDeleteButton.SetVisible(true);
		}
		else 
		{
			m_wDeleteButton.SetEnabled(false);
			m_wDeleteButton.SetVisible(false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void DestroyLine(bool cacheDrawn = false)
	{
		if (m_wLine)
			m_wLine.RemoveFromHierarchy();
		
		if (m_wDeleteButton)
			m_wDeleteButton.RemoveFromHierarchy();
		
		if (!cacheDrawn)
			m_bIsLineDrawn = false;
		
		m_OwnerComponent.m_iLinesDrawn--;
		m_OwnerComponent.UpdateLineCount();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateLine(bool updateEndPos)
	{			
		if (!m_wLine)	// can happen due to callater used for update
			return;
				
		if (updateEndPos)
			m_MapEntity.GetMapCursorWorldPosition(m_fEndPointX, m_fEndPointY);	
		
		int screenX, screenY, endX, endY;

		m_MapEntity.WorldToScreen(m_fStartPointX, m_fStartPointY, screenX, screenY, true);
		m_MapEntity.WorldToScreen(m_fEndPointX, m_fEndPointY, endX, endY, true);
		
		vector lineVector = vector.Zero;
		lineVector[0] = m_fStartPointX - m_fEndPointX;
		lineVector[1] = m_fStartPointY - m_fEndPointY;

		vector angles = lineVector.VectorToAngles();
		if (angles[0] == 90)
			angles[1] =  180 - angles[1]; 	// reverse angles when passing vertical axis
		
		m_wLineImage.SetRotation(angles[1]);
		
		lineVector = m_MapEntity.GetMapWidget().SizeToPixels(lineVector);
		m_wLineImage.SetSize( GetGame().GetWorkspace().DPIUnscale(lineVector.Length()), 50);
		
		FrameSlot.SetPos(m_wLine, GetGame().GetWorkspace().DPIUnscale(screenX), GetGame().GetWorkspace().DPIUnscale(screenY));	// needs unscaled coords
		
		lineVector = vector.Zero;
		lineVector[0] =  GetGame().GetWorkspace().DPIUnscale((screenX + endX) / 2);
		lineVector[1] =  GetGame().GetWorkspace().DPIUnscale((screenY + endY) / 2);
		FrameSlot.SetPos(m_wDeleteButton, lineVector[0], lineVector[1]);	//del button

	}
	
	//------------------------------------------------------------------------------------------------
	void MapLine(SCR_MapEntity mapEnt, SCR_MapDrawingUI ownerComp)
	{
		m_MapEntity = mapEnt;
		m_OwnerComponent = ownerComp;
	}
};

//------------------------------------------------------------------------------------------------
//! Temporary drawing substitute so the protractor can be utilized properly
class SCR_MapDrawingUI: SCR_MapUIBaseComponent
{	
	[Attribute("editor", UIWidgets.EditBox, desc: "Toolmenu imageset quad name")]
	string m_sToolMenuIconName;
	
	[Attribute("9", UIWidgets.EditBox, desc: "Max line count")]
	int m_iLineCount;
	
	bool m_bActivationThrottle; 	// onclick will be called same frame draw mode activates/ delete button is clicked, this bool is used to ignore it
	bool m_bIsDrawModeActive;
	bool m_bIsLineBeingDrawn;
	int m_iLinesDrawn; 				// count of currently drawn lines
	protected int m_iLineID;		// active line id
	
	protected Widget m_wDrawingContainer;
	
	protected SCR_MapCursorModule 	m_CursorModule;
	protected SCR_MapToolEntry m_ToolMenuEntry;
	protected ref array<ref MapLine> m_aLines = new array <ref MapLine>();
	
	//------------------------------------------------------------------------------------------------
	//! Toggle draw mode
	protected void ToggleDrawMode()
	{
		if (!m_bIsDrawModeActive)
			SetDrawMode(true);
		else 
			SetDrawMode(false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Start/stop draw mode
	protected void SetDrawMode(bool state, bool cacheDrawn = false)
	{
		m_bIsDrawModeActive = state;
		
		if (state)
		{
			GetGame().GetInputManager().AddActionListener("MapSelect", EActionTrigger.UP, OnMapClick);
			m_bActivationThrottle = true;
			
			for (int i; i < m_iLineCount; i++)
			{
				if (m_aLines[i].m_bIsLineDrawn)
					m_aLines[i].SetButtonVisible(true);
			}
		}
		else 
		{
			GetGame().GetInputManager().RemoveActionListener("MapSelect", EActionTrigger.UP, OnMapClick);
			
			m_CursorModule.HandleDraw(false); // in case drawing was in progress
			
			for (int i; i < m_iLineCount; i++)	
			{	
				if (m_bIsLineBeingDrawn && i == m_iLineID)	// if drawing in progress, dont cache
				{
					m_aLines[i].DestroyLine(false);
					m_bIsLineBeingDrawn = false;
					m_iLineID = -1;
				}
				else if (cacheDrawn)						// map is closing, cache drawn lines and destroy
					m_aLines[i].DestroyLine(true);
				else if (m_aLines[i].m_bIsLineDrawn)
					m_aLines[i].SetButtonVisible(false);	// only draw mode disabled
			}
		}
		
		m_ToolMenuEntry.SetActive(state);
		m_ToolMenuEntry.m_ButtonComp.SetTextVisible(state);
		UpdateLineCount();
	}
	
	//------------------------------------------------------------------------------------------------		
	void UpdateLineCount()
	{
		m_ToolMenuEntry.m_ButtonComp.SetText(m_iLinesDrawn.ToString() + "/" + m_iLineCount.ToString());
	}
				
	//------------------------------------------------------------------------------------------------		
	protected void OnMapClick(float value, EActionTrigger reason)
	{				
		if (m_bActivationThrottle)
		{
			m_bActivationThrottle = false;
			return;
		}
		
		if (m_bIsLineBeingDrawn)	// state 2, line drawing in progress
		{
			m_bIsLineBeingDrawn = false;
			m_aLines[m_iLineID].m_bIsLineDrawn = true;
			m_iLineID = -1;
			
			m_CursorModule.HandleDraw(false);
			
			return;
		}
		
		if (m_iLinesDrawn >= m_iLineCount)
			return;
		
		if (!m_CursorModule.HandleDraw(true))	// draw restricted, return here
			return;
		
		for (int i; i < m_iLineCount; i++)	// state 1, start drawing line
		{
			if (!m_aLines[i].m_bIsLineDrawn)
			{
				m_aLines[i].CreateLine(m_wDrawingContainer, true);
				m_bIsLineBeingDrawn = true;
				m_iLineID = i;
				
				return;
			}
				
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_MapEntity event
	protected void OnMapPan(float x, float y, bool adjustedPan)
	{	
		for (int i; i < m_iLineCount; i++)
		{
			if (m_aLines[i].m_bIsLineDrawn) 
				m_aLines[i].UpdateLine(false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_MapEntity event
	protected void OnMapPanEnd(float x, float y)
	{
		for (int i; i < m_iLineCount; i++)
		{
			if (m_aLines[i].m_bIsLineDrawn) 
				GetGame().GetCallqueue().CallLater(m_aLines[i].UpdateLine, 0, false, false); // needs to be delayed by a frame as it cant always update the size after zoom correctly within the same frame
		}
	}
	
	//------------------------------------------------------------------------------------------------	
	//! SCR_MapToolEntry event
	protected void OnEntryToggled(SCR_MapToolEntry entry)
	{
		if (m_bIsDrawModeActive && entry != m_ToolMenuEntry)
			SetDrawMode(false);
	}
		
	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		
		m_wDrawingContainer = FrameWidget.Cast(config.RootWidgetRef.FindAnyWidget(SCR_MapConstants.DRAWING_CONTAINER_WIDGET_NAME));
		
		m_iLinesDrawn = 0;
		
		for (int i; i < m_iLineCount; i++)
		{
			if (m_aLines[i].m_bIsLineDrawn)
			{
				m_aLines[i].CreateLine(m_wDrawingContainer);
				GetGame().GetCallqueue().CallLater(m_aLines[i].UpdateLine, 0, false, false);
				m_aLines[i].SetButtonVisible(false);
			}
		}
						
		m_MapEntity.GetOnMapPan().Insert(OnMapPan);		// pan for scaling
		m_MapEntity.GetOnMapPanEnd().Insert(OnMapPanEnd);
		SCR_MapToolEntry.GetOnEntryToggledInvoker().Insert(OnEntryToggled);
		
		m_CursorModule = SCR_MapCursorModule.Cast(m_MapEntity.GetMapModule(SCR_MapCursorModule));
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{		
		m_MapEntity.GetOnMapPan().Remove(OnMapPan);
		m_MapEntity.GetOnMapPanEnd().Remove(OnMapPanEnd);
		SCR_MapToolEntry.GetOnEntryToggledInvoker().Remove(OnEntryToggled);
		
		SetDrawMode(false, true);
		m_bIsLineBeingDrawn = false;
		
		super.OnMapClose(config);
	}
	
	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{		
		if (m_bIsLineBeingDrawn)
			m_aLines[m_iLineID].UpdateLine(true);
	}
	
	//------------------------------------------------------------------------------------------------
	override void Init()
	{
		SCR_MapToolMenuUI toolMenu = SCR_MapToolMenuUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapToolMenuUI));
		if (toolMenu)
		{
			m_ToolMenuEntry = toolMenu.RegisterToolMenuEntry(SCR_MapToolMenuUI.s_sToolMenuIcons, m_sToolMenuIconName, 13); // add to menu
			m_ToolMenuEntry.m_OnClick.Insert(ToggleDrawMode);
			m_ToolMenuEntry.SetEnabled(true);
		}
		
		for (int i; i < m_iLineCount; i++)
		{
			MapLine line = new MapLine(m_MapEntity, this);
			m_aLines.Insert(line);
		}
	}
};