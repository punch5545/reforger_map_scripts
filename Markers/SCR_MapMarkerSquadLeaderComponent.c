//------------------------------------------------------------------------------------------------
class SCR_MapMarkerSquadLeaderComponent : SCR_MapMarkerDynamicWComponent
{
	bool m_bIsInit;
	Widget m_wMarkerVLayout;
	Widget m_wGroupInfo;
	Widget m_wGroupInfoList;
	TextWidget m_wGroupFrequency;
	
	ref array<Widget> m_aGroupMemberEntries = new array<Widget>;
	
	protected const ResourceName LINE_LAYOUT = "{B09864CA15145CD3}UI/layouts/Map/MapMarkerGroupInfoLine.layout";
	protected const string LINE_TEXT = "lineText";
	protected const string LINE_ICON = "lineIcon";
		
	//------------------------------------------------------------------------------------------------
	//! Differentiates visuals between our group and the others
	void SetGroupActive(bool state)
	{
		if (state)
			m_wMarkerVLayout.SetOpacity(1);
		else 
			m_wMarkerVLayout.SetOpacity(0.75);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 0)	// LMB only
			return true;
		
		GetGame().OpenGroupMenu();
		
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_wMarkerText.SetColor(GUIColors.ORANGE);
		
		SCR_AIGroup group = SCR_MapMarkerSquadLeader.Cast(m_MarkerEnt).GetGroup();
		if (group)
		{
			float fFrequency = Math.Round(group.GetRadioFrequency() * 0.1) * 0.01; 	// Format the frequency text: round and convert to 2 digits with one possible decimal place (39500 -> 39.5)			
			m_wGroupFrequency.SetText(fFrequency.ToString(3, 1));
				
			if (!m_bIsInit)
			{
				int capacity = group.GetMaxMembers();
				
				for (int i = 0; i < capacity; i++)
				{
					m_aGroupMemberEntries.Insert(GetGame().GetWorkspace().CreateWidgets(LINE_LAYOUT, m_wGroupInfoList));
				}
				
				m_bIsInit = true;
			}
			
			int playerCount = group.GetPlayerCount();
			array<int> memberIDs = group.GetPlayerIDs();
			PlayerManager pManager = GetGame().GetPlayerManager();
			int leaderID = group.GetLeaderID();
			
			foreach (int i, Widget entry :  m_aGroupMemberEntries) 
			{
				if (i < playerCount)
				{
					TextWidget txtW = TextWidget.Cast(entry.FindWidget(LINE_TEXT));
					txtW.SetText(pManager.GetPlayerName(memberIDs[i]));
					entry.SetVisible(true);
					
					if (GetGame().GetPlayerController().GetPlayerId() == memberIDs[i])
						txtW.SetColor(GUIColors.ORANGE);
					else 
						txtW.SetColor(GUIColors.DEFAULT);
					
					ImageWidget imgW = ImageWidget.Cast(entry.FindWidget(LINE_ICON));
					if (leaderID == memberIDs[i])
						imgW.SetOpacity(1);
					else 
						imgW.SetOpacity(0);
					
					continue;
				} 
				
				entry.SetVisible(false);
			}
			
			m_wGroupInfo.SetVisible(true);
		}
		
		return true;
	}
	
	
	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_wMarkerText.SetColor(m_TextColor);
		m_wGroupInfo.SetVisible(false);
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wMarkerVLayout = m_wRoot.FindWidget("markerVLayout");
		m_wGroupInfo = m_wRoot.FindAnyWidget("groupInfo");
		m_wGroupInfoList = m_wGroupInfo.FindAnyWidget("groupInfoList");
		m_wGroupFrequency = TextWidget.Cast(m_wGroupInfo.FindAnyWidget("groupFrequency"));
	}
}