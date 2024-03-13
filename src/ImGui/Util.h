#pragma once

namespace ImGui
{
	void ExtendWindowPastBorder();

	void CenteredText(const char* label, bool vertical);
	void TextColoredWrapped(const ImVec4& col, const char* fmt, ...);

	ImVec2 GetNativeViewportSize();
	ImVec2 GetNativeViewportPos();
	ImVec2 GetNativeViewportCenter();
}
