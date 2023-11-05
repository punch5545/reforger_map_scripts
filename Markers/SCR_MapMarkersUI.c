//------------------------------------------------------------------------------------------------
//! Markers UI map component 
class SCR_MapMarkersUI : SCR_MapUIBaseComponent
{
	[Attribute("Markers", UIWidgets.Auto, "Menu category name" )]
	protected string m_sCategoryName;
	
	[Attribute("{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", UIWidgets.ResourceNamePicker, desc: "Icons imageset", params: "imageset" )]
	protected ResourceName m_sIconImageset;
	
	[Attribute("scenarios", UIWidgets.Auto, "Category icon quad" )]
	protected string m_sCategoryIconName;
	
	[Attribute("{46C46D97D1FE6241}UI/layouts/Map/MapMarkerEditBox.layout", UIWidgets.ResourceNamePicker, desc: "Edit box dialog when placing custom marker", params: "layout" )]
	protected ResourceName m_sEditBoxLayout;
	
	[Attribute("cancel", UIWidgets.Auto, "Delete icon quad" )]
	protected string m_sDeleteIconName;
	
	[Attribute("6", UIWidgets.Auto, "Color selector entries per line" )]
	protected int m_iColorsPerLine;
	
	[Attribute("6", UIWidgets.Auto, "Icon selector entries per line" )]
	protected int m_iIconsPerLine;
	
	protected const string BUTTON_WIDGET = "ButtonPublic";	// TEMP
	
	protected SCR_MapMarkerEntryPlaced m_SavedEntry;		// saved entry for custom text placed markers
	protected Widget m_MarkerEditRoot;
	protected ImageWidget m_wMarkerPreview;
	
	protected int m_iSelectedColorIndex;
	protected int m_iSelectedIconIndex;
	protected SCR_EditBoxComponent m_LastEditBoxComp;
	
	protected SCR_MapMarkerManagerComponent m_MarkerMgr;
	protected SCR_SelectionMenuCategoryEntry m_RootCategoryEntry;
	protected SCR_SelectionMenuEntry m_MarkerRemoveEntry;
	protected SCR_MapMarkerBase m_RemovableMarker;
	
