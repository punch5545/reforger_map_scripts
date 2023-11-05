[EntityEditorProps(category: "GameScripted/GameMode/Components", description: "Component for map config selection")]
class SCR_MapConfigComponentClass : ScriptComponentClass
{
};

//------------------------------------------------------------------------------------------------
//! Attached to BaseGameMode, used for map config selection
class SCR_MapConfigComponent : ScriptComponent
{
	[Attribute("{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", UIWidgets.ResourceNamePicker, desc: "Gadget map config", "conf class=SCR_MapConfig")]
	protected ResourceName m_sGadgetMapConfigPath;
	
	[Attribute("{418989FA279F1257}Configs/Map/MapSpawnMenu.conf", UIWidgets.ResourceNamePicker, desc: "Spawn screen map config", "conf class=SCR_MapConfig")]
	protected ResourceName m_sSpawnMapConfigPath;
	
	[Attribute("{19C76194B21EC3E1}Configs/Map/MapEditor.conf", UIWidgets.ResourceNamePicker, desc: "Game master map config", "conf class=SCR_MapConfig")]
	protected ResourceName m_sEditorMapConfigPath;
	
	//------------------------------------------------------------------------------------------------
	//! Get config of the ingame gadget map
	ResourceName GetGadgetMapConfig()
	{
		return m_sGadgetMapConfigPath;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get config of the respawn screen map
	ResourceName GetSpawnMapConfig()
	{
		return m_sSpawnMapConfigPath;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get config of the game master map
	ResourceName GetEditorMapConfig()
	{
		return m_sEditorMapConfigPath;
	}
};
