#pragma once

namespace ImGui
{
	void ExtendWindowPastBorder();

	void AlignForWidth(float width, float alignment = 0.5f);

	void CenteredText(const char* label, bool vertical);
	void TextColoredWrapped(const ImVec4& col, const char* fmt, ...);

	bool ToggleButton(const char* label, bool* v);

	bool IsItemSelected();

	void Spacing(std::uint32_t a_numSpaces);

	ImVec2 GetNativeViewportSize();
	ImVec2 GetNativeViewportPos();
	ImVec2 GetNativeViewportCenter();

	template <typename F>
	void AddBlurredShadow(const ImVec2& a_pos, F&& func)
	{
		const ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
		const ImU32 shadow_color = ImGui::GetColorU32(ImGuiCol_TextShadow);

		static const ImVec2 shadow_offset = ImGui::GetStyle().TextShadowOffset;

		const ImVec2 shadow_center(a_pos.x + shadow_offset.x, a_pos.y + shadow_offset.y);

		const std::int32_t sa = (shadow_color >> IM_COL32_A_SHIFT) & 0xFF;
		if (sa <= 0) {
			func(a_pos, text_color);
			return;
		}

		constexpr std::int32_t MAX_RADIUS = 16;

		struct Kernel
		{
			explicit Kernel(std::int32_t a_blurRadius)
			{
				const float sigma = (a_blurRadius > 0 ? a_blurRadius : 1) * 0.5f;
				const float inv_two_sigma_sq = 1.0f / (2.0f * sigma * sigma);
				float       sum = 0.0f;
				for (std::int32_t i = 0; i <= a_blurRadius; ++i) {
					const float v = std::expf(-static_cast<float>(i * i) * inv_two_sigma_sq);
					w[a_blurRadius + i] = v;
					w[a_blurRadius - i] = v;
					sum += (i == 0) ? v : 2.0f * v;
				}
				const float inv_sum = 1.0f / sum;
				for (std::int32_t i = 0; i < 2 * a_blurRadius + 1; ++i) {
					w[i] *= inv_sum;
				}
				radius = a_blurRadius;
			}

			std::int32_t                          radius{ -1 };
			std::array<float, 2 * MAX_RADIUS + 1> w{};
		};

		constexpr auto blur_radius = 2;

		static Kernel k(blur_radius);

		const std::int32_t sr = (shadow_color >> IM_COL32_R_SHIFT) & 0xFF;
		const std::int32_t sg = (shadow_color >> IM_COL32_G_SHIFT) & 0xFF;
		const std::int32_t sb = (shadow_color >> IM_COL32_B_SHIFT) & 0xFF;

		const float peak = k.w[blur_radius];

		for (std::int32_t dy = -blur_radius; dy <= blur_radius; ++dy) {
			const float wy = k.w[blur_radius + dy];
			const float sa_wy = static_cast<float>(sa) * wy;
			if (static_cast<std::int32_t>(sa_wy * peak + 0.5f) <= 0) {
				continue;
			}

			for (std::int32_t dx = -blur_radius; dx <= blur_radius; ++dx) {
				const auto a = static_cast<std::int32_t>(sa_wy * k.w[blur_radius + dx] + 0.5f);
				if (a <= 0) {
					continue;
				}
				const ImU32 col = IM_COL32(sr, sg, sb, a);
				func(ImVec2(shadow_center.x + dx, shadow_center.y + dy), col);
			}
		}

		func(a_pos, text_color);
	}

	void TextWithBlurredShadow(ImDrawList* a_drawlist, const ImVec2& a_pos, const char* a_text, const char* a_textEnd = nullptr);
}