	//------------------------------------------------------------------------------------------------
	//! Check whether the marker is owned by the local player
	static bool IsOwnedMarker(notnull SCR_MapMarkerBase marker)
	{
		PlayerController localController = GetGame().GetPlayerController();
		if (localController)
		{
			if (marker.GetMarkerOwnerID() == localController.GetPlayerId())
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Spawn a placed marker 
	protected void CreateMarkerPlaced(SCR_MapMarkerMenuEntry entry)
	{
		SCR_MapMarkerConfig markerConfig = m_MarkerMgr.GetMarkerConfig();
		if (!markerConfig)
			return;
		
		SCR_MapMarkerEntryPlaced markerEntry = SCR_MapMarkerEntryPlaced.Cast(markerConfig.GetMarkerEntryConfigByType(entry.GetMarkerType()));
		
		if (entry.GetMarkerType() == SCR_EMapMarkerType.PLACED_CUSTOM)
		{
			CreateMarkerEditDialog(markerEntry);
		}		
		else if (entry.GetMarkerType() == SCR_EMapMarkerType.PLACED_MILITARY)			
		{
			float wX, wY;
			m_MapEntity.GetMapCenterWorldPosition(wX, wY);
			
			SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
			marker.SetType(entry.GetMarkerType());
			marker.SetWorldPos(wX, wY);
			marker.SetMarkerConfigID(entry.GetMarkerConfigID());
			m_MarkerMgr.InsertStaticMarker(marker);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Create custom marker dialog
	protected void CreateMarkerEditDialog(SCR_MapMarkerEntryPlaced markerEntry)
	{
		//! TODO unhardcode 
		
		string imageset, quad;
		int refX, refY;
		
		m_SavedEntry = markerEntry;
		WidgetManager.GetReferenceScreenSize(refX, refY);
		
		if (m_MarkerEditRoot)
			CleanupMarkerEditWidget();
		
		m_MarkerEditRoot = GetGame().GetWorkspace().CreateWidgets(m_sEditBoxLayout, m_RootWidget);
		FrameSlot.SetPos(m_MarkerEditRoot, refX * 0.5, refY * 0.5);
		
		m_wMarkerPreview = ImageWidget.Cast(m_MarkerEditRoot.FindAnyWidget("MarkerPreview"));
		
		Widget colorSelector = m_MarkerEditRoot.FindAnyWidget("ColorSelector");
		array<ref SCR_MarkerColorEntry> colorsArr = markerEntry.GetColorEntries();
		
		Widget colorSelectorLine = m_MarkerEditRoot.FindAnyWidget("ColorSelectorLine0");
		int colorEntryCount;
		
		foreach (int i, SCR_MarkerColorEntry colorEntry : colorsArr)
		{
			colorEntryCount++;
			if (colorEntryCount > m_iColorsPerLine)
			{
				colorSelectorLine = GetGame().GetWorkspace().CreateWidgets("{CF8EC7A0D310A8D9}UI/layouts/Map/MapColorSelectorLine.layout", colorSelector);
				colorEntryCount = 1;
			}
			
			Widget button = GetGame().GetWorkspace().CreateWidgets("{8A5D43FC8AC6C171}UI/layouts/Map/MapColorSelectorEntry.layout", colorSelectorLine);
			button.SetName(i.ToString() + "ColorEntry");
			SCR_ButtonImageComponent buttonComp = SCR_ButtonImageComponent.Cast(button.FindHandler(SCR_ButtonImageComponent));
			Color colorHover = colorEntry.GetColor();
			colorHover.SetA(0.8);
			buttonComp.SetBackgroundColors(colorEntry.GetColor(), colorHover);
			buttonComp.ColorizeBackground();
			buttonComp.m_OnClicked.Insert(OnColorEntryClicked);

		}
		
		Widget iconSelector = m_MarkerEditRoot.FindAnyWidget("IconSelector");
		array<ref SCR_MarkerIconEntry> iconsArr = markerEntry.GetIconEntries();
		
		Widget iconSelectorLine = m_MarkerEditRoot.FindAnyWidget("IconSelectorLine0");
		int iconEntryCount;
		
		foreach (int j, SCR_MarkerIconEntry iconEntry : iconsArr)
		{
			iconEntryCount++;
			if (iconEntryCount > m_iIconsPerLine)
			{
				iconSelectorLine = GetGame().GetWorkspace().CreateWidgets("{CF8EC7A0D310A8D9}UI/layouts/Map/MapColorSelectorLine.layout", iconSelector);
				iconEntryCount = 1;
			}
			
			Widget button = GetGame().GetWorkspace().CreateWidgets("{DEA2D3B788CDCB4F}UI/layouts/Map/MapIconSelectorEntry.layout", iconSelectorLine);
			button.SetName(j.ToString() + "IconEntry");
			SCR_ButtonImageComponent buttonComp = SCR_ButtonImageComponent.Cast(button.FindHandler(SCR_ButtonImageComponent));
			
			iconEntry.GetIconResource(imageset, quad);
			buttonComp.SetImage(imageset, quad);
			buttonComp.m_OnClicked.Insert(OnIconEntryClicked);

		}
		
		m_LastEditBoxComp =  SCR_EditBoxComponent.Cast(m_MarkerEditRoot.FindAnyWidget("EditBoxRoot").FindHandler(SCR_EditBoxComponent));
		
		SCR_ButtonTextComponent confirmComp =  SCR_ButtonTextComponent.Cast(m_MarkerEditRoot.FindAnyWidget("ButtonPublic").FindHandler(SCR_ButtonTextComponent));
		confirmComp.m_OnClicked.Insert(OnEditConfirmed);
		
		SCR_ButtonTextComponent confirmComp2 =  SCR_ButtonTextComponent.Cast(m_MarkerEditRoot.FindAnyWidget("ButtonPrivate").FindHandler(SCR_ButtonTextComponent));
		confirmComp2.m_OnClicked.Insert(OnEditConfirmed);
		
		m_wMarkerPreview.SetColor(colorsArr[m_iSelectedColorIndex].GetColor());
		
		iconsArr[m_iSelectedIconIndex].GetIconResource(imageset, quad);
		m_wMarkerPreview.LoadImageFromSet(0, imageset, quad);
		
		GetGame().GetWorkspace().SetFocusedWidget(confirmComp.GetRootWidget());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Init radial menu marker entries along with their visual previews
	protected void InitFactionPlacedMarkers(SCR_MapMarkerConfig markerConfig, SCR_MapRadialUI radialUI)
	{
		SCR_MapMarkerEntryMilitary milConf = SCR_MapMarkerEntryMilitary.Cast(markerConfig.GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_MILITARY));
		if (!milConf)
			return;
		
		array<ref SCR_MarkerMilitaryFactionEntry> milFactionEntries = milConf.GetMilitaryFactionEntries();
		array<ref SCR_MarkerMilitaryEntry> milEntries = milConf.GetMilitaryEntries();
		
		if (milFactionEntries.IsEmpty() || milEntries.IsEmpty())
			return;
		
		foreach (int i, SCR_MarkerMilitaryFactionEntry milFaction : milFactionEntries)
		{			
			SCR_MapMarkerMenuCategory categoryEntry = new SCR_MapMarkerMenuCategory();
			categoryEntry.SetMarkerType(SCR_EMapMarkerType.PLACED_MILITARY);
			categoryEntry.SetLayout();
			categoryEntry.SetSymbolProps(milFaction.GetFactionIdentity(), milFaction.GetColor());
			radialUI.InsertCustomRadialCategory(categoryEntry, m_RootCategoryEntry);
			
			foreach (SCR_MarkerMilitaryEntry milEntry : milEntries)
			{
				SCR_MapMarkerMenuEntry menuEntry = new SCR_MapMarkerMenuEntry();
				menuEntry.SetMarkerType(SCR_EMapMarkerType.PLACED_MILITARY);
				menuEntry.SetLayout();
				menuEntry.SetName(milEntry.GetDescription());
				menuEntry.GetOnPerform().Insert(OnEntryPerformed);
				menuEntry.SetSymbolProps(milFaction.GetFactionIdentity(), milFaction.GetColor(), milEntry.GetDimension(), milEntry.GetIcons(), milEntry.GetAmplifier());
						
				menuEntry.SetMarkerConfigID(i * 1000 + milEntry.GetEntryID());

				radialUI.InsertCustomRadialEntry(menuEntry, categoryEntry);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Create local markers
	protected void CreateLocalMarkers()
	{
		array<ref SCR_MapMarkerBase> markers = m_MarkerMgr.GetLocalMarkers();
		foreach (SCR_MapMarkerBase marker : markers)
		{
			marker.OnCreateMarker();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Create static markers
	protected void CreateStaticMarkers()
	{
		array<ref SCR_MapMarkerBase> markersSimple = m_MarkerMgr.GetStaticMarkers();
		for (int i; i < markersSimple.Count(); i++)
		{
			if (!markersSimple.IsIndexValid(i))
				continue;
			
			SCR_MapMarkerBase marker = markersSimple[i];
			Faction markerFaction = SCR_FactionManager.SGetPlayerFaction(marker.GetMarkerOwnerID());
			Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
			if ( (marker.GetMarkerOwnerID() != GetGame().GetPlayerController().GetPlayerId()) && (!localFaction || localFaction.IsFactionEnemy(markerFaction)))
			{
				if (Replication.IsServer())				// if server, enemy markers have to be kept for sync but are disabled
					marker.SetServerDisabled(true);
				else 									// if client, enemy marker can be safely removed
				{
					markersSimple.RemoveItem(marker);
					i--;
					continue;
				}
			}
			
			marker.OnCreateMarker();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Create dynamic markers
	protected void CreateDynamicMarkers()
	{
		array<SCR_MapMarkerEntity> markersDynamic = m_MarkerMgr.GetDynamicMarkers();
		foreach (SCR_MapMarkerEntity marker : markersDynamic)
		{
			marker.OnCreateMarker();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Attemt to remove marker, only works if owned
	protected void RemoveOwnedMarker(SCR_SelectionMenuEntry entry)
	{		
		if (!m_RemovableMarker)
			return;
		
		if (m_RemovableMarker.GetMarkerID() == -1)		// basic
			m_MarkerMgr.RemoveLocalMarker(m_RemovableMarker);
		else 
			m_MarkerMgr.RemoveStaticMarker(m_RemovableMarker);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Marker edit widget removal
	protected void CleanupMarkerEditWidget()
	{
		if (m_MarkerEditRoot)
			m_MarkerEditRoot.RemoveFromHierarchy();
	}
	
	//------------------------------------------------------------------------------------------------
	// EVENTS
	//------------------------------------------------------------------------------------------------
	//! SCR_ButtonImageComponent event
	protected void OnColorEntryClicked(SCR_ButtonBaseComponent component)
	{
		m_iSelectedColorIndex = component.GetRootWidget().GetName().ToInt();
		
		m_wMarkerPreview.SetColor(m_SavedEntry.GetColorEntry(m_iSelectedColorIndex));
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_ButtonImageComponent event
	protected void OnIconEntryClicked(SCR_ButtonBaseComponent component)
	{
		m_iSelectedIconIndex = component.GetRootWidget().GetName().ToInt();

		ResourceName imageset;
		string quad;
		
		m_SavedEntry.GetIconEntry( m_iSelectedIconIndex, imageset, quad);
		
		m_wMarkerPreview.LoadImageFromSet(0, imageset, quad);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_ButtonTextComponent event
	protected void OnEditConfirmed(SCR_ButtonTextComponent button)
	{
		if (!m_SavedEntry)
			return;
		
		float wX, wY;
		m_MapEntity.GetMapCenterWorldPosition(wX, wY);
		
		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(m_SavedEntry.GetMarkerType());
		marker.SetWorldPos(wX, wY);
		marker.SetColorEntry(m_iSelectedColorIndex);
		marker.SetIconEntry(m_iSelectedIconIndex);
		marker.SetCustomText(m_LastEditBoxComp.GetValue());
						
		if (button.GetRootWidget().GetName() == BUTTON_WIDGET)
			m_MarkerMgr.InsertStaticMarker(marker);
		else
			m_MarkerMgr.InsertLocalMarker(marker);
		
		CleanupMarkerEditWidget();
		
		m_SavedEntry = null;
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_MapRadialUI event
	protected void OnRadialMenuInit()
	{
		SCR_MapMarkerConfig markerConfig = m_MarkerMgr.GetMarkerConfig();
		if (!markerConfig)
			return;
		
		SCR_MapRadialUI radialUI = SCR_MapRadialUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapRadialUI));

		m_RootCategoryEntry = radialUI.AddRadialCategory(m_sCategoryName);
		m_RootCategoryEntry.SetIcon(m_sIconImageset, m_sCategoryIconName);
		
		array<ref SCR_MapMarkerEntryConfig> entryConfigs = markerConfig.GetMarkerEntryConfigs();
		
		foreach (SCR_MapMarkerEntryConfig entry : entryConfigs)		// menu entries
		{			
			if (entry.GetMarkerType() == SCR_EMapMarkerType.PLACED_MILITARY)
			{
				InitFactionPlacedMarkers(markerConfig, radialUI);
			}
			else if (entry.GetMarkerType() == SCR_EMapMarkerType.PLACED_CUSTOM)
			{
				SCR_MapMarkerEntryPlaced entryPlaced = SCR_MapMarkerEntryPlaced.Cast(entry);
				
				SCR_MapMarkerMenuEntry menuEntry = new SCR_MapMarkerMenuEntry();
				menuEntry.SetMarkerType(SCR_EMapMarkerType.PLACED_CUSTOM);
				menuEntry.SetName(entryPlaced.GetMenuDescription());
				menuEntry.GetOnPerform().Insert(OnEntryPerformed);
				menuEntry.SetIcon(entryPlaced.GetMenuImageset(), entryPlaced.GetMenuIcon());
	
				radialUI.InsertCustomRadialEntry(menuEntry, m_RootCategoryEntry);
			}
		}		
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_RadialMenuController event
	protected void OnRadialMenuOpen(SCR_RadialMenuController controller)
	{		
		SCR_MapRadialUI radialUI = SCR_MapRadialUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapRadialUI));
		if (m_MarkerRemoveEntry)
		{
			radialUI.RemoveRadialEntry(m_MarkerRemoveEntry);
			m_RemovableMarker = null;
		}
		
		// delete marker button
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		
		SCR_MapMarkerWidgetComponent markerComp;
		foreach ( Widget widget : widgets )
		{
			markerComp = SCR_MapMarkerWidgetComponent.Cast(widget.FindHandler(SCR_MapMarkerWidgetComponent));	
			if (!markerComp)
				continue;
						
			SCR_MapMarkerBase marker = m_MarkerMgr.GetMarkerByWidget(widget);
			if (marker)
			{
				if (!IsOwnedMarker(marker) || (marker.GetType() != SCR_EMapMarkerType.PLACED_CUSTOM && marker.GetType() != SCR_EMapMarkerType.PLACED_MILITARY))
					continue;
				
				m_MarkerRemoveEntry = radialUI.AddRadialEntry("Delete Marker");
				m_MarkerRemoveEntry.SetIcon(m_sIconImageset, m_sDeleteIconName);
				m_MarkerRemoveEntry.GetOnPerform().Insert(RemoveOwnedMarker);
				
				m_RemovableMarker = marker;
				
				return;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_MapMarkerMenuEntry event
	protected void OnEntryPerformed(SCR_SelectionMenuEntry entry)
	{
		SCR_MapMarkerMenuEntry markerEntry = SCR_MapMarkerMenuEntry.Cast(entry);
		CreateMarkerPlaced(markerEntry);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Quick open input for marker category
	protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		SCR_MapRadialUI mapRadial = SCR_MapRadialUI.GetInstance();
		if (!mapRadial)
			return;
		
		mapRadial.GetRadialController().OnInputOpen();
		mapRadial.GetRadialController().GetRadialMenu().PerformEntry(m_RootCategoryEntry);
	
	}
	
	//------------------------------------------------------------------------------------------------
	//! Marker delete quickbind
	protected void OnInputMarkerDelete(float value, EActionTrigger reason)
	{
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		
		SCR_MapMarkerWidgetComponent markerComp;
		foreach ( Widget widget : widgets )
		{
			markerComp = SCR_MapMarkerWidgetComponent.Cast(widget.FindHandler(SCR_MapMarkerWidgetComponent));	
			if (!markerComp)
				continue;
						
			SCR_MapMarkerBase marker = m_MarkerMgr.GetMarkerByWidget(widget);
			if (marker)
			{
				if (!IsOwnedMarker(marker) || (marker.GetType() != SCR_EMapMarkerType.PLACED_CUSTOM && marker.GetType() != SCR_EMapMarkerType.PLACED_MILITARY))
					continue;
				
				m_RemovableMarker = marker;
				RemoveOwnedMarker(null);
			}
				
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Menu back/espace bind
	protected void OnInputMenuBack(float value, EActionTrigger reason)
	{
		if (m_MarkerEditRoot)
			CleanupMarkerEditWidget();
	}
	
	//------------------------------------------------------------------------------------------------
	// OVERRIDES
	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		
		m_MarkerMgr = SCR_MapMarkerManagerComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_MapMarkerManagerComponent));
		
		CreateLocalMarkers();
		CreateStaticMarkers();
		CreateDynamicMarkers();
		
		m_MarkerMgr.EnableUpdate(true);		// run frame update manager side
		
		GetGame().GetInputManager().AddActionListener("MapQuickMarkerMenu", EActionTrigger.DOWN, OnInputQuickMarkerMenu);
		GetGame().GetInputManager().AddActionListener("MapMarkerDelete", EActionTrigger.DOWN, OnInputMarkerDelete);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnInputMenuBack);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{		
		CleanupMarkerEditWidget();
		GetGame().GetInputManager().RemoveActionListener("MapQuickMarkerMenu", EActionTrigger.DOWN, OnInputQuickMarkerMenu);
		GetGame().GetInputManager().RemoveActionListener("MapMarkerDelete", EActionTrigger.DOWN, OnInputMarkerDelete);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnInputMenuBack);
		
		m_MarkerMgr.EnableUpdate(false);
		super.OnMapClose(config);
	}

	//------------------------------------------------------------------------------------------------
	override void Init()
	{
		SCR_MapRadialUI radialMenu = SCR_MapRadialUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapRadialUI));
		if (radialMenu)
		{
			radialMenu.GetOnMenuInitInvoker().Insert(OnRadialMenuInit);
			radialMenu.GetRadialController().GetOnInputOpen().Insert(OnRadialMenuOpen);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		if (m_MarkerEditRoot)
		{
			GetGame().GetInputManager().ActivateContext("MapMarkerEditContext");
		}
	}
}