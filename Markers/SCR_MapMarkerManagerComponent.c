[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "")]
class SCR_MapMarkerManagerComponentClass : SCR_BaseGameModeComponentClass
{
};

//------------------------------------------------------------------------------------------------
//! Map marker manager, keeps array of markers and takes care of synchronization
//! Attached to GameMode entity
class SCR_MapMarkerManagerComponent : SCR_BaseGameModeComponent
{	
	[Attribute("{720E8E61D7692172}Configs/Map/MapMarkerConfig.conf", UIWidgets.ResourceNamePicker )]
	protected ResourceName m_sMarkerCfgPath;
	
	protected static SCR_MapMarkerManagerComponent s_Instance;
	
	protected ref array<ref SCR_MapMarkerBase> m_aLocalMarkers = {};		// client side markers
	protected ref array<ref SCR_MapMarkerBase> m_aStaticMarkers = {};		// replicated static markers, one time RPC call for sync
	protected ref array<SCR_MapMarkerEntity> m_aDynamicMarkers = {};		// dynamically replicated markers
	
	protected SCR_MapMarkerSyncComponent m_MarkerSyncComp;
	protected ref SCR_MapMarkerConfig m_MarkerCfg; 
		
	//------------------------------------------------------------------------------------------------
	//! GETTERS
	//------------------------------------------------------------------------------------------------
	static SCR_MapMarkerManagerComponent GetInstance() 
	{ 
		return s_Instance; 
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_MapMarkerConfig GetMarkerConfig()
	{
		return m_MarkerCfg;
	}
	
	//------------------------------------------------------------------------------------------------
	array<ref SCR_MapMarkerBase> GetLocalMarkers()
	{
		return m_aLocalMarkers;
	}
	
	//------------------------------------------------------------------------------------------------
	array<ref SCR_MapMarkerBase> GetStaticMarkers()
	{
		return m_aStaticMarkers;
	}
	
	//------------------------------------------------------------------------------------------------
	array<SCR_MapMarkerEntity> GetDynamicMarkers()
	{
		return m_aDynamicMarkers;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_MapMarkerBase GetMarkerByWidget(Widget w)
	{
		foreach ( SCR_MapMarkerBase marker : m_aLocalMarkers )
		{
			if (w == marker.GetRootWidget())
				return marker;
		}
		
		foreach ( SCR_MapMarkerBase marker : m_aStaticMarkers )
		{
			if (w == marker.GetRootWidget())
				return marker;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_MapMarkerBase GetStaticMarkerByID(int markerID)
	{
		foreach ( SCR_MapMarkerBase marker : m_aStaticMarkers )
		{
			if (markerID == marker.GetMarkerID())
				return marker;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_MapMarkerEntity GetDynamicMarkerByTarget(SCR_EMapMarkerType type, IEntity target)
	{
		foreach (SCR_MapMarkerEntity marker : m_aDynamicMarkers)
		{
			if (type == marker.GetType() && target == marker.GetTarget())
				return marker;
		}
		
		return null;
	}
		
	//------------------------------------------------------------------------------------------------
	//! SPAWN/REMOVE API
	//------------------------------------------------------------------------------------------------
	//! \param type is primary marker type
	//! \param worldX is x world poosition
	//! \param worldY is y world poosition
	//! \param configId is secondary marker ID used to select a predefined subtype
	void InsertLocalMarkerByType(SCR_EMapMarkerType type, int worldX, int worldY, int configId = -1)
	{
		if (!m_MarkerCfg)
			return;
		
		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(type);
		marker.SetWorldPos(worldX, worldY);
		marker.SetMarkerConfigID(configId);
		
		InsertLocalMarker(marker);
	}
	
	//------------------------------------------------------------------------------------------------
	void InsertLocalMarker(SCR_MapMarkerBase marker)
	{
		marker.SetMarkerOwnerID(GetGame().GetPlayerController().GetPlayerId());
		
		m_aLocalMarkers.Insert(marker);
		marker.OnCreateMarker();
	}
	
	//------------------------------------------------------------------------------------------------
	//! \param type is primary marker type
	//! \param worldX is x world poosition
	//! \param worldY is y world poosition
	//! \param configId is secondary marker ID used to select a predefined subtype
	void InsertStaticMarkerByType(SCR_EMapMarkerType type, int worldX, int worldY, int configId = -1)
	{
		if (!m_MarkerCfg)
			return;
				
		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(type);
		marker.SetWorldPos(worldX, worldY);
		marker.SetMarkerConfigID(configId);
		
		InsertStaticMarker(marker);
	}
	
	//------------------------------------------------------------------------------------------------
	void InsertStaticMarker(SCR_MapMarkerBase marker)
	{
		if (!m_MarkerSyncComp)
		{
			if (!FindMarkerSyncComponent())
				return;
		}
		
		m_MarkerSyncComp.AskAddStaticMarker(marker);
	}
			
	//------------------------------------------------------------------------------------------------
	//! Authority only
	SCR_MapMarkerEntity InsertDynamicMarker(SCR_EMapMarkerType type, IEntity entity, int configId = -1)
	{
		if (!m_pGameMode.IsMaster())
			return null;
				
		SCR_MapMarkerEntryDynamic cfgEntry = SCR_MapMarkerEntryDynamic.Cast(m_MarkerCfg.GetMarkerEntryConfigByType(type));
		if (!cfgEntry)
			return null;
		
		Resource prefab = Resource.Load(cfgEntry.GetMarkerPrefab());
		
		SCR_MapMarkerEntity markerEnt = SCR_MapMarkerEntity.Cast(GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld()));
		if (!markerEnt)
			return null;
		
		markerEnt.SetType(type, configId);
		markerEnt.SetTarget(entity);
		
		return markerEnt;
	}
	
	//------------------------------------------------------------------------------------------------
	void RemoveLocalMarker(SCR_MapMarkerBase marker)
	{
		marker.OnDelete();
		m_aLocalMarkers.RemoveItem(marker);
	}
	
	//------------------------------------------------------------------------------------------------
	void RemoveStaticMarker(SCR_MapMarkerBase marker)
	{
		if (!m_MarkerSyncComp)
		{
			if (!FindMarkerSyncComponent())
				return;
		}
		
		m_MarkerSyncComp.AskRemoveStaticMarker(marker.GetMarkerID());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Authority only
	void RemoveDynamicMarker(notnull SCR_MapMarkerEntity ent)
	{
		if (!m_pGameMode.IsMaster())
			return;
		
		RplComponent.DeleteRplEntity(ent, false);
	}
	
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//! Enable stream out of specific marker for target identity
	//! \param marker is the subject marker
	//! \param PlayerController is the subject player's controller
	//! \param state sets target state of stream out -> enabled if true, meaning that replication will stream out the subject
	protected void HandleStreamOut(SCR_MapMarkerEntity marker, PlayerController pController, bool state)
	{
		RplComponent rplComponent = RplComponent.Cast(marker.FindComponent(RplComponent));
		if (!rplComponent || !pController)
			return;
				
		RplIdentity identity = pController.GetRplIdentity();
		if (!identity.IsValid())
			return;
		
		rplComponent.EnableStreamingConNode(identity, state);
		
		if (identity == RplIdentity.Local())	// if this is a hosted server, we cannot control visibilty through streaming, and so it has to be set manually
		{
			marker.SetLocalVisible(!state);
		}
	} 
	
	//------------------------------------------------------------------------------------------------
	//! Network streaming rules for the marker
	//! Authority only -> Called when marker faction is assigned
	void SetMarkerStreamRules(notnull SCR_MapMarkerEntity marker)
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		
		for (int i = 0; i < players.Count(); i++)
		{
			Faction markerFaction = marker.GetFaction();
			
			if (!markerFaction || markerFaction == SCR_FactionManager.SGetPlayerFaction(players[i]))
				HandleStreamOut(marker, GetGame().GetPlayerManager().GetPlayerController(players[i]), false);
			else
				HandleStreamOut(marker, GetGame().GetPlayerManager().GetPlayerController(players[i]), true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Network streaming rules of all markers of player
	//! Authority only ->  Set when player is spawned
	void SetStreamRulesForPlayer(int playerID)
	{			
		for (int i = 0; i < m_aDynamicMarkers.Count(); i++)
		{
			if (!m_aDynamicMarkers.IsIndexValid(i))
				continue;
			
			Faction markerFaction = m_aDynamicMarkers[i].GetFaction();
			
			if (!markerFaction || markerFaction == SCR_FactionManager.SGetPlayerFaction(playerID))
				HandleStreamOut(m_aDynamicMarkers[i], GetGame().GetPlayerManager().GetPlayerController(playerID), false);
			else
				HandleStreamOut(m_aDynamicMarkers[i], GetGame().GetPlayerManager().GetPlayerController(playerID), true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set manager to run UI updates for markers
	void EnableUpdate(bool state)
	{
		if (state)
			SetEventMask(GetOwner(), EntityEvent.POSTFRAME);
		else 
			ClearEventMask(GetOwner(), EntityEvent.POSTFRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool FindMarkerSyncComponent()
	{
		m_MarkerSyncComp = SCR_MapMarkerSyncComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_MapMarkerSyncComponent));
		if (m_MarkerSyncComp)
			return true;
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Registers the marker within manager, called by the marker entity
	void RegisterDynamicMarker(SCR_MapMarkerEntity markerEnt)
	{
		if (!m_aDynamicMarkers.Contains(markerEnt))
			m_aDynamicMarkers.Insert(markerEnt);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Unregisters the marker within manager, called by the marker entity
	void UnregisterDynamicMarker(SCR_MapMarkerEntity markerEnt)
	{
		if (m_aDynamicMarkers.Contains(markerEnt))
			m_aDynamicMarkers.RemoveItem(markerEnt);
	}
	
	//------------------------------------------------------------------------------------------------
	// EVENTS
	//------------------------------------------------------------------------------------------------
	//! RPC result of marker add broadcast
	void OnAddSynchedMarker(SCR_MapMarkerBase marker)
	{									
		m_aStaticMarkers.Insert(marker);
		
		if (System.IsConsoleApp())
			marker.SetServerDisabled(true);
		else if (marker.GetMarkerOwnerID() != GetGame().GetPlayerController().GetPlayerId())
		{
			Faction markerFaction = SCR_FactionManager.SGetPlayerFaction(marker.GetMarkerOwnerID());	
			Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
			
			if (!localFaction || localFaction.IsFactionEnemy(markerFaction))
			{
				if (Replication.IsServer())				// hosted server 
					marker.SetServerDisabled(true);
				else 
					m_aStaticMarkers.RemoveItem(marker);

				return;
			}
		}
		
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (mapEnt.IsOpen() && mapEnt.GetMapUIComponent(SCR_MapMarkersUI))		
			marker.OnCreateMarker();
	}
	
	//------------------------------------------------------------------------------------------------
	//! RPC result of marker remove broadcast
	void OnRemoveSynchedMarker(int markerID)
	{			
		SCR_MapMarkerBase marker = GetStaticMarkerByID(markerID);
		if (marker)
			marker.OnDelete();
		
		m_aStaticMarkers.RemoveItem(marker);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Server side call from sync component for RPC marker add broadcast
	void OnAskAddStaticMarker(SCR_MapMarkerBase markerData)
	{
		Rpc(RPC_DoAddStaticMarker, markerData);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Server side call from sync component for RPC marker remove broadcast
	void OnAskRemoveStaticMarker(int markerID)
	{
		Rpc(RPC_DoRemoveStaticMarker, markerID);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_DoAddStaticMarker(SCR_MapMarkerBase markerData)
    {			
		OnAddSynchedMarker(markerData);
    }
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_DoRemoveStaticMarker(int markerID)
    {		
		OnRemoveSynchedMarker(markerID);
    }
	
	//------------------------------------------------------------------------------------------------
	//! Faction manager event -> server only
	void OnPlayerFactionChanged_S(int playerID, SCR_PlayerFactionAffiliationComponent playerComponent, Faction faction)
	{
		SetStreamRulesForPlayer(playerID);
	}
	
	//------------------------------------------------------------------------------------------------
	// OVERRIDES
	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		if (!m_pGameMode.IsMaster())	// server only
			return;
		
		PlayerController playerController = GetGame().GetPlayerManager().GetPlayerController(playerId);			
		SCR_MapMarkerSyncComponent markerSyncComp = SCR_MapMarkerSyncComponent.Cast(playerController.FindComponent(SCR_MapMarkerSyncComponent));
		if (markerSyncComp)
			markerSyncComp.ClearOwnedMarkers();
	}
	
	//------------------------------------------------------------------------------------------------
	override event protected bool RplSave(ScriptBitWriter writer)
	{
		int count = m_aStaticMarkers.Count();
		writer.WriteInt(count);
		if (count == 0)
			return true;
		
		foreach (SCR_MapMarkerBase marker : m_aStaticMarkers)
		{
			int pos[2];
			marker.GetWorldPos(pos);
			
			writer.WriteInt(pos[0]);
			writer.WriteInt(pos[1]);
			writer.WriteInt(marker.GetMarkerID());
			writer.WriteInt(marker.GetMarkerOwnerID());
			writer.WriteInt(marker.GetMarkerConfigID());
			writer.WriteInt(marker.GetType());
			writer.Write(marker.GetColorEntry(), 8);
			writer.Write(marker.GetIconEntry(), 8);
			writer.WriteString(marker.GetCustomText());
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override event protected bool RplLoad(ScriptBitReader reader)
	{
		int count;
		reader.ReadInt(count);
		if (count == 0)
			return true;
		
		int posX, posY, markerID, markerOwnerID, markerConfigID, markerType, colorID, iconID;
		string customText;
		SCR_MapMarkerBase marker;
		Faction markerFaction;
		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		
		for (int i; i < count; i++)
		{	
				
			reader.ReadInt(posX);
			reader.ReadInt(posY);
			reader.ReadInt(markerID);
			reader.ReadInt(markerOwnerID);
			reader.ReadInt(markerConfigID);
			reader.ReadInt(markerType);
			reader.Read(colorID, 8);
			reader.Read(iconID, 8);
			reader.ReadString(customText);	
			
			marker = new SCR_MapMarkerBase();
			marker.SetType(markerType);
			marker.SetWorldPos(posX, posY);
			marker.SetMarkerID(markerID);
			marker.SetMarkerOwnerID(markerOwnerID);
			marker.SetMarkerConfigID(markerConfigID);
			marker.SetColorEntry(colorID);
			marker.SetIconEntry(iconID);
			marker.SetCustomText(customText);
						
			m_aStaticMarkers.Insert(marker);
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnPostFrame(IEntity owner, float timeSlice)
	{
		foreach (SCR_MapMarkerBase marker : m_aLocalMarkers)
		{
			marker.OnUpdate();
		}
		
		foreach (SCR_MapMarkerBase marker : m_aStaticMarkers)
		{
			marker.OnUpdate();
		}
		
		foreach (SCR_MapMarkerEntity markerEnt : m_aDynamicMarkers)
		{
			if (markerEnt)
				markerEnt.OnUpdate();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		if (!m_pGameMode.IsMaster())	// init server logic below
			return;
		
		array<ref SCR_MapMarkerEntryConfig> entryCfgs = m_MarkerCfg.GetMarkerEntryConfigs();
		foreach ( SCR_MapMarkerEntryConfig cfg : entryCfgs )
		{
			SCR_MapMarkerEntryDynamic entryDynamic = SCR_MapMarkerEntryDynamic.Cast(cfg);
			if (entryDynamic)
				entryDynamic.InitServerLogic();
		}
		
		SCR_FactionManager mgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (mgr)
			mgr.GetOnPlayerFactionChanged_S().Insert(OnPlayerFactionChanged_S);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		s_Instance = this;
		
		Resource container = BaseContainerTools.LoadContainer(m_sMarkerCfgPath);
		if (container)
			m_MarkerCfg = SCR_MapMarkerConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(container.GetResource().ToBaseContainer()));
		
		SetEventMask(owner, EntityEvent.INIT);
	} 
	
	//------------------------------------------------------------------------------------------------
	void ~SCR_MapMarkerManagerComponent()
	{
		s_Instance = null;
	}
	
}
