//------------------------------------------------------------------------------------------------
class SCR_ToolMenuButtonComponent : SCR_ButtonImageComponent
{
	TextWidget m_wTextLimit;
	
	//------------------------------------------------------------------------------------------------
	void SetText(string text)
	{
		m_wTextLimit.SetText(text);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetTextVisible(bool state)
	{
		m_wTextLimit.SetVisible(state);
	}
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wTextLimit = TextWidget.Cast(w.FindAnyWidget("TextLimit"));
	}
};
