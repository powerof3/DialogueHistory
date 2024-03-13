#include "Styles.h"

#include "IconsFonts.h"
#include "Renderer.h"
#include "Settings.h"

namespace ImGui
{
	ImVec4 Styles::GetColorVec4(USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kSpeakerName:
			return speakerName;
		case USER_STYLE::kSpeakerLine:
			return speakerLine;
		case USER_STYLE::kPlayerName:
			return playerName;
		case USER_STYLE::kPlayerLine:
			return playerLine;
		default:
			return ImVec4();
		}
	}

	float Styles::GetVar(USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kButtons:
			return buttonScale;
		case USER_STYLE::kDisabledTextAlpha:
			return textDisabledAlpha;
		case USER_STYLE::kSeparatorThickness:
			return separatorThickness;
		default:
			return 1.0f;
		}
	}

	void Styles::LoadStyles()
	{
		Settings::GetSingleton()->SerializeStyles([this](auto& ini) {
			LoadStyles(ini);
		});
	}

	void Styles::LoadStyles(CSimpleIniA& a_ini)
	{
		auto get_value = [&]<typename T>(T& a_value, const char* a_section, const char* a_key) {
			a_value = ToStyle<T>(a_ini.GetValue(a_section, a_key, ToString(a_value).c_str()));
			a_ini.SetValue(a_section, a_key, ToString(a_value).c_str());
		};

		get_value(buttonScale, "Icon", "fButtonScale");

		get_value(background, "Window", "rBackgroundColor");
		get_value(border, "Window", "rBorderColor");
		get_value(borderSize, "Window", "fBorderSize");

		get_value(text, "Text", "rColor");
		get_value(textDisabledAlpha, "Text", "rDisabledTextAlpha");
		get_value(playerName, "Text", "rPlayerNameColor");
		get_value(playerLine, "Text", "rPlayerLineColor");
		get_value(speakerName, "Text", "rSpeakerNameColor");
		get_value(speakerLine, "Text", "rSpeakerLineColor");

		get_value(header, "GlobalHistory", "rSelectedColor");
		get_value(headerHovered, "GlobalHistory", "rHoveredColor");

		get_value(indentSpacing, "Widget", "fIndentSpacing");
		get_value(separator, "Widget", "rSeparatorColor");
		get_value(separatorThickness, "Widget", "fSeparatorThickness");
	}

	void Styles::OnStyleRefresh()
	{
		if (!refreshStyle) {
			return;
		}

		LoadStyles();

		ImGuiStyle style;
		auto&      colors = style.Colors;

		style.WindowBorderSize = borderSize;
		style.FrameBorderSize = borderSize;
		style.IndentSpacing = indentSpacing;

		colors[ImGuiCol_WindowBg] = background;
		colors[ImGuiCol_ChildBg] = background;

		colors[ImGuiCol_Border] = border;
		colors[ImGuiCol_Separator] = separator;

		colors[ImGuiCol_Text] = text;

		colors[ImGuiCol_Header] = header;
		colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_Header];
		colors[ImGuiCol_HeaderHovered] = headerHovered;

		colors[ImGuiCol_NavHighlight] = ImVec4();

		style.ScaleAllSizes(DisplayTweaks::GetResolutionScale());

		GetStyle() = style;

		// reload fonts/icons

		Settings::GetSingleton()->SerializeFonts([&](auto& ini) { MANAGER(IconFont)->LoadSettings(ini); });

		MANAGER(IconFont)->ReloadFonts();
		MANAGER(IconFont)->ResizeIcons();

		refreshStyle = false;
	}

	void Styles::RefreshStyle()
	{
		refreshStyle = true;
	}

	ImVec4 GetUserStyleColorVec4(USER_STYLE a_style)
	{
		return Styles::GetSingleton()->GetColorVec4(a_style);
	}

	float GetUserStyleVar(USER_STYLE a_style)
	{
		return Styles::GetSingleton()->GetVar(a_style);
	}
}
