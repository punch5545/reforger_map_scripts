//------------------------------------------------------------------------------------------------
//! Squad leader marker entry 
[BaseContainerProps(), SCR_MapMarkerTitle()]
class SCR_MapMarkerEntrySquadLeader: SCR_MapMarkerEntryDynamic
{
	protected SCR_GroupsManagerComponent m_GroupsManager;
	
	protected ref map<SCR_AIGroup, SCR_MapMarkerSquadLeader> m_mGroupMarkers = new map<SCR_AIGroup, SCR_MapMarkerSquadLeader>();
	
	//------------------------------------------------------------------------------------------------
	//! Register marker here so it can be fetched from the map
	void RegisterMarker(SCR_MapMarkerSquadLeader marker, SCR_AIGroup group)
	{
		m_mGroupMarkers.Insert(group, marker);
	}
	
	//------------------------------------------------------------------------------------------------
	void UnregisterMarker(SCR_AIGroup group)
	{
		m_mGroupMarkers.Remove(group);
	}

	//------------------------------------------------------------------------------------------------
	//! SCR_GroupsManagerComponent event
	void OnPlayableGroupCreated(SCR_AIGroup group)
	{		
		SCR_MapMarkerSquadLeader marker = SCR_MapMarkerSquadLeader.Cast(m_MarkerMgr.InsertDynamicMarker(SCR_EMapMarkerType.SQUAD_LEADER, group));
		if (!marker)
			return;
		
		marker.SetFaction(group.GetFaction());
		RegisterMarker(marker, group);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_GroupsManagerComponent event
	void OnPlayableGroupRemoved(SCR_AIGroup group)
	{
		SCR_MapMarkerSquadLeader marker = m_mGroupMarkers.Get(group);
		if (marker)
			m_MarkerMgr.RemoveDynamicMarker(marker);
		
		UnregisterMarker(group);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_AIGroup event
	void OnPlayerLeaderChanged(int groupID, int playerId)
	{			
		SCR_AIGroup group = m_GroupsManager.FindGroup(groupID);
		SCR_MapMarkerSquadLeader marker = m_mGroupMarkers.Get(group);
		if (marker)
			marker.SetPlayerID(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_BaseGameMode event
	void OnPlayerSpawned(int playerId, IEntity player)
	{
		UpdateMarkerTarget(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_BaseGameMode event
	void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity)
	{		
		UpdateMarkerTarget(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_BaseGameMode event
	void OnPlayerDeleted(int playerId, IEntity player)
	{		
		UpdateMarkerTarget(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_AIGroup event
	void OnGroupCustomNameChanged(SCR_AIGroup group)
	{
		SCR_MapMarkerSquadLeader marker = m_mGroupMarkers.Get(group);
		if (marker)
			marker.m_bDoGroupTextUpdate = true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Update marker target, will trigger creation of a marker if within map
	protected void UpdateMarkerTarget(int playerID)
	{
		SCR_AIGroup group = m_GroupsManager.GetPlayerGroup(playerID);
		if (!group || !group.IsPlayerLeader(playerID))					// ignore if not leader
			return;
		
		SCR_MapMarkerSquadLeader marker = m_mGroupMarkers.Get(group);
		if (marker)
			marker.SetPlayerID(playerID);
	} 
	
	//------------------------------------------------------------------------------------------------
	override SCR_EMapMarkerType GetMarkerType()
	{
	 	return SCR_EMapMarkerType.SQUAD_LEADER;
	}
	
	//------------------------------------------------------------------------------------------------
	override void InitServerLogic()
	{	
		super.InitServerLogic();
		
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return;

		m_GroupsManager = SCR_GroupsManagerComponent.GetInstance();
		m_GroupsManager.GetOnPlayableGroupCreated().Insert(OnPlayableGroupCreated);
		m_GroupsManager.GetOnPlayableGroupRemoved().Insert(OnPlayableGroupRemoved);
		
		SCR_AIGroup.GetOnPlayerLeaderChanged().Insert(OnPlayerLeaderChanged);
		
		gameMode.GetOnPlayerSpawned().Insert(OnPlayerSpawned);
		gameMode.GetOnPlayerKilled().Insert(OnPlayerKilled);
		gameMode.GetOnPlayerDeleted().Insert(OnPlayerDeleted);
	}
	
	//------------------------------------------------------------------------------------------------
	override void InitClientSettingsDynamic(SCR_MapMarkerEntity marker, SCR_MapMarkerDynamicWComponent widgetComp)
	{
		super.InitClientSettingsDynamic(marker, widgetComp);
		
		SCR_AIGroup.GetOnCustomNameChanged().Remove(OnGroupCustomNameChanged);
		SCR_AIGroup.GetOnCustomNameChanged().Insert(OnGroupCustomNameChanged);
		
	}
}