//------------------------------------------------------------------------------------------------
// Base for 2d map UI components
[BaseContainerProps()]
class SCR_MapUIBaseComponent : ScriptedWidgetComponent
{		
	protected bool m_bHookToRoot = false;							// determine whether this component is hooked to the root widget for use of ScriptedWidgetEventHandler events
	protected Widget m_RootWidget;									// map layout root widget	
	protected SCR_MapEntity m_MapEntity;	
						
	//------------------------------------------------------------------------------------------------
	// BASE METHODS
	//------------------------------------------------------------------------------------------------
	//! SCR_MapEntity event
	protected void OnMapOpen(MapConfiguration config)
	{	
		if (m_bHookToRoot)
			m_RootWidget.AddHandler(this);
	}
	
	//------------------------------------------------------------------------------------------------
	//! SCR_MapEntity event
	protected void OnMapClose(MapConfiguration config)
	{
		if (m_bHookToRoot)
			m_RootWidget.RemoveHandler(this);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Enable open/close events, called every time component is activated, usually on map open
	//! \param active is target state
	//! \param isCleanup determines if this is just deactivation or a cleanup
	void SetActive(bool active, bool isCleanup = false)
	{
		if (active)
		{
			m_RootWidget = m_MapEntity.GetMapConfig().RootWidgetRef; // Needs to be refreshed here
			
			m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
			m_MapEntity.GetOnMapClose().Insert(OnMapClose);
		}
		else 
		{
			m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
			m_MapEntity.GetOnMapClose().Remove(OnMapClose);
				
			if (!isCleanup)	
				m_MapEntity.DeactivateComponent(this);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Init method for cases where all modules and components should be loaded already so constructor cannot be used, called once after creation
	void Init()
	{}
	
	//------------------------------------------------------------------------------------------------
	//! Update method for frame operations
	void Update(float timeSlice)
	{}
	
	//------------------------------------------------------------------------------------------------
	void SCR_MapUIBaseComponent()
	{
		m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
};