#include "Util.h"

#include "IconsFonts.h"
#include "Renderer.h"

namespace ImGui
{
	void ExtendWindowPastBorder()
	{
		const ImGuiWindow* window = GetCurrentWindowRead();
		const auto         borderSize = window->WindowBorderSize;

		if (borderSize == 0.0f) {
			return;
		}

		const ImGuiWindow* rootWindow = FindWindowByName("##Main");

		const auto newWindowPos = ImVec2{ window->Pos.x - borderSize, window->Pos.y - borderSize };
		rootWindow->DrawList->AddRect(newWindowPos, newWindowPos + ImVec2(window->Size.x + 2 * borderSize, window->Size.y + 2 * borderSize), GetColorU32(ImGuiCol_WindowBg), 0.0f, 0, borderSize);
	}

	// https://github.com/ocornut/imgui/discussions/3862
	void AlignForWidth(float width, float alignment)
	{
		float avail = GetContentRegionAvail().x;
		float off = (avail - width) * alignment;

		if (off > 0.0f) {
			SetCursorPosX(GetCursorPosX() + off);
		}
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

		ImGui::TextUnformatted(label);
	}

	void TextColoredWrapped(const ImVec4& col, const char* fmt, ...)
	{
		PushStyleColor(ImGuiCol_Text, col);
		PushTextWrapPos(0.0f);
		TextUnformatted(fmt);
		PopTextWrapPos();
		PopStyleColor();
	}

	// https://github.com/ocornut/imgui/issues/1537#issuecomment-355569554
	bool ToggleButton(const char* str_id, bool* v)
	{
		bool pressed = false;

		ImVec2      p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec4*     colors = ImGui::GetStyle().Colors;

		float height = ImGui::GetFrameHeight() / 1.5f;
		float width = height * 2.0f;
		float radius = height * 0.50f;

		ImGui::InvisibleButton(str_id, ImVec2(width, height));
		if (ImGui::IsItemClicked()) {
			*v = !*v;
			pressed = true;
		}

		float t = *v ? 1.0f : 0.0f;

		ImGuiContext& g = *GImGui;
		float         ANIM_SPEED = 0.05f;
		if (g.LastActiveId == g.CurrentWindow->GetID(str_id))  // && g.LastActiveIdTimer < ANIM_SPEED)
		{
			float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
			t = *v ? (t_anim) : (1.0f - t_anim);
		}

		ImU32 col_bg = GetColorU32(colors[ImGuiCol_Header]);
		draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
		draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, GetColorU32(colors[ImGuiCol_Button]));

		return pressed;
	}

	void Spacing(std::uint32_t a_numSpaces)
	{
		for (std::uint32_t i = 0; i < a_numSpaces; i++) {
			Spacing();
		}
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
