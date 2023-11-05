//------------------------------------------------------------------------------------------------
//! Attach this component to a widget in a map layout to configure interactions
class SCR_MapElementMoveComponent : ScriptedWidgetComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Allows the widget to be dragged")]
	bool m_bCanDrag;
	
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Allows to drag widget further off screen then default: half the size of the widget")]
	bool m_bCanDragOffScreen;
	
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Allows the widget to be rotated")]
	bool m_bCanRotate;
	
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Allows the widget to be activated")]
	bool m_bCanActivate;
};

//------------------------------------------------------------------------------------------------
//! Component for interacting with map tools
class SCR_MapToolInteractionUI : SCR_MapUIBaseComponent
{
	static bool s_bIsDragging = false;
	static bool s_bIsRotating = false;
	static bool s_bCanDragOffScreen = false;
	
	static protected ref ScriptInvoker<Widget> s_OnDragWidget = new ScriptInvoker();
	static protected ref ScriptInvoker<Widget> s_OnActivateTool = new ScriptInvoker();
	
	static protected Widget s_DraggedWidget;
	static protected Widget s_RotatedWidget;
	
	protected SCR_MapCursorModule m_CursorModule;
	protected SCR_MapCursorInfo m_CursorInfo;
	
	//------------------------------------------------------------------------------------------------
	// Invokers
	static ScriptInvoker GetOnDragWidgetInvoker() { return s_OnDragWidget; }
	static ScriptInvoker GetOnActivateToolInvoker() { return s_OnActivateTool; }
			
	//------------------------------------------------------------------------------------------------
	//! Tool action
	static void ActivateAction()
	{
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		SCR_MapElementMoveComponent moveComp;
		
		if (!CanBeManipulated(widgets))
			return;
		
		foreach ( Widget widget : widgets )
		{
			moveComp = SCR_MapElementMoveComponent.Cast(widget.FindHandler(SCR_MapElementMoveComponent));	
			if (!moveComp || !moveComp.m_bCanActivate)
				continue;
						
			s_OnActivateTool.Invoke(widget);
			return;
		}
	}
		
	//------------------------------------------------------------------------------------------------
	//! Begin drag
	//! \return Returns true if there is a draggable widget under the cursor
	static bool StartDrag()
	{
		if (s_bIsDragging)
			return false;
		
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		SCR_MapElementMoveComponent moveComp;
		
		if (!CanBeManipulated(widgets))
			return false;
		
		foreach ( Widget widget : widgets )
		{
			moveComp = SCR_MapElementMoveComponent.Cast(widget.FindHandler(SCR_MapElementMoveComponent));	
			if (moveComp)
			{
				if (!moveComp.m_bCanDrag)
					continue;
				
				s_DraggedWidget = widget;
				s_bCanDragOffScreen = moveComp.m_bCanDragOffScreen;
				break;
			}
		}
		
		if (s_DraggedWidget)
		{
			s_bIsDragging = true;
			s_OnDragWidget.Invoke(s_DraggedWidget);
			
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_GRAB);
			
			return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Begin rotation
	//! \return Returns true if there is a rotatable widget under the cursor
	static bool StartRotate()
	{
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		SCR_MapElementMoveComponent moveComp;
		
		if (!CanBeManipulated(widgets))
			return false;
		
		foreach ( Widget widget : widgets )
		{
			moveComp = SCR_MapElementMoveComponent.Cast(widget.FindHandler(SCR_MapElementMoveComponent));	
			if (moveComp)
			{
				if (!moveComp.m_bCanRotate)
					continue;
				
				s_RotatedWidget = widget;
				break;
			}
		}
		
		if (s_RotatedWidget)
		{
			s_bIsRotating = true;			
			return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check whether the tool isnt currently clicked from top of/under of a button 
	protected static bool CanBeManipulated(array<Widget> tracedWidgets)
	{
		foreach ( Widget widget : tracedWidgets )
		{
			if (ButtonWidget.Cast(widget))
				return false;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! End drag
	static void EndDrag()
	{
		s_bIsDragging = false;
		s_DraggedWidget = null;
		s_OnDragWidget.Invoke(null);
		
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_RELEASE);
	}
	
	//------------------------------------------------------------------------------------------------
	//! End rotation
	static void EndRotate()
	{
		s_bIsRotating = false;
		s_RotatedWidget = null;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Drag widget
	void DragWidget(Widget widget, SCR_MapCursorInfo cursorInfo)
	{				
		int screenX = GetGame().GetWorkspace().GetWidth();
		int screenY = GetGame().GetWorkspace().GetHeight();
		
		float widgetX, widgetY, fx, fy, minX, minY, maxX, maxY;
		
		// mouse position difference
		fx = cursorInfo.x - cursorInfo.lastX; 
		fy = cursorInfo.y - cursorInfo.lastY;

		// no change
		if (fx == 0 && fy == 0)
			return;

		// new pos
		vector pos = FrameSlot.GetPos(widget);
		fx = fx + pos[0];
		fy = fy + pos[1];

		//! get widget size
		widget.GetScreenSize(widgetX, widgetY);

		if (!s_bCanDragOffScreen)
		{
			//! get max screen size
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			minX = workspace.DPIUnscale(-widgetX/2);
			minY = workspace.DPIUnscale(-widgetY/2);
			maxX = workspace.DPIUnscale(screenX) - workspace.DPIUnscale(widgetX/2);
			maxY = workspace.DPIUnscale(screenY) - workspace.DPIUnscale(widgetY/2);
	
			// avoid moving the element off screen
			fx = Math.Clamp(fx, minX, maxX);
			fy = Math.Clamp(fy, minY, maxY);
		}
		
		// set new postion
		FrameSlot.SetPos(widget, fx, fy);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Rotate widget
	//! /param direction true is clockwise, false is counter-clockwise
	//! /param deg is ignored if 0, if not rotates the widget by the set amount of degrees
	static void RotateWidget(bool direction, int deg)
	{		
		ImageWidget img = ImageWidget.Cast(s_RotatedWidget.GetChildren());
		if (img)
		{
			if (deg == 0)
				deg = 1;

			if (direction)
				deg *= -1;
			
			img.SetRotation(img.GetRotation() + deg);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		
		m_CursorModule = SCR_MapCursorModule.Cast(m_MapEntity.GetMapModule(SCR_MapCursorModule));
		m_CursorInfo = m_CursorModule.GetCursorInfo();
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{
		s_OnDragWidget.Clear();
		s_OnActivateTool.Clear();
		s_DraggedWidget = null;
		s_RotatedWidget = null;
		s_bIsDragging = false;
		s_bIsRotating = false;
		
		super.OnMapClose(config);
	}
	
	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		if (s_bIsDragging)
		{
			DragWidget(s_DraggedWidget, m_CursorInfo);
		}
	}
};
