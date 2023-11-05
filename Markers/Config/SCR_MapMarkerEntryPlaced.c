//------------------------------------------------------------------------------------------------
//! Marker entry which can be placed through map
[BaseContainerProps(), SCR_MapMarkerTitle()]
class SCR_MapMarkerEntryPlaced : SCR_MapMarkerEntryConfig
{
	[Attribute("Custom marker", desc: "Description in selection menu")]
	protected string m_sMenuDescription;
	
	[Attribute("{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", UIWidgets.ResourcePickerThumbnail, desc: "Imageset resource", params: "imageset")]
	protected ResourceName m_sMenuImageset;
	
	[Attribute("settings", desc: "Imageset icon")]
	protected string m_sMenuIcon;
	
	[Attribute("", UIWidgets.Object, "Icons which can be chosen when placing a custom marker")]
	protected ref array<ref SCR_MarkerIconEntry> m_aPlacedMarkerIcons;
	
	[Attribute("", UIWidgets.Object, "Colors which can be chosen when placing a custom marker")]
	protected ref array<ref SCR_MarkerColorEntry> m_aPlacedMarkerColors;
		
	//------------------------------------------------------------------------------------------------
	string GetMenuDescription()
	{
	 	return m_sMenuDescription;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetMenuImageset()
	{
	 	return m_sMenuImageset;
	}
	
	//------------------------------------------------------------------------------------------------
	string GetMenuIcon()
	{
	 	return m_sMenuIcon;
	}
	
	//------------------------------------------------------------------------------------------------
	Color GetColorEntry(int i)
	{
		if (!m_aPlacedMarkerColors.IsIndexValid(i))
			return Color.White;
		
		return m_aPlacedMarkerColors[i].GetColor();
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetIconEntry(int i, out ResourceName imageset, out string imageQuad)
	{
		if (!m_aPlacedMarkerIcons.IsIndexValid(i))
			return false;
		
		m_aPlacedMarkerIcons[i].GetIconResource(imageset, imageQuad);
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	array<ref SCR_MarkerColorEntry> GetColorEntries()
	{
		return m_aPlacedMarkerColors;
	}
	
	//------------------------------------------------------------------------------------------------
	array<ref SCR_MarkerIconEntry> GetIconEntries()
	{
		return m_aPlacedMarkerIcons;
	}
	
	//------------------------------------------------------------------------------------------------
	override SCR_EMapMarkerType GetMarkerType()
	{
	 	return SCR_EMapMarkerType.PLACED_CUSTOM;
	}
	
	//------------------------------------------------------------------------------------------------
	override void InitClientSettings(SCR_MapMarkerBase marker, SCR_MapMarkerWidgetComponent widgetComp)
	{
		super.InitClientSettings(marker, widgetComp);
				
		ResourceName imageset;
		string quad;
		GetIconEntry(marker.GetIconEntry(), imageset, quad);
		widgetComp.SetImage(imageset, quad);
		widgetComp.SetText(marker.GetCustomText());
		widgetComp.SetColor(GetColorEntry(marker.GetColorEntry()));
		
		widgetComp.SetEventListening(true);
	}
}

//------------------------------------------------------------------------------------------------
//! Placed marker color entry
[BaseContainerProps()]
class SCR_MarkerColorEntry
{
	[Attribute("1.0 1.0 1.0 1.0")]
	protected ref Color m_Color;
	
	//------------------------------------------------------------------------------------------------
	Color GetColor()
	{
		return new Color(m_Color.R(), m_Color.G(), m_Color.B(), m_Color.A());
	}
	
}

//------------------------------------------------------------------------------------------------
//! Placed marker icon entry
[BaseContainerProps()]
class SCR_MarkerIconEntry
{
	[Attribute("{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", UIWidgets.ResourcePickerThumbnail, desc: "Image resource", params: "imageset")]
	protected ResourceName m_sIconImageset;
	
	[Attribute("", desc: "Imageset quad")]
	protected string m_sIconImagesetQuad;
	
	//------------------------------------------------------------------------------------------------
	void GetIconResource(out ResourceName imageset, out string imageQuad)
	{
		imageset = m_sIconImageset;
		imageQuad = m_sIconImagesetQuad;
	}
}

//------------------------------------------------------------------------------------------------
class SCR_MapMarkerTitle: BaseContainerCustomTitle
{
	//------------------------------------------------------------------------------------------------
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{				
		typename tName = source.GetClassName().ToType();
		SCR_MapMarkerEntryConfig cfg = SCR_MapMarkerEntryConfig.Cast(tName.Spawn());
		if (cfg)
			title = source.GetClassName() + ": " + typename.EnumToString(SCR_EMapMarkerType, cfg.GetMarkerType());
				
		return true;
	}
}
