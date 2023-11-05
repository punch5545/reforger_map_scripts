//------------------------------------------------------------------------------------------------
//! Marker military symbol entry 
[BaseContainerProps(), SCR_MapMarkerTitle()]
class SCR_MapMarkerEntryMilitary : SCR_MapMarkerEntryConfig
{
	[Attribute("", UIWidgets.Object, "Predefined marker factions")]
	protected ref array<ref SCR_MarkerMilitaryFactionEntry> m_aMilitaryFactionEntries;
	
	[Attribute("", UIWidgets.Object, "Predefined marker military entries")]
	protected ref array<ref SCR_MarkerMilitaryEntry> m_aMilitaryEntries;
	
	protected const float FACTION_DETERMINATOR = 0.001;
	
	//------------------------------------------------------------------------------------------------
	array<ref SCR_MarkerMilitaryFactionEntry> GetMilitaryFactionEntries()
	{
		return m_aMilitaryFactionEntries;
	}
	
	//------------------------------------------------------------------------------------------------
	array<ref SCR_MarkerMilitaryEntry> GetMilitaryEntries()
	{
		return m_aMilitaryEntries;
	}
	
	//------------------------------------------------------------------------------------------------
	override SCR_EMapMarkerType GetMarkerType()
	{
	 	return SCR_EMapMarkerType.PLACED_MILITARY;
	}
	
	//------------------------------------------------------------------------------------------------
	override void InitClientSettings(SCR_MapMarkerBase marker, SCR_MapMarkerWidgetComponent widgetComp)
	{
		super.InitClientSettings(marker, widgetComp);
		
		widgetComp.SetEventListening(true);
		
		array<ref SCR_MarkerMilitaryFactionEntry> milFactionEntries = GetMilitaryFactionEntries();
		array<ref SCR_MarkerMilitaryEntry> milEntries = GetMilitaryEntries();
		
		int id = marker.GetMarkerConfigID() * FACTION_DETERMINATOR;	// faction determinator -> find faction id
		if (!milFactionEntries.IsIndexValid(id))
			return;
		
		SCR_MarkerMilitaryFactionEntry factionEntry = milFactionEntries[id];	
		foreach (SCR_MarkerMilitaryEntry milEntry : milEntries)
		{
			if (milEntry.GetEntryID() != marker.GetMarkerConfigID() - (1000*id)) // subtract faction id to get entry id
				continue;
			
			widgetComp.SetMilitarySymbolMode(true);
			
			SCR_MilitarySymbol milSymbol = new SCR_MilitarySymbol();
			milSymbol.SetIdentity(factionEntry.GetFactionIdentity());
			milSymbol.SetDimension(milEntry.GetDimension());
			milSymbol.SetIcons(milEntry.GetIcons());
			milSymbol.SetAmplifier(milEntry.GetAmplifier());
							
			widgetComp.UpdateMilitarySymbol(milSymbol);
			widgetComp.SetText(milEntry.GetText());
			widgetComp.SetColor(factionEntry.GetColor());

			break;	
		}
	}
}

//------------------------------------------------------------------------------------------------
//! Marker predefined military entry 
[BaseContainerProps(), SCR_MapMarkerMilitaryTitle()]
class SCR_MarkerMilitaryEntry
{
	[Attribute("0", desc: "Unique secondary ID (primary is EMarkerType) used to reduce toll on the network during synchronization by loading const data from config")]
	protected int m_iEntryID;
	
	[Attribute("1", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EMilitarySymbolDimension))]
	protected EMilitarySymbolDimension m_Dimension;
	
	[Attribute("1", uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(EMilitarySymbolIcon))]
	protected EMilitarySymbolIcon m_Icons;
	
	[Attribute("0", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EMilitarySymbolAmplifier))]
	protected EMilitarySymbolAmplifier m_Amplifier;
	
	[Attribute("", desc: "Description in selection menu")]
	protected string m_sDescription;
	
	[Attribute("", desc: "Default text under the marker icon")]
	protected string m_sMarkerText;
	
	//------------------------------------------------------------------------------------------------
	int GetEntryID()
	{
		return m_iEntryID;
	}
	
	//------------------------------------------------------------------------------------------------
	EMilitarySymbolDimension GetDimension()
	{
		return m_Dimension;
	}
	
	//------------------------------------------------------------------------------------------------
	EMilitarySymbolIcon GetIcons()
	{
		return m_Icons;
	}
	
	//------------------------------------------------------------------------------------------------
	EMilitarySymbolAmplifier GetAmplifier()
	{
		return m_Amplifier;
	}
	
	//------------------------------------------------------------------------------------------------
	string GetDescription()
	{
		return m_sDescription;
	}
	
	//------------------------------------------------------------------------------------------------
	string GetText()
	{
		return m_sMarkerText;
	}

}

//------------------------------------------------------------------------------------------------
class SCR_MapMarkerMilitaryTitle: BaseContainerCustomTitle
{
	//------------------------------------------------------------------------------------------------
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{		
		int id;
		source.Get("m_iEntryID", id);
		
		EMilitarySymbolDimension dimension;
		source.Get("m_Dimension", dimension);
		
		EMilitarySymbolIcon symbols;
		source.Get("m_Icons", symbols);

		title = id.ToString() + ": " + typename.EnumToString(EMilitarySymbolDimension, dimension) + " | " + SCR_Enum.FlagsToString(EMilitarySymbolIcon, symbols, " ");
		
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Class container which holds faction specific predefined marker entries
[BaseContainerProps(), SCR_MapMarkerFactionTitle()]
class SCR_MarkerMilitaryFactionEntry
{
	[Attribute("0", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EMilitarySymbolIdentity))]
	protected EMilitarySymbolIdentity m_FactionIdentity;
	
	[Attribute("1.0 1.0 1.0 1.0")]
	protected ref Color m_FactionColor;
	
	//------------------------------------------------------------------------------------------------
	EMilitarySymbolIdentity GetFactionIdentity()
	{
		return m_FactionIdentity;
	}
	
	//------------------------------------------------------------------------------------------------
	Color GetColor()
	{
		return new Color(m_FactionColor.R(), m_FactionColor.G(), m_FactionColor.B(), m_FactionColor.A());
	}
}

//------------------------------------------------------------------------------------------------
class SCR_MapMarkerFactionTitle: BaseContainerCustomTitle
{
	//------------------------------------------------------------------------------------------------
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{		
		EMilitarySymbolIdentity faction;
		source.Get("m_FactionIdentity", faction);
				
		title = typename.EnumToString(EMilitarySymbolIdentity, faction);
		
		return true;
	}
}