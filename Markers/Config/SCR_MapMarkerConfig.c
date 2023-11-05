//------------------------------------------------------------------------------------------------
//! Base marker types, set in SCR_MapMarkerConfig
//! They are used to differentiate markers in order to easily configure them and reuduce toll on the replication
//! If you have multiple of similar marker types, you should add a single enum here and diffenrentiate the subtypes based on secondaryID (see SCR_MarkerMilitaryEntry)
enum SCR_EMapMarkerType
{
	UNSET = 0,
	SIMPLE,				// simple static marker entry
	DYNAMIC_EXAMPLE,	// example entry of a dynamic marker
	PLACED_CUSTOM,		// configurable placed static marker
	PLACED_MILITARY,	// placed static marker - predefined military symbol
	SQUAD_LEADER		// dynamic squad leader marker 
}

//------------------------------------------------------------------------------------------------
//! Map marker config root
[BaseContainerProps(configRoot: true)]
class SCR_MapMarkerConfig
{		
	[Attribute("", UIWidgets.Object, "Definition of map marker types")]
	protected ref array<ref SCR_MapMarkerEntryConfig> m_aMarkerEntryConfigs;
		
	//------------------------------------------------------------------------------------------------
	//! Get all marker entry configs
	array<ref SCR_MapMarkerEntryConfig> GetMarkerEntryConfigs()
	{
		return m_aMarkerEntryConfigs;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get specific marker entry config
	SCR_MapMarkerEntryConfig GetMarkerEntryConfigByType(SCR_EMapMarkerType type)
	{
		if (!m_aMarkerEntryConfigs || m_aMarkerEntryConfigs.IsEmpty())
			return null;
		
		foreach (SCR_MapMarkerEntryConfig cfg : m_aMarkerEntryConfigs)
		{
			if (cfg.GetMarkerType() == type)
				return cfg;
		}
		
		return null;
	}
}

//------------------------------------------------------------------------------------------------
//! Base entry config
[BaseContainerProps(), SCR_MapMarkerTitle()]
class SCR_MapMarkerEntryConfig
{
	[Attribute("{DD15734EB89D74E2}UI/layouts/Map/MapMarkerBase.layout", UIWidgets.ResourceNamePicker, desc: "Marker layout", params: "layout")]
	protected ResourceName m_sMarkerLayout;
		
	//------------------------------------------------------------------------------------------------
	//! Override this in child classes with own config entries to define type
	SCR_EMapMarkerType GetMarkerType()
	{
	 	return SCR_EMapMarkerType.UNSET;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetMarkerLayout()
	{
	 	return m_sMarkerLayout;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Client side initialization of visuals or other client logic
	//! Override this in children where wanted
	void InitClientSettings(SCR_MapMarkerBase marker, SCR_MapMarkerWidgetComponent widgetComp)
	{
		int ownerID = marker.GetMarkerOwnerID();
		
		if (ownerID > 0)
			widgetComp.SetAuthor("(" + GetGame().GetPlayerManager().GetPlayerName(ownerID) + ")");
	}
	
	//------------------------------------------------------------------------------------------------
	//! Client side initialization of visuals or other client logic for dynamic markers
	//! Override this in children where wanted
	void InitClientSettingsDynamic(SCR_MapMarkerEntity marker, SCR_MapMarkerDynamicWComponent widgetComp)
	{
	}
	
	//------------------------------------------------------------------------------------------------
	//! Override this to set up server side logic & event behavior for dynamic markers
	void InitServerLogic()
	{
	}
}