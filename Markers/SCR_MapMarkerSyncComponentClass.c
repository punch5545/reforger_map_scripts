//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class SCR_MapMarkerSyncComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Used for Client->Server RPC ask methods for spawn and removal of static version of networked markers 
//! Attached to PlayerController
class SCR_MapMarkerSyncComponent : ScriptComponent
{
	protected static int m_iID;						// server side only, unique id created for markers
	protected int m_iMarkerLimit = 5;				// server side only, limit of allowed synchronized markers per client
	protected ref array<int> m_OwnedMarkers = {}; 	// server side only, list of markers owned by this controller to enforce limits
	
	//------------------------------------------------------------------------------------------------
	//! Ask to add a networked marker
	//! Called by marker manager
	void AskAddStaticMarker(notnull SCR_MapMarkerBase marker)
	{				
		Rpc(RPC_AskAddStaticMarker, marker);
	}
		
	//------------------------------------------------------------------------------------------------
	//! Ask to remove a networked marker
	//! Called by marker manager
	void AskRemoveStaticMarker(int markerID)
	{
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMgr)
			return;
		
		SCR_MapMarkerBase marker = markerMgr.GetStaticMarkerByID(markerID);
		if (!marker || marker.GetMarkerOwnerID() != SCR_PlayerController.Cast(GetOwner()).GetPlayerId())	// client side sanity check
			return;
		
		Rpc(RPC_AskRemoveStaticMarker, markerID);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Clear owned markers
	//! Server side only
	//! Called by marker manager
	void ClearOwnedMarkers()
	{
		for (int i; i < m_OwnedMarkers.Count(); i++)
		{
			if (m_OwnedMarkers.IsIndexValid(i))
			{
				AskRemoveStaticMarker(m_OwnedMarkers[i]);
				i--;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// RPC
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
   	protected void RPC_AskAddStaticMarker(SCR_MapMarkerBase markerData)
	{	
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMgr)
			return;
			
		if (m_iID == int.MAX)
			m_iID == 0;
		else
			m_iID++;
		
		if (m_OwnedMarkers.Count() >= m_iMarkerLimit)	// remove first if over limit
			Rpc(RPC_AskRemoveStaticMarker, m_OwnedMarkers[0]);
		
		markerData.SetMarkerID(m_iID);
		markerData.SetMarkerOwnerID(SCR_PlayerController.Cast(GetOwner()).GetPlayerId());

		m_OwnedMarkers.Insert(m_iID);
		markerMgr.OnAddSynchedMarker(markerData);	// add server side
		markerMgr.OnAskAddStaticMarker(markerData);
	}
			
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
   	protected void RPC_AskRemoveStaticMarker(int markerID)
	{	
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMgr)
			return;
		
		SCR_MapMarkerBase marker = markerMgr.GetStaticMarkerByID(markerID);
		if (!marker || marker.GetMarkerOwnerID() != SCR_PlayerController.Cast(GetOwner()).GetPlayerId())	//  server side sanity check
			return;

		m_OwnedMarkers.RemoveItemOrdered(markerID);
		markerMgr.OnRemoveSynchedMarker(markerID);	// remove server side
		markerMgr.OnAskRemoveStaticMarker(markerID);
	}	
}