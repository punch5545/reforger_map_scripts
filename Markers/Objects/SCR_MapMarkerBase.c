//------------------------------------------------------------------------------------------------
//! Map marker object base class 
class SCR_MapMarkerBase
{
	// synchronized 
	protected SCR_EMapMarkerType m_eType;	// config type
	protected int m_iMarkerID = -1;			// network ID, -1 means the marker is not set as synchronized
	protected int m_iConfigID = -1;			// config id used when marker of a single type has bigger amount of configuration options
	protected int m_MarkerOwnerID;			// owner playerID
	protected int m_iPosWorldX;
	protected int m_iPosWorldY;
	protected int m_iColorEntry;			// placed marker color entry id
	protected int m_iIconEntry;				// placed marker icon entry id
	protected string m_sCustomText;
	
	// server only
	protected bool m_bIsServerSideDisabled; // in hosted server scenario, opposing faction markers have to be properly managed but still disabled from showing up
	
	// rest
	protected SCR_MapMarkerEntryConfig m_ConfigEntry; 	// marker entry associated with this marker type
	protected SCR_MapEntity m_MapEntity;
	protected Widget m_wRoot;
	protected SCR_MapMarkerWidgetComponent m_MarkerWidgetComp;
	protected ref Color m_Color;
		
	//------------------------------------------------------------------------------------------------
	Widget GetRootWidget()
	{
		return m_wRoot;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_EMapMarkerType GetType()
	{
		return m_eType;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetType(SCR_EMapMarkerType type)
	{
		m_eType = type;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetMarkerID()
	{
		return m_iMarkerID;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetMarkerID(int id)
	{
		m_iMarkerID = id;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetMarkerConfigID()
	{
		return m_iConfigID;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetMarkerConfigID(int id)
	{
		m_iConfigID = id;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetMarkerOwnerID()
	{
		return m_MarkerOwnerID;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetMarkerOwnerID(int playerID)
	{
		m_MarkerOwnerID = playerID;
	}
		
	//------------------------------------------------------------------------------------------------
	void GetWorldPos(out int pos[2])
	{
		pos[0] = m_iPosWorldX;
		pos[1] = m_iPosWorldY;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetWorldPos(int posX, int posY)
	{
		m_iPosWorldX = posX;
		m_iPosWorldY = posY;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetColorEntry()
	{
		return m_iColorEntry;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetIconEntry()
	{
		return m_iIconEntry;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetColorEntry(int colorEntry)
	{
		m_iColorEntry = colorEntry;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIconEntry(int iconEntry)
	{
		m_iIconEntry = iconEntry;
	}
	
	//------------------------------------------------------------------------------------------------
	string GetCustomText()
	{
		return m_sCustomText;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetCustomText(string text)
	{
		m_sCustomText = text;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Disable marker UI display on server -> for dedicated servers(no UI) or hosted server enemy faction 
	void SetServerDisabled(bool state)
	{
		m_bIsServerSideDisabled = state;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Fetch marker definition from config & create widget
	void OnCreateMarker()
	{
		if (m_bIsServerSideDisabled)
			return;
		
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame)
			return;
		
		m_ConfigEntry = SCR_MapMarkerManagerComponent.GetInstance().GetMarkerConfig().GetMarkerEntryConfigByType(m_eType);
		if (!m_ConfigEntry)
			return;
		
		m_wRoot = GetGame().GetWorkspace().CreateWidgets(m_ConfigEntry.GetMarkerLayout(), mapFrame);
		if (!m_wRoot)
			return;

		m_MarkerWidgetComp = SCR_MapMarkerWidgetComponent.Cast(m_wRoot.FindHandler(SCR_MapMarkerWidgetComponent));
		m_MarkerWidgetComp.SetMarkerObject(this);
		m_ConfigEntry.InitClientSettings(this, m_MarkerWidgetComp);
	}
	
	//------------------------------------------------------------------------------------------------
	void OnDelete()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Called from SCR_MapMarkerManagerComponent
	void OnUpdate()
	{
		if (m_bIsServerSideDisabled)
			return;
		
		int screenX, screenY;

		m_MapEntity.WorldToScreen(m_iPosWorldX, m_iPosWorldY, screenX, screenY, true);
		FrameSlot.SetPos(m_wRoot, GetGame().GetWorkspace().DPIUnscale(screenX), GetGame().GetWorkspace().DPIUnscale(screenY));	// needs unscaled coords
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Extract(SCR_MapMarkerBase instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iPosWorldX);
		snapshot.SerializeInt(instance.m_iPosWorldY);
		snapshot.SerializeInt(instance.m_iMarkerID);
		snapshot.SerializeInt(instance.m_MarkerOwnerID);
		snapshot.SerializeInt(instance.m_iConfigID);
		snapshot.SerializeBytes(instance.m_eType, 1);
		snapshot.SerializeBytes(instance.m_iColorEntry, 1);
		snapshot.SerializeBytes(instance.m_iIconEntry, 1);
		snapshot.SerializeString(instance.m_sCustomText);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, SCR_MapMarkerBase instance)
	{
		snapshot.SerializeInt(instance.m_iPosWorldX);
		snapshot.SerializeInt(instance.m_iPosWorldY);
		snapshot.SerializeInt(instance.m_iMarkerID);
		snapshot.SerializeInt(instance.m_MarkerOwnerID);
		snapshot.SerializeInt(instance.m_iConfigID);
		snapshot.SerializeBytes(instance.m_eType, 1);
		snapshot.SerializeBytes(instance.m_iColorEntry, 1);
		snapshot.SerializeBytes(instance.m_iIconEntry, 1);
		snapshot.SerializeString(instance.m_sCustomText);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.Serialize(packet, 23);
		snapshot.EncodeString(packet);
	}

	//------------------------------------------------------------------------------------------------
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.Serialize(packet, 23);
		snapshot.DecodeString(packet);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs , ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 23)	// m_iPosWorldX(4) + m_iPosWorldY(4) + m_iMarkerID(4) + m_MarkerOwnerID(4)  + m_iConfigID(4) + m_eType(1) + m_iColorEntry(1) + m_iIconEntry(1)
			&& lhs.CompareStringSnapshots(rhs); // m_sCustomText
	}

	//------------------------------------------------------------------------------------------------
	static bool PropCompare(SCR_MapMarkerBase instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_iPosWorldX)
			&& snapshot.CompareInt(instance.m_iPosWorldY) 
			&& snapshot.CompareInt(instance.m_iMarkerID)
			&& snapshot.CompareInt(instance.m_MarkerOwnerID)
			&& snapshot.CompareInt(instance.m_iConfigID)
		    && snapshot.Compare(instance.m_eType, 1)
			&& snapshot.Compare(instance.m_iColorEntry, 1)
			&& snapshot.Compare(instance.m_iIconEntry, 1)
			&& snapshot.CompareString(instance.m_sCustomText);
	}
};
