#pragma once

namespace ImGui
{
	void ExtendWindowPastBorder();

	void CenteredText(const char* label, bool vertical);
	void TextColoredWrapped(const ImVec4& col, const char* fmt, ...);

	bool ToggleButton(const char* label, bool* v);

	void Spacing(std::uint32_t a_numSpaces);

	ImVec2 GetNativeViewportSize();
	ImVec2 GetNativeViewportPos();
	ImVec2 GetNativeViewportCenter();
}
