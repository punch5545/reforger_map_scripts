class SCR_MapEntityClass: MapEntityClass
{
};

//------------------------------------------------------------------------------------------------
// invoker typedefs
void MapConfigurationInvoker(MapConfiguration config);
typedef func MapConfigurationInvoker;

void MapItemInvoker(MapItem mapItem);
typedef func MapItemInvoker;

void ScriptInvokerFloat2Bool(float f1, float f2, bool b1);
typedef func ScriptInvokerFloat2Bool;


//------------------------------------------------------------------------------------------------
//! Map entity
[EntityEditorProps(category: "GameScripted/Map", description: "Map entity, handles displaying of map etc", sizeMin: "-5 -5 -5", sizeMax: "5 5 5", color: "255 255 200 0", dynamicBox: true)]
class SCR_MapEntity: MapEntity
{				
	const int FRAME_DELAY = 1;	
	protected int m_iDelayCounter = FRAME_DELAY;			// used to delay the map logic by set amount of frames to give the map widget time to properly init
	
	// generic
	protected bool m_bIsOpen;								// is open flag
	protected bool m_bDoReload;								// mark whether map config changed in order to reload modules/components
	protected bool m_bDoUpdate;								// mark whether user setting changed, update zoom & position
	protected bool m_bIsDebugMode;							// variable debug mode
	protected int m_iMapSize[2];							// map size in meters/units
	protected EMapEntityMode m_eLastMapMode;				// cached mode of last map for reload check
	protected Widget m_wMapRoot;							// map menu root widget
	protected CanvasWidget m_MapWidget;						// map widget
	protected WorkspaceWidget m_Workspace;
	protected ref MapConfiguration m_ActiveMapCfg; 			// map config
	protected ref SCR_MapDescriptorDefaults m_DefaultsCfg;	// descriptor defaults
	protected static SCR_MapEntity s_MapInstance;			// map entity instance
	
	protected MapItem m_HoveredMapItem;						// currently hovered map item
		
	// zoom
	protected bool m_bIsZoomInterp;		// is currently zoom animating
	protected float m_fZoomPPU = 1;		// current zoom PixelPerUnit value
	protected float m_fStartPPU;		// zoom start PixelPerUnit
	protected float m_fTargetPPU = 1;	// zoom target PixelPerUnit
	protected float m_fZoomTimeModif;	// zoom anim speed modifier
	protected float m_fZoomSlice;		// zoom anim timeslce
	protected float m_fMinZoom = 1;		// minimal zoom PixelPerUnit
	protected float m_fMaxZoom;			// maximal zoom PixelPerUnit
	
	// pan
	protected bool m_bIsPanInterp;		// is currently pan animating
	protected float m_fPanX = 0;		// current horizontal pan offset - UNSCALED value in px
	protected float m_fPanY = 0;		// current vertical pan offset - UNSCALED value in px
	protected float m_aStartPan[2];		// pan start coords - UNSCALED value in px
	protected float m_aTargetPan[2];	// pan target coords - UNSCALED value in px
	protected float m_fPanTimeModif;	// pan anim speed modifier
	protected float m_fPanSlice;		// pan anim timeslce
	
	// modules & components
	protected ref array<ref SCR_MapModuleBase> m_aActiveModules = {};
	protected ref array<ref SCR_MapModuleBase> m_aLoadedModules = {};
	protected ref array<ref SCR_MapUIBaseComponent> m_aActiveComponents = {};
	protected ref array<ref SCR_MapUIBaseComponent> m_aLoadedComponents = {};
	
	// invokers
	protected static ref ScriptInvokerBase<MapConfigurationInvoker> s_OnMapInit = new ScriptInvokerBase<MapConfigurationInvoker>();	// map init, called straight after opening the map
	protected static ref ScriptInvokerBase<MapConfigurationInvoker> s_OnMapOpen = new ScriptInvokerBase<MapConfigurationInvoker>();	// map open, called after map is properly initialized
	protected static ref ScriptInvokerBase<MapConfigurationInvoker> s_OnMapClose = new ScriptInvokerBase<MapConfigurationInvoker>();// map close
	protected static ref ScriptInvokerBase<ScriptInvokerFloat2Bool> s_OnMapPan 	= new ScriptInvokerBase<ScriptInvokerFloat2Bool>;	// map pan, passes UNSCALED x & y
	protected static ref ScriptInvoker<float, float> s_OnMapPanEnd 				= new ScriptInvoker();						// map pan interpolated end
	protected static ref ScriptInvokerFloat s_OnMapZoom							= new ScriptInvokerFloat();							// map zoom
	protected static ref ScriptInvokerFloat s_OnMapZoomEnd 						= new ScriptInvokerFloat();							// map zoom interpolated end
	protected static ref ScriptInvokerVector s_OnSelection 						= new ScriptInvokerVector();						// any click/selection on map
	protected static ref ScriptInvokerInt s_OnLayerChanged 						= new ScriptInvokerInt();							// map layer changed
	protected static ref ScriptInvokerBase<MapItemInvoker> s_OnSelectionChanged = new ScriptInvokerBase<MapItemInvoker>();			// map items de/selected
	protected static ref ScriptInvokerBase<MapItemInvoker> s_OnHoverItem 		= new ScriptInvokerBase<MapItemInvoker>();			// map item hovered
	protected static ref ScriptInvokerBase<MapItemInvoker> s_OnHoverEnd 		= new ScriptInvokerBase<MapItemInvoker>();			// map item hover end
	
	//TEMP & TODOs
	protected int m_iLayerIndex = 0;	// current layer (switch to getter from gamecode)	
	ref array<int> imagesetIndices = new array<int>();
	
	//------------------------------------------------------------------------------------------------
	// GETTERS / SETTERS
	//------------------------------------------------------------------------------------------------
	//! Get on map init invoker, caution: called during the first frame of opening the map when widget related stuff is not initialized yet
	static ScriptInvokerBase<MapConfigurationInvoker> GetOnMapInit() { return s_OnMapInit; }
	//! Get on map open invoker
	static ScriptInvokerBase<MapConfigurationInvoker> GetOnMapOpen() { return s_OnMapOpen; }
	//! Get on map close invoker
	static ScriptInvokerBase<MapConfigurationInvoker> GetOnMapClose() { return s_OnMapClose; }
	//! Get on map pan invoker
	static ScriptInvokerBase<ScriptInvokerFloat2Bool> GetOnMapPan() { return s_OnMapPan; }
	//! Get on map pan interpolated end invoker
	static ScriptInvoker GetOnMapPanEnd() { return s_OnMapPanEnd; }
	//! Get on map zoom invoker
	static ScriptInvokerFloat GetOnMapZoom() { return s_OnMapZoom; }
	//! Get on map zoom interpolated end invoker
	static ScriptInvokerFloat GetOnMapZoomEnd() { return s_OnMapZoomEnd; }
	//! Get on selection changed invoker
	static ScriptInvokerBase<MapItemInvoker> GetOnSelectionChanged() { return s_OnSelectionChanged; }
	//! Get on selection invoker
	static ScriptInvokerVector GetOnSelection() { return s_OnSelection; }
	//! Get on hover item invoker
	static ScriptInvokerBase<MapItemInvoker> GetOnHoverItem() { return s_OnHoverItem; }
	//! Get on hover end invoker
	static ScriptInvokerBase<MapItemInvoker> GetOnHoverEnd() { return s_OnHoverEnd; }
	//! Get on layer changed invoker
	static ScriptInvokerInt GetOnLayerChanged() { return s_OnLayerChanged; }
	//! Get map entity instance
	static SCR_MapEntity GetMapInstance() { return s_MapInstance; }
	
