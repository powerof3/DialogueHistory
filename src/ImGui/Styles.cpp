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
			return user.speakerName;
		case USER_STYLE::kSpeakerLine:
			return user.speakerLine;
		case USER_STYLE::kPlayerName:
			return user.playerName;
		case USER_STYLE::kPlayerLine:
			return user.playerLine;
		default:
			return ImVec4();
		}
	}

	float Styles::GetVar(USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kButtons:
			return user.buttonScale;
		case USER_STYLE::kDisabledTextAlpha:
			return user.textDisabledAlpha;
		case USER_STYLE::kSeparatorThickness:
			return user.separatorThickness;
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
#define GET_VALUE(a_value, a_section, a_key)                                                                                                        \
	bool a_value##_hex = false;                                                                                                                     \
	std::tie(user.a_value, a_value##_hex) = ToStyle<decltype(user.a_value)>(a_ini.GetValue(a_section, a_key, ToString(def.a_value, true).c_str())); \
	a_ini.SetValue(a_section, a_key, ToString(user.a_value, a_value##_hex).c_str());

		GET_VALUE(background, "Window", "rBackgroundColor");
		GET_VALUE(border, "Window", "rBorderColor");
		GET_VALUE(borderSize, "Window", "fBorderSize");

		GET_VALUE(text, "Text", "rColor");
		GET_VALUE(playerName, "Text", "rPlayerNameColor");
		GET_VALUE(playerLine, "Text", "rPlayerLineColor");
		GET_VALUE(speakerName, "Text", "rSpeakerNameColor");
		GET_VALUE(speakerLine, "Text", "rSpeakerLineColor");
		GET_VALUE(textDisabledAlpha, "Text", "rDisabledTextAlpha");

		GET_VALUE(header, "GlobalHistory", "rSelectedColor");
		GET_VALUE(headerHovered, "GlobalHistory", "rHoveredColor");
		GET_VALUE(frameBG, "GlobalHistory", "rSearchBoxColor");
		GET_VALUE(frameBorderSize, "GlobalHistory", "rSearchBoxBorderSize");
		GET_VALUE(button, "GlobalHistory", "rToggleColor");

		GET_VALUE(indentSpacing, "Widget", "fIndentSpacing");
		GET_VALUE(separator, "Widget", "rSeparatorColor");
		GET_VALUE(separatorThickness, "Widget", "fSeparatorThickness");

#undef GET_VALUE
	}

	void Styles::OnStyleRefresh()
	{
		if (!refreshStyle) {
			return;
		}

		LoadStyles();

		ImGuiStyle style;
		auto&      colors = style.Colors;

		style.WindowBorderSize = user.borderSize;
		style.ChildBorderSize = user.borderSize;
		style.FrameBorderSize = user.frameBorderSize;
		style.IndentSpacing = user.indentSpacing;

		colors[ImGuiCol_WindowBg] = user.background;
		colors[ImGuiCol_ChildBg] = user.background;
		colors[ImGuiCol_FrameBg] = user.frameBG;

		colors[ImGuiCol_Border] = user.border;
		colors[ImGuiCol_Separator] = user.separator;

		colors[ImGuiCol_Text] = user.text;

		colors[ImGuiCol_Header] = user.header;
		colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_Header];
		colors[ImGuiCol_HeaderHovered] = user.headerHovered;

		colors[ImGuiCol_Button] = user.button;

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
