//------------------------------------------------------------------------------------------------
class SCR_MapTaskListUI : SCR_MapRTWBaseUI
{
	const string TASK_LIST_FRAME = "MapTaskList";
	const string ICON_NAME = "faction";

	protected Widget m_wUI;
	protected OverlayWidget m_wTaskListFrame;
	protected SCR_UITaskManagerComponent m_UITaskManager;
	protected SCR_MapToolMenuUI m_ToolMenu;

	[Attribute("JournalFrame", desc: "Journal frame widget name")]
	protected string m_sJournalFrameName;

	[Attribute("MapTaskList", desc: "Map task list root widget name")]
	protected string m_sMapTaskListRootName;
	
	[Attribute("MapTaskListFrame", desc: "Map task list root frame widget name")]
	protected string m_sMapTaskListRootFrameName;

	[Attribute("exclamationCircle", desc: "Journal Toolmenu imageset quad name")]
	protected string m_sJournalToolMenuIconName;

	//------------------------------------------------------------------------------------------------
	override protected void OnMapClose(MapConfiguration config)
	{
		if (m_UITaskManager)
		{
			m_UITaskManager.ClearWidget();
			m_UITaskManager.CreateTaskList();
		}
	}

	//------------------------------------------------------------------------------------------------
	override void Init()
	{
		m_UITaskManager = SCR_UITaskManagerComponent.GetInstance();
		if (!m_UITaskManager)
			return;

		m_ToolMenu = SCR_MapToolMenuUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapToolMenuUI));
		if (m_ToolMenu)
		{
			m_ToolMenuEntry = m_ToolMenu.RegisterToolMenuEntry(SCR_MapToolMenuUI.s_sToolMenuIcons, ICON_NAME, 2);
			m_ToolMenuEntry.m_OnClick.Insert(Action_TasksOpen);
		}

		//If there is a OverlayWidget like on the DeployMenu we use that instead of the default one
		m_wTaskListFrame = OverlayWidget.Cast(m_RootWidget.FindAnyWidget(TASK_LIST_FRAME));
		if (!m_wTaskListFrame)
			return;

		m_wUI = m_UITaskManager.CreateTaskList(m_wTaskListFrame);
	}

	//------------------------------------------------------------------------------------------------
	void Action_TasksOpen()
	{

		Widget taskListRoot = Widget.Cast(m_RootWidget.FindAnyWidget(m_sMapTaskListRootName));
		if (!taskListRoot)
			return;
		
		taskListRoot.SetVisible(true);
		
		Widget taskListRootFrame = Widget.Cast(m_RootWidget.FindAnyWidget(m_sMapTaskListRootFrameName));
		if (taskListRootFrame)
			taskListRootFrame.SetVisible(true);

		foreach (SCR_MapToolEntry toolEntry : m_ToolMenu.GetMenuEntries())
		{
			if (toolEntry.GetImageSet() != m_sJournalToolMenuIconName)
				continue;

			Widget mapJournalFrame = m_RootWidget.FindAnyWidget(m_sJournalFrameName);
			if (mapJournalFrame && mapJournalFrame.IsVisible())
			{
				mapJournalFrame.SetVisible(false);
				toolEntry.SetActive(false);
			}
		}

		if (!m_UITaskManager.IsTaskListOpen())
		{
			m_UITaskManager.ClearWidget();
			m_wUI = m_UITaskManager.CreateTaskList(m_wTaskListFrame);
			if (m_wUI)
			{
				m_UITaskManager.Action_ShowTasks(m_wUI);
				m_ToolMenuEntry.SetActive(true);
			}
		}
		else
		{
			m_UITaskManager.Action_TasksClose();
			m_ToolMenuEntry.SetActive(false);
		}
	}
}