	//! Get map config
	MapConfiguration GetMapConfig() { return m_ActiveMapCfg; }
	
	//! Check if the map is opened
	bool IsOpen() { return m_bIsOpen; }
	//! Get map sizeX in meters
	int GetMapSizeX() { return m_iMapSize[0]; }
	//! Get map sizeY in meters
	int GetMapSizeY() { return m_iMapSize[1]; }
	//! Get maximal zoom
	float GetMaxZoom() { return m_fMaxZoom; }
	//! Get minimal zoom
	float GetMinZoom() { return m_fMinZoom; }
	//! Get target zoom in the form of PixelPerUnit value, which is different from current zoom if interpolation is ongoing
	float GetTargetZoomPPU() { return m_fTargetPPU; }
	//! Get whether zoom interpolation is ongoing
	bool IsZooming() { return m_bIsZoomInterp;	}
	//! Get current DPIScaled pan offsets 
	vector GetCurrentPan() { return Vector(m_Workspace.DPIScale(m_fPanX), m_Workspace.DPIScale(m_fPanY), 0); }
	//! Get map widget
	CanvasWidget GetMapWidget() { return m_MapWidget; }
	//! Get map menu root widget
	Widget GetMapMenuRoot() { return m_wMapRoot; }
	//! Set map widget
	void SetMapWidget(Widget mapW) { m_MapWidget = CanvasWidget.Cast(mapW); }
	//! Get hovered item
	MapItem GetHoveredItem() { return m_HoveredMapItem; }
		
