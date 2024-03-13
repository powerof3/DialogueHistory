#include "Util.h"

#include "Renderer.h"

namespace ImGui
{
	void ExtendWindowPastBorder()
	{
		const ImGuiWindow* window = GetCurrentWindowRead();
		const ImGuiWindow* rootWindow = FindWindowByName("##Main");

		const auto borderSize = window->WindowBorderSize;
		const auto newWindowPos = ImVec2{ window->Pos.x - borderSize, window->Pos.y - borderSize };

		rootWindow->DrawList->AddRect(newWindowPos, newWindowPos + ImVec2(window->Size.x + 2 * borderSize, window->Size.y + 2 * borderSize), GetColorU32(ImGuiCol_WindowBg), 0.0f, 0, borderSize);
	}

	void CenteredText(const char* label, bool vertical)
	{
		const auto windowSize = ImGui::GetWindowSize();
		const auto textSize = ImGui::CalcTextSize(label);

		if (vertical) {
			ImGui::SetCursorPosY((windowSize.y - textSize.y) * 0.5f);
		} else {
			ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
		}

		ImGui::Text(label);
	}

	void TextColoredWrapped(const ImVec4& col, const char* fmt, ...)
	{
		PushStyleColor(ImGuiCol_Text, col);
		TextWrapped(fmt);
		PopStyleColor();
	}

	ImVec2 GetNativeViewportPos()
	{
		return GetMainViewport()->Pos;
	}

	ImVec2 GetNativeViewportSize()
	{
		return GetMainViewport()->Size;
	}

	ImVec2 GetNativeViewportCenter()
	{
		const auto Size = GetNativeViewportSize();
		return { Size.x * 0.5f, Size.y * 0.5f };
	}
}
