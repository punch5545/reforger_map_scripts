//------------------------------------------------------------------------------------------------
//! 2d map radial menu UI
class SCR_MapRadialUI: SCR_MapUIBaseComponent
{
	[Attribute()]
	protected ref SCR_RadialMenuController m_RadialController;
	
	[Attribute()]
	protected ref SCR_RadialMenu m_RadialMenu;
	
	protected SCR_MapCursorModule m_CursorModule;
	protected SCR_MapRadialDisplay m_Display;
	
	protected bool m_bRefresh;
	protected bool m_bEntriesUpdate = false;		// entries updated instead of entry selected

	protected vector m_vMenuWorldPos;
	
	protected ref ScriptInvoker<> m_OnMenuInit = new ScriptInvoker();	// used to fill in entries after map open
	protected ref ScriptInvoker<SCR_SelectionMenuEntry, float[]> m_OnEntryPerformed = new ScriptInvoker();
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnMenuInitInvoker() { return m_OnMenuInit; }
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnEntryPerformedInvoker() { return m_OnEntryPerformed; }
	
	//------------------------------------------------------------------------------------------------
	SCR_RadialMenuController GetRadialController() 
	{
		return m_RadialController;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_MapRadialDisplay GetRadialDisplay()
	{
		return m_Display;
	}
	
	//------------------------------------------------------------------------------------------------
	static SCR_MapRadialUI GetInstance()
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return null;
		
		return SCR_MapRadialUI.Cast(mapEntity.GetMapUIComponent(SCR_MapRadialUI));
	}
		
	//------------------------------------------------------------------------------------------------
	vector GetMenuWorldPosition()
	{				
		m_vMenuWorldPos[1] = GetGame().GetWorld().GetSurfaceY(m_vMenuWorldPos[0], m_vMenuWorldPos[2]);
		
		return m_vMenuWorldPos;
	}
		