	//------------------------------------------------------------------------------------------------
	//! Get how much pixels per unit(meter) are currently visible on screen. If this value is 1 and resolution is 1920x1080, then 1920 units(meters) of map will be visible
	//! \return Current pixel per unit value
	float GetCurrentZoom()
	{		
		return m_fZoomPPU;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get map cursor world position
	//! \param worldX is x coord
	//! \param worldY is y coord
	void GetMapCursorWorldPosition(out float worldX, out float worldY)
	{
		ScreenToWorld(m_Workspace.DPIScale(SCR_MapCursorInfo.x), m_Workspace.DPIScale(SCR_MapCursorInfo.y), worldX, worldY);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get map center of screen world position
	//! \param worldX is x coord
	//! \param worldY is y coord
	void GetMapCenterWorldPosition(out float worldX, out float worldY)
	{
		float screenX, screenY;
		m_MapWidget.GetScreenSize(screenX, screenY);
		
		ScreenToWorld(screenX/2, screenY/2, worldX, worldY);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get a specific map module
	//! \param moduleType is a typename of the wanted module
	//! \return Returns requested module or null if not found
	SCR_MapModuleBase GetMapModule(typename moduleType)
	{
		foreach ( SCR_MapModuleBase module : m_aActiveModules )
		{
			if ( module.IsInherited(moduleType) )
				return module;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get a specific map UI component
	//! \param moduleType is a typename of the wanted UI component
	//! \return Returns requested UI component or null if not found
	SCR_MapUIBaseComponent GetMapUIComponent(typename componentType)
	{
		foreach ( SCR_MapUIBaseComponent comp : m_aActiveComponents )
		{
			if ( comp.IsInherited(componentType) )
				return comp;
		}
		
		return null;
	}
		
	//------------------------------------------------------------------------------------------------
	// OPEN / CLOSE
	//------------------------------------------------------------------------------------------------
	//! Open the map
	//! \param config is the configuration object
	void OpenMap(MapConfiguration config)
	{
		if (!config)
			return;
		
		if (m_bIsOpen)
		{
			Print("SCR_MapEntity: Attempted opening a map while it is already open", LogLevel.WARNING);
			CloseMap();
		}
		
		if (config.MapEntityMode != m_eLastMapMode)
			m_bDoReload = true;
		
		m_eLastMapMode = config.MapEntityMode;
		m_ActiveMapCfg = config;
		m_Workspace = GetGame().GetWorkspace();
		m_wMapRoot = config.RootWidgetRef;
		
		SetMapWidget(config.RootWidgetRef.FindAnyWidget(SCR_MapConstants.MAP_WIDGET_NAME));		
		
		if (config.MapEntityMode == EMapEntityMode.FULLSCREEN)
		{			
			ChimeraCharacter char = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
			if (char)
				SCR_CharacterControllerComponent.Cast(char.GetCharacterController()).m_OnPlayerDeathWithParam.Insert(OnPlayerDeath);
		}
		
		InitLayers(config.LayerCount, config.LayerConfigs);
				
		SetFrame(Vector(0, 0, 0), Vector(0, 0, 0)); // Gamecode starts rendering stuff like descriptors straight away instead of waiting a frame - this is a hack to display nothing, avoiding the "blink" of icons
		
		m_bIsOpen = true;
		
		s_OnMapInit.Invoke(config);

		PlayerController plc = GetGame().GetPlayerController();
		if (plc && GetGame().GetCameraManager().CurrentCamera() == plc.GetPlayerCamera())
			plc.SetCharacterCameraRenderActive(false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Close the map
	void CloseMap()
	{
		OnMapClose();
		
		m_iLayerIndex = -1;
		m_bIsOpen = false;
		m_DefaultsCfg = null;
		m_iDelayCounter = FRAME_DELAY;
		auto plc = GetGame().GetPlayerController();
		if (plc)
			plc.SetCharacterCameraRenderActive(true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Map open event
	//! \param config is the configuration object
	protected void OnMapOpen(MapConfiguration config)
	{						
		// init zoom & layers
		vector mapSize = {m_iMapSize[0], m_iMapSize[1], 0};		
		m_MapWidget.SetSizeInUnits(mapSize);				// unit size to meters
		UpdateZoomBounds();
		AssignViewLayer();
		
		if (m_bDoUpdate)	// when resolution changes, zoom to the same PPU to update zoom and pos
		{
			ZoomSmooth(m_fZoomPPU);
			m_bDoUpdate = false;
		}
		
		// activate modules & components
		ActivateModules(config.Modules);
		ActivateComponents(config.Components);
		ActivateOtherComponents(config.OtherComponents);
		
		m_bDoReload = false;
		
		if (s_OnMapOpen)
			s_OnMapOpen.Invoke(config);

		if (config.MapEntityMode == EMapEntityMode.FULLSCREEN)
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_HUD_MAP_OPEN);
		
		EnableVisualisation(true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Map close event
	protected void OnMapClose()
	{
		if (s_OnMapClose)
			s_OnMapClose.Invoke(m_ActiveMapCfg);
		
		if (m_ActiveMapCfg.MapEntityMode == EMapEntityMode.FULLSCREEN)
		{
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_HUD_MAP_CLOSE);
			
			PlayerController controller = GetGame().GetPlayerController();
			if (controller)
			{
				ChimeraCharacter char = ChimeraCharacter.Cast(controller.GetControlledEntity());
				if (char)
					SCR_CharacterControllerComponent.Cast(char.GetCharacterController()).m_OnPlayerDeathWithParam.Remove(OnPlayerDeath);
			}
		}
		
		if ( m_ActiveMapCfg.OtherComponents & EMapOtherComponents.LEGEND_SCALE)
			EnableLegend(false);
		
		EnableVisualisation(false);
		
		Cleanup();
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_CharacterControllerComponent event
	//! Called only in case of a fullscreen map
	protected void OnPlayerDeath(SCR_CharacterControllerComponent charController, IEntity instigator)
	{		
		SCR_GadgetManagerComponent gadgetMgr = SCR_GadgetManagerComponent.GetGadgetManager(GetGame().GetPlayerController().GetControlledEntity());
		if (!gadgetMgr)
			return;
		
		IEntity mapGadget = gadgetMgr.GetGadgetByType(EGadgetType.MAP);
		if (!mapGadget)
			return;
		
		SCR_MapGadgetComponent mapComp = SCR_MapGadgetComponent.Cast(mapGadget.FindComponent(SCR_MapGadgetComponent));
		mapComp.SetMapMode(false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Game event
	protected void OnUserSettingsChanged()
	{
		if (m_bIsOpen)
			ZoomSmooth(m_fZoomPPU);
		else
			m_bDoUpdate = true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Game event
	protected void OnWindowResized(int width, int heigth, bool windowed)
	{
		OnUserSettingsChanged();
	}
	
	//------------------------------------------------------------------------------------------------
	// MAP SETUP
	//------------------------------------------------------------------------------------------------
	//! Prepare MapConfiguration object from provided map config
	//! \param mapMode is the last map mode opened, if its the same we do not need to setup new config
	//! \param configPath is the path to the map config resource
	//! \param rootWidget is the root widget of the layout used to display map
	MapConfiguration SetupMapConfig(EMapEntityMode mapMode, ResourceName configPath, Widget rootWidget)
	{		
		if (mapMode == m_eLastMapMode)
		{
			m_ActiveMapCfg.RootWidgetRef = rootWidget;
			SetupDescriptorTypes(m_ActiveMapCfg, null, true);	// TODO this is here until the cached cfg is properly implemented, descriptors need to be reloaded
			return m_ActiveMapCfg;
		}
		
		// clear loaded compas and modules
		m_aLoadedComponents.Clear();
		m_aLoadedModules.Clear();
		
		// Load config
		Resource container = BaseContainerTools.LoadContainer(configPath);
		if (!container)	
			return null;	
					
		SCR_MapConfig mapConfig = SCR_MapConfig.Cast( BaseContainerTools.CreateInstanceFromContainer( container.GetResource().ToBaseContainer() ) );
		MapConfiguration configObject = new MapConfiguration();
		
		// basic
		configObject.RootWidgetRef = rootWidget;
		configObject.MapEntityMode = mapConfig.m_iMapMode;
				
		// modules & componentss
		configObject.Modules = mapConfig.m_aModules;
		configObject.Components = mapConfig.m_aUIComponents;
		
		if (mapConfig.m_bEnableLegendScale == true)
			configObject.OtherComponents |= EMapOtherComponents.LEGEND_SCALE;
		
		if (mapConfig.m_bEnableGrid == true)
			configObject.OtherComponents |= EMapOtherComponents.GRID;
			
		SetupLayers(configObject, mapConfig);
		SetupMapProps(configObject, mapConfig);
		SetupDescriptorTypes(configObject, mapConfig);
					
		return configObject;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Layer config setup
	//! \param configObject is a config object created for map init
	//! \param mapCfg is the provided map config
	protected void SetupLayers(inout MapConfiguration configObject, SCR_MapConfig mapCfg)
	{
		SCR_MapLayersBase layersCfg = mapCfg.m_LayersConfig;
		if (!layersCfg)	// use default if above fails
		{
			Resource containerDefs = BaseContainerTools.LoadContainer(SCR_MapConstants.CFG_LAYERS_DEFAULT);
			if (!containerDefs)
				return;
			
			layersCfg = SCR_MapLayersBase.Cast( BaseContainerTools.CreateInstanceFromContainer( containerDefs.GetResource().ToBaseContainer() ) );	
		}
				
		if (layersCfg.m_aLayers.IsEmpty())
			configObject.LayerCount = 0;
		else 
			configObject.LayerCount = layersCfg.m_aLayers.Count();
		
		SCR_MapDescriptorVisibilityBase descriptorViewCfg = mapCfg.m_DescriptorVisibilityConfig;
		if (!descriptorViewCfg)	// use default if above fails
		{
			Resource containerDefs = BaseContainerTools.LoadContainer(SCR_MapConstants.CFG_DESCVIEW_DEFAULT);
			if (!containerDefs)
				return;
			
			layersCfg = SCR_MapLayersBase.Cast( BaseContainerTools.CreateInstanceFromContainer( containerDefs.GetResource().ToBaseContainer() ) );	
		}
		
		if (descriptorViewCfg.m_aDescriptorViewLayers.IsEmpty())
			return;
		
		configObject.LayerConfigs = new array<ref LayerConfiguration>;
		for (int i = 0; i < configObject.LayerCount; i++)
		{
			configObject.LayerConfigs.Insert(new LayerConfiguration());
			configObject.LayerConfigs[i].LayerProps = layersCfg.m_aLayers[i];
			
			foreach (SCR_DescriptorViewLayer descriptorCfg : descriptorViewCfg.m_aDescriptorViewLayers)
			{
				// Add only if set up to this layer
				if (descriptorCfg.m_iViewLayer >= i + 1) // offset by 1 as first layer has ID = 1 in config
					configObject.LayerConfigs[i].DescriptorConfigs.Insert(new MapDescriptorConfiguration(descriptorCfg.m_iDescriptorType));
		
			}
		}	
	}
	
	//------------------------------------------------------------------------------------------------
	//! Map properties config setup
	//! \param configObject is a config object created for map init
	//! \param mapCfg is the provided map config
	protected void SetupMapProps(inout MapConfiguration configObject, SCR_MapConfig mapCfg)
	{
		// props configs
		SCR_MapPropsBase propsCfg = mapCfg.m_MapPropsConfig;
		if (!propsCfg)	// use default if above fails
		{
			Resource containerDefs = BaseContainerTools.LoadContainer(SCR_MapConstants.CFG_PROPS_DEFAULT);
			if (!containerDefs)
				return;
			
			propsCfg = SCR_MapPropsBase.Cast( BaseContainerTools.CreateInstanceFromContainer( containerDefs.GetResource().ToBaseContainer() ) );	
		}
		
		configObject.MapPropsConfigs = propsCfg.m_aMapPropConfigs;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Map properties config setup
	//! \param configObject is a config object created for map init
	//! \param mapCfg is the provided map config
	protected void SetupDescriptorTypes(inout MapConfiguration configObject, SCR_MapConfig mapCfg, bool cached = false)
	{
		if (cached)
			m_DefaultsCfg = configObject.DescriptorDefsConfig;
		else
			m_DefaultsCfg = mapCfg.m_DescriptorDefaultsConfig;
		if (!m_DefaultsCfg)
		{
			Resource containerDefs = BaseContainerTools.LoadContainer(SCR_MapConstants.CFG_DESCTYPES_DEFAULT);
			if (!containerDefs)
				return;
			
			m_DefaultsCfg = SCR_MapDescriptorDefaults.Cast( BaseContainerTools.CreateInstanceFromContainer( containerDefs.GetResource().ToBaseContainer() ) );	
		}
		
		configObject.DescriptorDefsConfig = m_DefaultsCfg;
		if (!m_DefaultsCfg.m_aDescriptorDefaults.IsEmpty())
		{
			// The constructor guarantees the size matching with EMapDescriptorType.MDT_COUNT
			for (int ii = 0; ii < EMapDescriptorType.MDT_COUNT; ++ii)
			{
				imagesetIndices[ii] = -1;
			}
			
			foreach (SCR_DescriptorDefaultsBase descriptorDefs : m_DefaultsCfg.m_aDescriptorDefaults)
			{
				imagesetIndices[descriptorDefs.m_iDescriptorType] = descriptorDefs.m_iImageSetIndex;
			}
			
			SetImagesetMapping(imagesetIndices);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// MAP CONTROL METHODS
	//------------------------------------------------------------------------------------------------
	//! Center the map
	void CenterMap()
	{
		if (m_iDelayCounter > 0)
		{
			Print("SCR_MapEntity: Attempt to call CenterMap before map init is completed", LogLevel.WARNING);
			return;
		}		
		
		//! get current size of map(includes zoom level) in screen space
		int x, y;
		WorldToScreen(GetMapSizeX() / 2, GetMapSizeY() / 2, x, y);

		SetPan(x, y); 
	}
		
	//------------------------------------------------------------------------------------------------
	//! Set minimal zoom and center the map
	void ZoomOut()
	{
		SetZoom(m_fMinZoom, true);
		CenterMap();
	}
			
	//------------------------------------------------------------------------------------------------
	//! Show/hide debug info table
	void ShowScriptDebug()
	{
		m_bIsDebugMode = !m_bIsDebugMode;
	}
		
	//------------------------------------------------------------------------------------------------
	//! Set zoom value
	//! \param targetPPU is wanted pixel per unit (zoom) value
	//! \param instant determines whether it is a one time zoom or an animation
	void SetZoom(float targetPPU, bool instant = false)
	{
		if (m_iDelayCounter > 0)
		{
			Print("SCR_MapEntity: Attempt to call SetZoom before map init is completed", LogLevel.NORMAL);
			return;
		}
		
		UpdateZoomBounds(); // TODO call this somewhere on reso/settings changed
		
		targetPPU = Math.Clamp(targetPPU, m_fMinZoom, m_fMaxZoom); 
		
		m_fZoomPPU = targetPPU;
		AssignViewLayer();
		ZoomChange(targetPPU / m_MapWidget.PixelPerUnit());	// contours
		
		if (instant)
			m_fTargetPPU = m_fZoomPPU;
		
		s_OnMapZoom.Invoke(targetPPU);
	}
		
	//------------------------------------------------------------------------------------------------
	//! Interpolated zoom
	//! \param targetPixPerUnit is the target pixel per unit value
	//! \param zoomTime is interpolation duration
	//! \param zoomToCenter determines whether zoom target is screen center (true) or relative position of mouse within window (false)
	void ZoomSmooth(float targetPixPerUnit, float zoomTime = 0.25, bool zoomToCenter = true)
	{
		if (m_iDelayCounter > 0)
		{
			Print("SCR_MapEntity: Attempt to call ZoomSmooth before map init is completed", LogLevel.NORMAL);
			return;
		}
		
		if (zoomTime <= 0)
			zoomTime = 0.1;

		targetPixPerUnit = Math.Clamp(targetPixPerUnit, m_fMinZoom, m_fMaxZoom);
		
		m_fStartPPU = m_fZoomPPU;
		m_fTargetPPU = targetPixPerUnit;
		m_fZoomTimeModif = 1/zoomTime;
		m_fZoomSlice = 1.0;
		m_bIsZoomInterp = true;
		
		float screenX, screenY, worldX, worldY, targetScreenX, targetScreenY;
		m_MapWidget.GetScreenSize(screenX, screenY);
		
		if (zoomToCenter)
		{
			// zoom according to the current screen center
			GetMapCenterWorldPosition(worldX, worldY);
			WorldToScreen( worldX, worldY, targetScreenX, targetScreenY, false, targetPixPerUnit );
			PanSmooth( targetScreenX, targetScreenY, zoomTime ); 
		}
		else
		{
			// Calculate target pan position in a way which makes cursor stay on the same world pos while zooming
			float diffX = screenX/2 - m_Workspace.DPIScale(SCR_MapCursorInfo.x); // difference in pixels between screen center and cursor
			float diffY = screenY/2 - m_Workspace.DPIScale(SCR_MapCursorInfo.y);

			GetMapCursorWorldPosition(worldX, worldY); // current cursor world pos, relative anchor of zoom
			WorldToScreen( worldX, worldY, targetScreenX, targetScreenY, false, targetPixPerUnit ); // target screen pos of cursor with zoom applied
			PanSmooth( targetScreenX + diffX, targetScreenY + diffY, zoomTime );  // offset the target position by the pix diference from screen center
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Interpolated zoom with custom pan target for simultaneous use of zoom+pan
	//! \param targetPixPerUnit is the target pixel per unit value
	//! \param worldX is world pos X panning target
	//! \param worldY is world pos Y panning target
	//! \param zoomTime is interpolation duration
	void ZoomPanSmooth(float targetPixPerUnit, float worldX, float worldY, float zoomTime = 0.25)
	{
		if (m_iDelayCounter > 0)
		{
			Print("SCR_MapEntity: Attempt to call ZoomPanSmooth before map init is completed", LogLevel.NORMAL);
			return;
		}
		
		if (zoomTime <= 0)
			zoomTime = 0.1;

		if (targetPixPerUnit > m_fMaxZoom)
			targetPixPerUnit = m_fMaxZoom;
		else if (targetPixPerUnit < m_fMinZoom)
			targetPixPerUnit = m_fMinZoom;

		m_fStartPPU = m_fZoomPPU;
		m_fTargetPPU = targetPixPerUnit;
		m_fZoomTimeModif = 1/zoomTime;
		m_fZoomSlice = 1.0;
		m_bIsZoomInterp = true;
		
		float screenX, screenY, targetScreenX, targetScreenY;
		m_MapWidget.GetScreenSize(screenX, screenY);
		
		WorldToScreen( worldX, worldY, targetScreenX, targetScreenY, false, targetPixPerUnit ); // target pos with zoom applied
		PanSmooth( targetScreenX, targetScreenY, zoomTime );
	}
		
	//------------------------------------------------------------------------------------------------
	//! Pan the map to target position, all of scripted panning is called through this
	//! \param x is horizontal screen UNSCALED coordinate
	//! \param y is vertical screen UNSCALED coordinate
	//! \param center determines whether the map should center to the supplied coordinates
	//! \param IsPanEnd determines whether this is also the end of panning operation, resetting the start pos for drag pannning
	void SetPan(float x, float y, bool isPanEnd = true, bool center = true)
	{	
		if (m_iDelayCounter > 0)
		{
			Print("SCR_MapEntity: Attempt to call SetPan before map init is completed", LogLevel.NORMAL);
			return;
		}
			
		bool adjustedPan = false;
		
		// test bounds
		if (!FitPanBounds(x, y, center))
			adjustedPan = true;
		
		// save current pan
		m_fPanX = x;
		m_fPanY = y;
		
		PosChange(m_Workspace.DPIScale(m_fPanX), m_Workspace.DPIScale(m_fPanY));
		
		if (isPanEnd)
			SCR_MapCursorInfo.startPos = {0, 0};
		
		s_OnMapPan.Invoke(m_fPanX, m_fPanY, adjustedPan);
	}
		
	//------------------------------------------------------------------------------------------------
	//! Pan the map by px amount
	//! \param panMode is mode of panning
	//! \param panValue is amount of pixels to pan
	void Pan(EMapPanMode panMode, float panValue = 0)
	{
		if (m_iDelayCounter > 0)
		{
			Print("SCR_MapEntity: Attempt to call Pan before map init is completed", LogLevel.NORMAL);
			return;
		}
		
		float panX, panY;
		
		// panning mode				
		if (panMode == EMapPanMode.DRAG)
		{			
			// begin drag
			if (SCR_MapCursorInfo.startPos[0] == 0 && SCR_MapCursorInfo.startPos[1] == 0)
				SCR_MapCursorInfo.startPos = {SCR_MapCursorInfo.x, SCR_MapCursorInfo.y};
			
			// mouse position difference
			int diffX = SCR_MapCursorInfo.x - SCR_MapCursorInfo.startPos[0]; 
			int diffY = SCR_MapCursorInfo.y - SCR_MapCursorInfo.startPos[1];
			
			panX = m_fPanX + diffX;
			panY = m_fPanY + diffY;
			
			SCR_MapCursorInfo.startPos[0] = SCR_MapCursorInfo.x;
			SCR_MapCursorInfo.startPos[1] = SCR_MapCursorInfo.y;
		}
		else if (panMode == EMapPanMode.HORIZONTAL)
		{
			panX = m_fPanX + panValue;
			panY = m_fPanY;
		}
		else if (panMode == EMapPanMode.VERTICAL)
		{
			panX = m_fPanX;
			panY = m_fPanY + panValue;
		}
		
		// Pan
		SetPan(panX, panY, false, false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Interpolated pan
	//! \param panX is x target screen coordinate in DPIScaled px
	//! \param panY is y target screen coordinate in DPIScaled px
	//! \param panTime is interpolation duration
	void PanSmooth(float panX, float panY, float panTime = 0.25)
	{		
		if (m_iDelayCounter > 0)
		{
			Print("SCR_MapEntity: Attempt to call PanSmooth before map init is completed", LogLevel.NORMAL);
			return;
		}
		
		float screenWidth, screenHeight;
		m_MapWidget.GetScreenSize(screenWidth, screenHeight);
		
		if (panTime <= 0)
			panTime = 0.1;
		
		// un-center to get direct pan pos
		m_aStartPan = { m_Workspace.DPIUnscale(screenWidth/2) - m_fPanX, m_Workspace.DPIUnscale(screenHeight/2) - m_fPanY };
		m_aTargetPan = { m_Workspace.DPIUnscale(panX), m_Workspace.DPIUnscale(panY) };
		m_fPanTimeModif = 1/panTime;
		m_fPanSlice = 1.0;
		m_bIsPanInterp = true;
	}
				
	//------------------------------------------------------------------------------------------------
	//! Triggers OnSelection event
	//! \param selectionPos the selection (click) position in scaled screen coordinates
	void InvokeOnSelect(vector selectionPos)
	{
		s_OnSelection.Invoke(selectionPos);
	}
	
	//! Select target MapItem
	//! \param MapItem is the target 
	void SelectItem(MapItem item)
	{
		item.Select(true);
		item.SetHighlighted(true);
		

		s_OnSelectionChanged.Invoke(item);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set hover mode to target MapItem
	//! \param MapItem is the target 
	void HoverItem(MapItem item)
	{
		item.SetHovering(true);
		m_HoveredMapItem = item;
		
		s_OnHoverItem.Invoke(item);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Clear selected items
	void ClearSelection()
	{
		ResetSelected();
		ResetHighlighted();
		
		s_OnSelectionChanged.Invoke(null);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Clear hover state
	void ClearHover()
	{
		ResetHovering();

		s_OnHoverEnd.Invoke(m_HoveredMapItem);
		m_HoveredMapItem = null;
	}
	
	//------------------------------------------------------------------------------------------------
	// CONVERSION METHODS
	//------------------------------------------------------------------------------------------------
	//! Use canvas world coords to get DPIscaled screen coords, flips the y-axis
	// \param worldX is world x
	// \param worldY is world y
	// \param screenPosX is screen x
	// \param screenPosY is screen y
	// \param withPan determines whether current pan is added to the result
	// \param targetPPU determines whether the calculation uses current PixelPerUnit (when 0) or one supplied to it (fur future zoom calculation)
	void WorldToScreen(float worldX, float worldY, out int screenPosX, out int screenPosY, bool withPan = false, float targetPPU = 0)
	{
		vector mapSizeUnits = m_MapWidget.GetSizeInUnits();
		float pixPerUnit = m_fZoomPPU;
		
		if (targetPPU > 0)
			pixPerUnit = targetPPU;
		
		worldY = mapSizeUnits[1] - worldY; // fix Y axis which is reversed between screen and world
	
		if (withPan)
		{
			screenPosX = (worldX * pixPerUnit) + m_Workspace.DPIScale(m_fPanX);
			screenPosY = (worldY * pixPerUnit) + m_Workspace.DPIScale(m_fPanY);
		}
		else
		{
			screenPosX = worldX * pixPerUnit;
			screenPosY = worldY * pixPerUnit;		
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Use scaled screen coords to get canvas world coords, flips the y-axis
	// \param screenPosX is screen x
	// \param screenPosY is screen y
	// \param worldX is world x
	// \param worldY is world y
	void ScreenToWorld(int screenPosX, int screenPosY, out float worldX, out float worldY)
	{
		vector mapSizeUnits = m_MapWidget.GetSizeInUnits();
				
		worldX = (screenPosX - m_Workspace.DPIScale(m_fPanX)) / m_fZoomPPU;
		worldY = (screenPosY - m_Workspace.DPIScale(m_fPanY)) / m_fZoomPPU;
		worldY =  mapSizeUnits[1] - worldY;	// fix Y axis which is reversed between screen and world
	}
	
	//------------------------------------------------------------------------------------------------
	//! Use scaled screen coords to get canvas world coords without flipping the y-axis
	// \param screenPosX is screen x
	// \param screenPosY is screen y
	// \param worldX is world x
	// \param worldY is world y
	void ScreenToWorldNoFlip(int screenPosX, int screenPosY, out float worldX, out float worldY)
	{		
		worldX = (screenPosX - m_Workspace.DPIScale(m_fPanX)) / m_fZoomPPU;
		worldY = (screenPosY - m_Workspace.DPIScale(m_fPanY)) / m_fZoomPPU;
	}
		
	/*!
	Get grid coordinates for given position.
	\param pos World position
	\param resMin Minimum grid resolution as 10^x, e.g., 2 = 100, 3 = 1000, etc.
	\param resMax Maximum grid resolution as 10^x
	\param delimiter String added between horizontal and vertical coordinate
	\return Grid coordinates
	*/
	//--- ToDo: Use grid sizes configured in layers in case someone will define non-metric grid
	static string GetGridPos(vector pos, int resMin = 2, int resMax = 4, string delimiter = " ")
	{
		//--- Convert to int so we can use native mod operator %
		int posX = pos[0];
		int posZ = pos[2];
		
		string gridX, gridZ; //--- ToDo: Use [out] params instead?
		
		for (int i = resMax; i >= resMin; i--)
		{
			int mod = Math.Pow(10, i);
			int modX = posX - posX % mod;
			int modZ = posZ - posZ % mod;
			gridX += (modX / mod).ToString();
			gridZ += (modZ / mod).ToString();
			posX -= modX;
			posZ -= modZ;
		}
		
		return gridX + delimiter + gridZ;
	}
	
	//------------------------------------------------------------------------------------------------
	// SUPPORT METHODS
	//------------------------------------------------------------------------------------------------
	//! Adjust provided pan values by pan bounds conditions
	//! \param panX is UNSCALED pan offset x in px
	//! \param panY is UNSCALED pan offset y in px
	//! \return false if pan had to be adjusted to fit the pan rules
	protected bool FitPanBounds(inout float panX, inout float panY, bool center)
	{
		float windowWidth, windowHeight;
		m_MapWidget.GetScreenSize(windowWidth, windowHeight);
		
		windowWidth = m_Workspace.DPIUnscale(windowWidth);
		windowHeight = m_Workspace.DPIUnscale(windowHeight);
		
		// center to coords
		if (center)
		{			
			panX = windowWidth/2 - panX;
			panY = windowHeight/2 - panY;
		}
		
		// map size in px
		vector size = m_MapWidget.GetSizeInUnits();
		float pixPerUnit = m_fZoomPPU;	
			
		int width 	= size[0] * pixPerUnit;
		int height 	= size[1] * pixPerUnit;
		
		// center of the screen can travel everywhere within map
		int minCoordX = windowWidth/2 - m_Workspace.DPIUnscale(width);
		int minCoordY = windowHeight/2 - m_Workspace.DPIUnscale(height);
		int maxCoordX = windowWidth/2;
		int maxCoordY = windowHeight/2;
		
		// cannot pan outside of map bounds 
		/*int minCoordX = windowWidth - width;
		int minCoordY = windowHeight - height;
		int maxCoordX = 0;
		int maxCoordY = 0;*/
		
		bool adjusted = false;
		
		// stop when over min/max
		if (panX < minCoordX)
		{
			panX = minCoordX;
			adjusted = true;
		}
		
		if (panX > maxCoordX)
		{ 
			panX = maxCoordX;
			adjusted = true;
		}
		
		if (panY < minCoordY) 
		{ 
			panY = minCoordY;
			adjusted = true;
		}
		
		if (panY > maxCoordY)
		{
			panY = maxCoordY;
			adjusted = true;
		}
		
		if (adjusted)
			return false;
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Calculate zoom min/max 
	//! \return true if successful
	bool UpdateZoomBounds()
	{
		float screenWidth, screenHeight;
		m_MapWidget.GetScreenSize(screenWidth, screenHeight);

		vector size = m_MapWidget.GetSizeInUnits();	
		float maxUnitsPerScreen = screenHeight / m_MapWidget.PixelPerUnit();	
		m_fMinZoom = (maxUnitsPerScreen / size[1]) * m_MapWidget.PixelPerUnit();
		m_fMaxZoom = SCR_MapConstants.MAX_PIX_PER_METER;
		
		return true;
	}
		
	//------------------------------------------------------------------------------------------------
	//! Checks whether layer should change based on current zoom
	protected void AssignViewLayer()
	{
		int count = LayerCount();
		// No layers to change to
		if (count == 0)
			return;
		
		for ( int layer = 0; layer < count; layer++ )
		{
			if ( GetCurrentZoom() >= GetLayer(layer).GetCeiling() )
			{
				ChangeLayer(layer);
				break;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set new layer
	protected void ChangeLayer(int newIndex)
	{
		// if not the current layer
		if (newIndex != m_iLayerIndex)
		{
			m_iLayerIndex = newIndex;
			SetLayer(m_iLayerIndex);
			
			s_OnLayerChanged.Invoke(m_iLayerIndex);
		}
	}
		
	//------------------------------------------------------------------------------------------------
	// INIT / CLEANUP METHODS
	//------------------------------------------------------------------------------------------------
	//! Initialize layers from config
	//! \param layerCount is total count of layers
	//! \param layerConfigs is array of individual layer configs
	protected void InitLayers(int layerCount, array<ref LayerConfiguration> layerConfigs)
	{		
		// set count
		InitializeLayers(layerCount);
		
		if (layerConfigs.Count() < 1)
			return;
		
		
		// per layer configuration
		for (int i = 0; i < layerCount; i++)
		{
			MapLayer layer = GetLayer(i);
			if (layer && layerConfigs[i].DescriptorConfigs.Count() > 0)
			{
				// setr ceiling vals
				layerConfigs[i].LayerProps.SetLayerProps(layer);
									
				// props configs
				foreach ( SCR_MapPropsConfig propsCfg : m_ActiveMapCfg.MapPropsConfigs)
				{
					propsCfg.SetDefaults(layer);
				}
				
				// descriptor configuration init
				InitDescriptors(i, layer, layerConfigs[i]);
	
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Init visibility and default config values for descriptors based on type
	//! \param layerID is id of the layer
	//! \param layer is the map layer
	//! \param layerConfig is the config for the layer
	protected void InitDescriptors(int layerID, MapLayer layer, LayerConfiguration layerConfig)
	{
		// create descriptor visiiblity map
		map<int, int> descriptorVisibility = new map<int, int>;
		
		// descriptor visibility configuration
		foreach (MapDescriptorConfiguration descConfig : layerConfig.DescriptorConfigs)
		{
			descriptorVisibility.Set(descConfig.Type, layerID); // match descriptor type with visibility
		}
		
		if (!m_DefaultsCfg)
			return;
		
		foreach (SCR_DescriptorDefaultsBase descriptorType : m_DefaultsCfg.m_aDescriptorDefaults)
		{
			// visible in the current layer
			if ( descriptorVisibility.Get(descriptorType.m_iDescriptorType) >= layerID )
			{	
				MapDescriptorProps props;
				
				// is using faction based configuration
				if (descriptorType.m_bUseFactionColors)
				{
					foreach (SCR_FactionColorDefaults factionDefaults : m_DefaultsCfg.m_aFactionColors)
					{
						props = layer.GetPropsFor(factionDefaults.m_iFaction, descriptorType.m_iDescriptorType);
						if (props)
						{
							descriptorType.SetDefaults(props);
							factionDefaults.SetColors(props);
						}
					}
				}
				else
				{
					props = layer.GetPropsFor(EFactionMapID.UNKNOWN, descriptorType.m_iDescriptorType);
					if (props)
					{
						descriptorType.SetDefaults(props);
						descriptorType.SetColors(props);
					}
				}
			}
			// not visible in the current layer
			else 
			{
				MapDescriptorProps props;
				
				// is using faction based configuration
				if (descriptorType.m_bUseFactionColors)
				{
					foreach (SCR_FactionColorDefaults factionDefaults : m_DefaultsCfg.m_aFactionColors)
					{
						props = layer.GetPropsFor(factionDefaults.m_iFaction, descriptorType.m_iDescriptorType);
						if (props)
							props.SetVisible(false);
					}
				}
				else
				{
					props = layer.GetPropsFor(EFactionMapID.UNKNOWN, descriptorType.m_iDescriptorType);
					if (props)
						props.SetVisible(false);
				}
			}
		}
	}
			
	//------------------------------------------------------------------------------------------------
	//! Activate modules 
	//! \param modules is an array of modules to activate
	//------------------------------------------------------------------------------------------------
	protected void ActivateModules( array<ref SCR_MapModuleBase> modules )
	{
		if (modules.IsEmpty())
			return;
				
		array<SCR_MapModuleBase> modulesToInit = new array<SCR_MapModuleBase>();
		
		foreach ( SCR_MapModuleBase module : modules ) 
		{
			// load new module
			if (m_bDoReload)
			{
				m_aLoadedModules.Insert(module);
				modulesToInit.Insert(module);
				m_aActiveModules.Insert(module);
				module.SetActive(true);
			}
			// activate modules
			else 
			{
				int count = m_aLoadedModules.Count();
				for (int i = 0; i < count; i++)
				{
					if (!m_aLoadedModules[i].IsInherited(module.Type()))
						continue;
					
					// is module active
					if (m_aActiveModules.Find(m_aLoadedModules[i]) == -1)
					{
						m_aActiveModules.Insert(m_aLoadedModules[i]);
						m_aLoadedModules[i].SetActive(true);
						break;
					}	
				}
			}
		}
		
		foreach ( SCR_MapModuleBase module : modulesToInit )
		{
			module.Init();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Activate UI components 
	//! \param modules is an array of module typenames
	protected void ActivateComponents( array<ref SCR_MapUIBaseComponent> components )
	{		
		if (components.IsEmpty())
			return;
				
		array<SCR_MapUIBaseComponent> componentsToInit = new array<SCR_MapUIBaseComponent>();
		
		foreach ( SCR_MapUIBaseComponent component : components ) 
		{
			// load new component
			if (m_bDoReload)
			{
				m_aLoadedComponents.Insert(component);
				componentsToInit.Insert(component);
				m_aActiveComponents.Insert(component);
				component.SetActive(true);
			}
			// activate components
			else
			{
				int count = m_aLoadedComponents.Count();
				for (int i = 0; i < count; i++)
				{
					if (!m_aLoadedComponents[i].IsInherited(component.Type()))
						continue;
									
					// is component active
					if (m_aActiveComponents.Find(m_aLoadedComponents[i]) == -1)
					{
						m_aActiveComponents.Insert(m_aLoadedComponents[i]);
						m_aLoadedComponents[i].SetActive(true);
						break;
					}	
				}
			}
		}
		
		foreach ( SCR_MapUIBaseComponent component : componentsToInit )
		{
			component.Init();
		}
		
	}
	
	//------------------------------------------------------------------------------------------------
	//! Activate other components
	//! \param modules is an array of component names
	protected void ActivateOtherComponents(EMapOtherComponents componentFlags)
	{
		if (componentFlags & EMapOtherComponents.LEGEND_SCALE)
			EnableLegend(true);
		else 
			EnableLegend(false);
		
		if (componentFlags & EMapOtherComponents.GRID)
			EnableGrid(true);
		else 
			EnableGrid(false);

	}
	
	//------------------------------------------------------------------------------------------------
	//! Deactivate module 
	void DeactivateModule(SCR_MapModuleBase module)
	{
		m_aActiveModules.RemoveItem(module);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Deactivate UI component 
	void DeactivateComponent(SCR_MapUIBaseComponent component)
	{
		m_aActiveComponents.RemoveItem(component);
	}	
	
	//------------------------------------------------------------------------------------------------
	//! Map close cleanup
	protected void Cleanup()
	{
		// deactivate components & modules
		foreach (SCR_MapUIBaseComponent component : m_aActiveComponents )
		{
			component.SetActive(false, true);
		}
		m_aActiveComponents.Clear();
		
		foreach (SCR_MapModuleBase module : m_aActiveModules )
		{
			module.SetActive(false, true);
		}
		m_aActiveModules.Clear();
				
		m_bIsDebugMode = false;
	}
	
	//------------------------------------------------------------------------------------------------
	// UPDATE METHODS
	//------------------------------------------------------------------------------------------------
	//! Updates view port
	void UpdateViewPort()
	{
		float screenWidth, screenHeight;
		m_MapWidget.GetScreenSize(screenWidth, screenHeight);
			
		float minCoordX, minCoordY, maxCoordX, maxCoordY;
			
		ScreenToWorld(screenWidth, 0, maxCoordX, maxCoordY);
		ScreenToWorld(0, screenHeight, minCoordX, minCoordY);
			
		SetFrame(Vector(minCoordX, 0, minCoordY), Vector(maxCoordX, 0, maxCoordY));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Interpolated pan update
	//! \param timeSlice is frame timeSlice
	protected void PanUpdate(float timeSlice)
	{
		m_fPanSlice -= timeSlice * m_fPanTimeModif;
		
		// End interpolation
		if (m_fPanSlice <= 0)
		{
			m_bIsPanInterp = false;
			SetPan(m_aTargetPan[0], m_aTargetPan[1]);
			s_OnMapPanEnd.Invoke(m_aTargetPan[0],  m_aTargetPan[1]);
		}
		else
		{
			float panX = Math.Lerp(m_aStartPan[0], m_aTargetPan[0], 1 - m_fPanSlice);
			float panY = Math.Lerp(m_aStartPan[1], m_aTargetPan[1], 1 - m_fPanSlice);
			SetPan(panX, panY, false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Interpolated zoom update
	//! \param timeSlice is frame timeSlice
	protected void ZoomUpdate(float timeSlice)
	{
		m_fZoomSlice -= timeSlice * m_fZoomTimeModif;
		
		// End interpolation
		if (m_fZoomSlice <= 0)
		{
			SetZoom(m_fTargetPPU);
			s_OnMapZoomEnd.Invoke(m_fTargetPPU);
			m_bIsZoomInterp = false;
		}
		else
		{
			float zoom = Math.Lerp(m_fStartPPU, m_fTargetPPU, 1 - m_fZoomSlice);
			SetZoom(zoom);
		}
	}
			
	//------------------------------------------------------------------------------------------------
	//! Update map debug table
	protected void UpdateDebug()
	{		
		float wX, wY;
		float x = m_Workspace.DPIScale(SCR_MapCursorInfo.x);
		float y = m_Workspace.DPIScale(SCR_MapCursorInfo.y);
		ScreenToWorld(x, y, wX, wY);
		
		vector pan = GetCurrentPan();
		array<MapItem> outItems = {};
		GetSelected(outItems);
		
		DbgUI.Begin("Map debug");
		string dbg1 = "CURSOR SCREEN POS: x: %1 y: %2";
		DbgUI.Text( string.Format( dbg1, x, y ) );
		string dbg2 = "CURSOR WORLD POS: x: %1 y: %2";
		DbgUI.Text( string.Format( dbg2, wX, wY ) );
		string dbg3 = "PAN OFFSET: x: %1 y: %2 ";
		DbgUI.Text( string.Format( dbg3, pan[0], pan[1] ) );
		string dbg4 = "ZOOM: min: %1 max: %2 | pixPerUnit: %3";
		DbgUI.Text( string.Format( dbg4, GetMinZoom(), GetMaxZoom(), GetCurrentZoom() ) );
		string dbg5 = "LAYER: current: %1 | pixPerUnit ceiling: %2";
		DbgUI.Text( string.Format( dbg5, m_iLayerIndex, GetLayer(m_iLayerIndex).GetCeiling() ) );
		string dbg6 = "MODULES: loaded: %1 | active: %2 | list: %3 ";
		DbgUI.Text( string.Format( dbg6, m_aLoadedModules.Count(), m_aActiveModules.Count(), m_aActiveModules ) );
		string dbg7 = "COMPONENTS: loaded: %1 | active: %2 | list: %3 ";
		DbgUI.Text( string.Format( dbg7, m_aLoadedComponents.Count(), m_aActiveComponents.Count(), m_aActiveComponents ) );
		string dbg8 = "MAPITEMS: selected: %1 | hovered: %2 ";
		DbgUI.Text( string.Format( dbg8, outItems, m_HoveredMapItem ) );
		DbgUI.End();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle update of modules, components and pan/zoom interpolation
	//! \param timeSlice is frame timeSlice 
	protected void UpdateMap(float timeSlice)
	{						
		// update modules
		foreach ( SCR_MapModuleBase module : m_aActiveModules)
		{
			module.Update(timeSlice);
		}
		
		//update components
		foreach ( SCR_MapUIBaseComponent component : m_aActiveComponents)
		{
			component.Update(timeSlice);
		}
		
		// interpolation update
		if (m_bIsZoomInterp)
			ZoomUpdate(timeSlice);
		
		if (m_bIsPanInterp)
			PanUpdate(timeSlice);
	}
	
#ifndef DISABLE_GADGETS		
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (IsOpen())
		{
			// delayed init -> This is here so the MapWidget has time to initiate its size & PixelPerUnit calculation
			if (m_iDelayCounter >= 0)
			{
				if (m_iDelayCounter == 0)
				{
					m_iDelayCounter--;
					OnMapOpen(m_ActiveMapCfg);
				}
				else 
				{
					m_iDelayCounter--;
					return;
				}
			}
						
			UpdateMap(timeSlice);
			
			if (m_bIsDebugMode)
				UpdateDebug();
						
			UpdateViewPort();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		// Save size
		m_iMapSize[0] = Size()[0];
		m_iMapSize[1] = Size()[2];
		
		if (m_iMapSize[0] == 0 || m_iMapSize[1] == 0)
		{
			Print("SCR_MapEntity: Cannot get the size from terrain. Using default.", LogLevel.WARNING);
			m_iMapSize[0] = 1024;
			m_iMapSize[1] = 1024;
		}
	}

	//------------------------------------------------------------------------------------------------
	void SCR_MapEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT | EntityEvent.FRAME);

		s_MapInstance = this;
		
		for(int ii = 0; ii < EMapDescriptorType.MDT_COUNT; ++ii)
		{
			imagesetIndices.Insert(-1);
		}
		
		DiagMenu.RegisterBool(SCR_DebugMenuID.DEBUGUI_UI_MAP_DEBUG_OPTIONS, "", "Enable map debug menu", "UI");
		
		GetGame().OnUserSettingsChangedInvoker().Insert(OnUserSettingsChanged);
		GetGame().OnWindowResizeInvoker().Insert(OnWindowResized);
	}

	//------------------------------------------------------------------------------------------------
	void ~SCR_MapEntity()
	{
		if (m_bIsOpen)
			CloseMap();
		
		s_OnMapInit.Clear();
		s_OnMapOpen.Clear();
		s_OnMapClose.Clear();
		s_OnMapPan.Clear();
		s_OnMapPanEnd.Clear();
		s_OnMapZoom.Clear();
		s_OnMapZoomEnd.Clear();
		s_OnSelectionChanged.Clear();
		s_OnSelection.Clear();
		s_OnHoverItem.Clear();
		s_OnHoverEnd.Clear();
		s_OnLayerChanged.Clear();
		
		DiagMenu.Unregister(SCR_DebugMenuID.DEBUGUI_UI_MAP_DEBUG_OPTIONS);
		
		s_MapInstance = null;
	}

#endif	
};