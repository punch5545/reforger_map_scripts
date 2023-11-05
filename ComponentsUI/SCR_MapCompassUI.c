//------------------------------------------------------------------------------------------------
//! Compass in map UI
class SCR_MapCompassUI: SCR_MapRTWBaseUI
{		
	const string ICON_NAME = "compass";
	
	protected SCR_CompassComponent m_CompassComp;

	//------------------------------------------------------------------------------------------------
	override void SetWidgetNames()
	{
		WIDGET_NAME = "CompassFrame";
		RT_WIDGET_NAME = "CompRTW";
		WORLD_NAME = "CompassUIWorld";
	}
	
	//------------------------------------------------------------------------------------------------
	override void InitPositionVectors()
	{
		m_CompassComp = SCR_CompassComponent.Cast( m_RTEntity.FindComponent(SCR_CompassComponent) );
		
		m_vPrefabPos = "0 0 0";
		m_vCameraPos = m_CompassComp.GetMapCamPosition();
		m_vCameraAngle = "0 -90 0";
	}
	
	//------------------------------------------------------------------------------------------------
	//! Update for the compass entity within preview world
	protected void UpdateCompassEntity()
	{								
		if (m_CompassComp)
		{
			IEntity player = SCR_PlayerController.GetLocalControlledEntity();
			if (!player)
				return;
			
			vector anglesPlayer = player.GetAngles();
			vector anglesCompass = vector.Zero;
			anglesCompass[1] = anglesPlayer[1];
			m_RTEntity.SetAngles(anglesCompass);
			
			// activate, apply proc anims
			if (m_CompassComp.GetMode() != EGadgetMode.IN_HAND)
				m_CompassComp.SetMapMode();
			
			// tick the frame
			float tick = System.GetFrameTimeS();
			m_CompassComp.EOnFrame( m_RTEntity, tick );
		}
		
		// update		
		BaseWorld previewWorld = m_RTWorld.GetRef();
		previewWorld.UpdateEntities();
	}
		
	//------------------------------------------------------------------------------------------------
	override void SetVisible(bool visible)
	{		
		if (visible)
		{
			// No compass equipped
			if (!FindRelatedGadget())
			{
				super.SetVisible(false);
				return;
			}
			
			super.SetVisible(visible);
					
			// Prep compass 
			m_CompassComp.Init2DMapCompass();
			
			// Start anim
			ScriptCallQueue queue = GetGame().GetCallqueue();
			if (queue)
				queue.CallLater(UpdateCompassEntity, 0, true);
		}
		else 
		{
			// stop anim
			ScriptCallQueue queue = GetGame().GetCallqueue();
			if (queue)
				queue.Remove(UpdateCompassEntity);
			
			super.SetVisible(visible);
		}
	}
									
	//------------------------------------------------------------------------------------------------
	override void Init()
	{
		SCR_MapToolMenuUI toolMenu = SCR_MapToolMenuUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapToolMenuUI));
		if (toolMenu)
		{
			m_ToolMenuEntry = toolMenu.RegisterToolMenuEntry(SCR_MapToolMenuUI.s_sToolMenuIcons, ICON_NAME, 11);
			m_ToolMenuEntry.m_OnClick.Insert(ToggleVisible);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{		
		// Drag compass
		if (m_bIsVisible && m_bIsDragged)
		{
			// apply shake
			m_CompassComp.DragMapCompass();
			
			// save compass position for map reopen
			WorkspaceWidget workspace = g_Game.GetWorkspace();
			m_wFrame.GetScreenPos(m_fPosX, m_fPosY);
			m_fPosX = workspace.DPIUnscale(m_fPosX);
			m_fPosY = workspace.DPIUnscale(m_fPosY);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SCR_MapCompassUI()
	{
		m_eGadgetType = EGadgetType.COMPASS;
	}
};