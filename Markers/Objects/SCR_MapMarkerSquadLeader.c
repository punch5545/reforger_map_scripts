//------------------------------------------------------------------------------------------------
[EntityEditorProps(category: "GameScripted/Markers")]
class SCR_MapMarkerSquadLeaderClass : SCR_MapMarkerEntityClass
{}

//------------------------------------------------------------------------------------------------
//! Dynamic map marker -> squad leader
class SCR_MapMarkerSquadLeader : SCR_MapMarkerEntity
{
	[RplProp(onRplName: "OnPlayerIdUpdate")]
	protected int m_PlayerID;				// target ID, needed for visibility rules and fetching group
	
	const float SL_UPDATE_DELAY = 15; 		// seconds 
	const float ASPECT_RATIO_FLAG = 1.45;	// temp -> force aspect ratio of a military symbol
	
	bool m_bDoGroupTextUpdate;				// group text update flag
	protected bool m_bDoGroupSymbolUpdate;	// group symbol update flag
	protected SCR_AIGroup m_Group;
		
	//------------------------------------------------------------------------------------------------
	// RPL EVENTS
	//------------------------------------------------------------------------------------------------
	void OnPlayerIdUpdate()
	{
		PlayerController pController = GetGame().GetPlayerController();
		if (!pController)
			return;
		
		if (m_PlayerID == pController.GetPlayerId())	// if this is us, dont display
			SetLocalVisible(false);
		else
		{
			SetLocalVisible(true);
			m_bDoGroupTextUpdate = true;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// API SERVER
	//------------------------------------------------------------------------------------------------
	void SetPlayerID(int id)
	{
		m_PlayerID = id;
		m_MarkerFaction = SCR_FactionManager.SGetPlayerFaction(id);
		
		Replication.BumpMe();
		
		if (!System.IsConsoleApp())
			OnPlayerIdUpdate();
		
		UpdateTarget();
	}
	
	//------------------------------------------------------------------------------------------------
	// EVENTS & OTHERS
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetGroup()
	{
		return m_Group;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void AssignGroup()
	{
		SCR_GroupsManagerComponent comp = SCR_GroupsManagerComponent.GetInstance();
		if (!comp)
			return;
		
		m_Group = comp.GetPlayerGroup(m_PlayerID);
		
		if (m_Group)
			SCR_MapMarkerEntrySquadLeader.Cast(m_ConfigEntry).RegisterMarker(this, m_Group);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Target tracking based on playerID 
	//! Authority only
	protected void UpdateTarget()
	{
		IEntity ent = GetGame().GetPlayerManager().GetPlayerControlledEntity(m_PlayerID);
		if (ent)
		{
			SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
			if (!charController.IsDead())
			{
				SetTarget(ent);
				SetGlobalVisible(true);
								
				return;
			}
		}
		
		SetTarget(null);
		SetGlobalVisible(false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set military symbol image, can change during lifetime
	protected void InitGroupMilitarySymbol()
	{
		if (!m_Group)
		{
			AssignGroup();
			return;
		}
		
		string company, platoon, squad, character, format;
		m_Group.GetCallsigns(company, platoon, squad, character, format);

	 	SCR_Faction faction = SCR_Faction.Cast(m_Group.GetFaction());
		if (faction)
		{
			string flag = m_Group.GetGroupFlag();
			if (flag == string.Empty)
				flag = faction.GetFlagName(0);
			
			SetImage(faction.GetGroupFlagImageSet(), flag);
		}
		
		if (m_MarkerWidgetComp)		// if map is open, update immediately
			m_MarkerWidgetComp.SetImage(m_sImageset, m_sIconName, ASPECT_RATIO_FLAG);

		if (m_Group.IsPlayerInGroup(GetGame().GetPlayerController().GetPlayerId()))
			SCR_MapMarkerSquadLeaderComponent.Cast(m_MarkerWidgetComp).SetGroupActive(true);
		else
			SCR_MapMarkerSquadLeaderComponent.Cast(m_MarkerWidgetComp).SetGroupActive(false);
		
		m_bDoGroupSymbolUpdate = false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set group text, can change during lifetime
	protected void InitGroupText()
	{
		if (!m_Group)
		{
			AssignGroup();
			return;
		}
		
		string customName = m_Group.GetCustomName();
		if (customName != string.Empty)
			SetText(customName);
		else 
		{
			string company, platoon, squad, character, format;
			m_Group.GetCallsigns(company, platoon, squad, character, format);
				
			SetText(WidgetManager.Translate(format, company, platoon, squad, character));
		}
		
		if (m_MarkerWidgetComp)
			m_MarkerWidgetComp.SetText(m_sText);
		
		m_bDoGroupTextUpdate = false;
	}
	
	//------------------------------------------------------------------------------------------------
	// OVERRIDES
	//------------------------------------------------------------------------------------------------
	override void OnCreateMarker()
	{
		RplComponent rplComp = RplComponent.Cast(FindComponent(RplComponent));
		if (rplComp.IsOwner())	// authority only
		{
			IEntity ent = GetGame().GetPlayerManager().GetPlayerControlledEntity(m_PlayerID);
			if (ent)
				SetTarget(ent);
		}
		
		Faction markerFaction = SCR_FactionManager.SGetPlayerFaction(m_PlayerID);	
		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();	
		if (!localFaction || localFaction.IsFactionEnemy(markerFaction))	// markers could still get streamed in rare cases due to distance based streaming, in which case we check for faction and dont display
			return;	
			
		super.OnCreateMarker();
		
		m_bDoGroupSymbolUpdate = true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnUpdate()
	{
		if (!m_wRoot)
			return;
		
		super.OnUpdate();
		
		if (m_bDoGroupSymbolUpdate)
			InitGroupMilitarySymbol();
		
		if (m_bDoGroupTextUpdate)
			InitGroupText();
	}
	
	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		m_fUpdateDelay = SL_UPDATE_DELAY;
	}
	
	//------------------------------------------------------------------------------------------------
	void ~SCR_MapMarkerSquadLeader()
	{
		if (m_ConfigEntry)
			SCR_MapMarkerEntrySquadLeader.Cast(m_ConfigEntry).UnregisterMarker(m_Group);
	}
}