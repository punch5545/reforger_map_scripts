//------------------------------------------------------------------------------------------------
//! Map marker layout component
//! Attached to root of marker base layout
class SCR_MapMarkerWidgetComponent : SCR_ScriptedWidgetComponent
{
	protected bool m_bIsEventListening;	// whether this marker reacts to events
	protected bool m_bIsSymbolMode;
	
	protected ImageWidget m_wMarkerIcon;
	protected TextWidget m_wMarkerText;
	protected TextWidget m_wMarkerAuthor;
	protected Widget m_wSymbolRoot;
	protected Widget m_wSymbolOverlay;
	
	protected ref Color m_TextColor = new Color(0.0, 0.0, 0.0, 1.0);
	protected ref Color m_CurrentImageColor = new Color(0.0, 0.0, 0.0, 1.0);
	protected SCR_MapMarkerBase m_MarkerObject;
	
	//------------------------------------------------------------------------------------------------
	void SetMarkerObject(notnull SCR_MapMarkerBase marker)
	{
		m_MarkerObject = marker;
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
	//! Set visual mode for military symbol which is constructed through additional component
	void SetMilitarySymbolMode(bool state)
	{
		m_bIsSymbolMode = state;
		
		m_wSymbolRoot.SetEnabled(state);
		m_wSymbolRoot.SetVisible(state);
		
		m_wMarkerIcon.SetVisible(!state);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetEventListening(bool state)
	{
		m_bIsEventListening = state;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateMilitarySymbol(SCR_MilitarySymbol milSymbol)
	{
		SCR_MilitarySymbolUIComponent symbolComp = SCR_MilitarySymbolUIComponent.Cast(m_wSymbolOverlay.FindHandler(SCR_MilitarySymbolUIComponent));
		if (symbolComp)
			symbolComp.Update(milSymbol);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetText(string text)
	{
		m_wMarkerText.SetText(text);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetAuthor(string text)
	{
		m_wMarkerAuthor.SetText(text);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetColor(Color color)
	{
		m_CurrentImageColor = color;
		
		if (m_bIsSymbolMode)
			m_wSymbolOverlay.SetColor(color);
		else 
			m_wMarkerIcon.SetColor(color);
			
		m_wMarkerText.SetColor(m_TextColor);
		m_wMarkerAuthor.SetColor(m_TextColor);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (!m_bIsEventListening || !m_MarkerObject || !SCR_MapMarkersUI.IsOwnedMarker(m_MarkerObject))
			return false;
		
		m_wMarkerIcon.SetColor(GUIColors.ORANGE);
		m_wMarkerText.SetColor(GUIColors.ORANGE);
		m_wMarkerAuthor.SetColor(GUIColors.ORANGE);
		
		return true;
	}
	
	
	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!m_bIsEventListening || !m_MarkerObject || !SCR_MapMarkersUI.IsOwnedMarker(m_MarkerObject))
			return false;
		
		m_wMarkerIcon.SetColor(m_CurrentImageColor);
		m_wMarkerText.SetColor(m_TextColor);
		m_wMarkerAuthor.SetColor(m_TextColor);
		
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wMarkerIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("MarkerIcon"));
		m_wMarkerText = TextWidget.Cast(m_wRoot.FindAnyWidget("MarkerText"));
		m_wMarkerAuthor = TextWidget.Cast(m_wRoot.FindAnyWidget("MarkerAuthor"));
		m_wSymbolRoot = m_wRoot.FindAnyWidget("SymbolWidget");
		m_wSymbolOverlay = m_wRoot.FindAnyWidget("SymbolOverlay");
	}
};