	//------------------------------------------------------------------------------------------------
	//! Listener callback
	void OnInputMenuOpen(float value, EActionTrigger reason)
	{
		if (m_RadialMenu && m_RadialMenu.IsOpened())
			return;
		
		m_RadialController.OnInputOpen();
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_RadialMenuController event
	protected void InputOpenMenu(SCR_RadialMenuController controller, bool hasControl)
	{		
		if (!hasControl)
		{
			m_RadialController.Control(m_MapEntity, m_RadialMenu);
			
			SCR_HUDManagerComponent hud = GetGame().GetHUDManager();
			m_Display = SCR_MapRadialDisplay.Cast(hud.FindInfoDisplay(SCR_MapRadialDisplay));
			m_RadialMenu.SetMenuDisplay(m_Display);
		}
		else if (m_RadialMenu.IsOpened())	// TODO proper close here
			return;

		if (m_RadialMenu.GetEntryCount() == 0 || !m_CursorModule.HandleContextualMenu())		// map side conditions to open
			m_RadialController.SetEnableControl(false);
		else 
			m_RadialController.SetEnableControl(true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_RadialMenuController event
	protected void OnControllerTakeControl(SCR_RadialMenuController controller)
	{
		m_RadialMenu = m_RadialController.GetRadialMenu();
		m_RadialMenu.GetOnPerform().Insert(OnEntryPerformed);
		m_RadialMenu.GetOnOpen().Insert(OpenMenu);
		m_RadialMenu.GetOnClose().Insert(CloseMenu);
		
		m_RadialMenu.ClearEntries();
		
		m_OnMenuInit.Invoke(); // fill menu entries
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_RadialMenuController event
	protected void OnControllerChanged(SCR_RadialMenuController controller)
	{
		m_RadialMenu = m_RadialController.GetRadialMenu();
		m_RadialMenu.GetOnPerform().Remove(OnEntryPerformed);
		m_RadialMenu.GetOnOpen().Remove(OpenMenu);
		m_RadialMenu.GetOnClose().Remove(CloseMenu);
	}
			
	//------------------------------------------------------------------------------------------------
	//! SCR_RadialMenu event
	//! \return false if cannot open due to not having any entries
	bool OpenMenu()
	{		
		float wX, wY, sX, sY;
		m_MapEntity.GetMapCursorWorldPosition(wX, wY);
		m_MapEntity.WorldToScreen(wX, wY, sX, sY);
		m_MapEntity.PanSmooth(sX, sY);
		
		m_vMenuWorldPos[0] = wX;
		m_vMenuWorldPos[2] = wY;
		
		m_CursorModule.ForceCenterCursor();

		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_RadialMenu event
	void CloseMenu()
	{
		m_CursorModule.HandleContextualMenu(true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_RadialMenu event
	void OnEntryPerformed(SCR_SelectionMenu menu, SCR_SelectionMenuEntry entry)
	{
		float wX, wY;
		float worldPos[2];
		m_MapEntity.GetMapCenterWorldPosition(wX, wY);
		worldPos[0] = wX;
		worldPos[1] = wY;
		
		m_OnEntryPerformed.Invoke(entry, worldPos);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Insert own entry into the menu
	void InsertCustomRadialEntry(SCR_SelectionMenuEntry entry, SCR_SelectionMenuCategoryEntry category = null)
	{
		if (category)
			category.AddEntry(entry);
		else
			m_RadialMenu.AddEntry(entry);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Insert own category into the menu
	void InsertCustomRadialCategory(SCR_SelectionMenuCategoryEntry entry, SCR_SelectionMenuCategoryEntry parent = null)
	{ 
		if (parent)
			parent.AddEntry(entry);
		else 
			m_RadialMenu.AddCategoryEntry(entry);
	}

	//------------------------------------------------------------------------------------------------
	//! Add simple entry
	SCR_SelectionMenuEntry AddRadialEntry(string name, SCR_SelectionMenuCategoryEntry category = null)
	{		
		SCR_SelectionMenuEntry entry = new SCR_SelectionMenuEntry();
		entry.SetName(name);
		
		if (category)
			category.AddEntry(entry);
		else
			m_RadialMenu.AddEntry(entry);
		
		return entry;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Add simple category
	SCR_SelectionMenuCategoryEntry AddRadialCategory(string name, SCR_SelectionMenuCategoryEntry parent = null)
	{
		SCR_SelectionMenuCategoryEntry entry = new SCR_SelectionMenuCategoryEntry();
		entry.SetName(name);
		
		if (parent)
			parent.AddEntry(entry);
		else
			m_RadialMenu.AddCategoryEntry(entry);
		
		return entry;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Remove entry
	void RemoveRadialEntry(SCR_SelectionMenuEntry entry)
	{
		m_RadialMenu.RemoveEntry(entry);
	}
										
	//------------------------------------------------------------------------------------------------
	// OVERRIDES
	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		if (m_RadialMenu)
			m_RadialMenu.Update(timeSlice);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		
		m_CursorModule = SCR_MapCursorModule.Cast(m_MapEntity.GetMapModule(SCR_MapCursorModule));
		
		m_RadialController.GetOnInputOpen().Insert(InputOpenMenu);
		m_RadialController.GetOnTakeControl().Insert(OnControllerTakeControl);
		m_RadialController.GetOnControllerChanged().Insert(OnControllerChanged);
		
		GetGame().GetInputManager().AddActionListener("MapContextualMenu", EActionTrigger.DOWN, OnInputMenuOpen);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{		
		GetGame().GetInputManager().RemoveActionListener("MapContextualMenu", EActionTrigger.DOWN, OnInputMenuOpen);
		
		m_RadialController.GetOnInputOpen().Remove(InputOpenMenu);
		m_RadialController.GetOnTakeControl().Remove(OnControllerTakeControl);
		m_RadialController.GetOnControllerChanged().Remove(OnControllerChanged);
		
		if (m_RadialController.HasControl())	
		{
			OnControllerChanged(null);
			m_RadialController.StopControl(true);
		}
				
		super.OnMapClose(config);
	}
		
	//------------------------------------------------------------------------------------------------
	void SCR_MapRadialUI()
	{
		m_bHookToRoot = true;
	}
};