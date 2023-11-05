//------------------------------------------------------------------------------------------------
//! Attached to root of marker dynamic base layout
class SCR_MapMarkerDynamicWComponent : SCR_ScriptedWidgetComponent
{	
	protected ImageWidget m_wMarkerIcon;
	protected TextWidget m_wMarkerText;
	protected SCR_MapMarkerEntity m_MarkerEnt;
	
	protected ref Color m_TextColor = new Color(0.0, 0.0, 0.0, 1.0);
	
	//------------------------------------------------------------------------------------------------
	void SetMarkerEntity(notnull SCR_MapMarkerEntity marker)
	{
		m_MarkerEnt = marker;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Supports custom aspect ratio in case of non standard size imagesets
	void SetImage(ResourceName icon, string quad, float aspectRatio = 1)
	{
		m_wMarkerIcon.LoadImageFromSet(0, icon, quad);
		if (aspectRatio != 1 || aspectRatio != 0)
		{
			vector size = m_wMarkerIcon.GetSize();
			m_wMarkerIcon.SetSize(size[0] * 0.9, (size[1] * (1/aspectRatio)) * 0.9); // todo, temp size adjust before symbols group side are fixed
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SetText(string text)
	{
		m_wMarkerText.SetText(text);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetColor(Color color)
	{
		m_wMarkerIcon.SetColor(color);
		m_wMarkerText.SetColor(m_TextColor);
	}
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wMarkerIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("MarkerIcon"));
		m_wMarkerText = TextWidget.Cast(m_wRoot.FindAnyWidget("MarkerText"));
	}

}