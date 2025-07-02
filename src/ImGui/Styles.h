#pragma once

namespace ImGui
{
	enum class USER_STYLE
	{
		kButtonScale,
		kButtonColor,
		kSpeakerName,
		kSpeakerLine,
		kPlayerName,
		kPlayerLine,
		kDisabledTextAlpha,
		kSeparatorThickness,
	};

	class Styles : public REX::Singleton<Styles>
	{
	public:
		ImVec4 GetColorVec4(USER_STYLE a_style) const;
		float  GetVar(USER_STYLE a_style) const;

		void LoadStyles();
		void LoadStyles(CSimpleIniA& a_ini);

		void OnStyleRefresh();
		void RefreshStyle();

	private:
		template <class T>
		std::pair<T, bool> ToStyle(const std::string& a_str);
		template <class T>
		std::string ToString(const T& a_style, bool a_hex);

		// members
		struct Style
		{
			// unused, helpers
			float bgAlpha{ 0.68f };
			float disabledAlpha{ 0.30f };

			float buttonScale{ 0.6f };
			float indentSpacing{ 8.0f };

			ImVec4 background{ 0.0f, 0.0f, 0.0f, bgAlpha };

			ImVec4 border{ 0.569f, 0.545f, 0.506f, bgAlpha };
			float  borderSize{ 3.0f };

			ImVec4 text{ 1.0f, 1.0f, 1.0f, 1.0f };
			ImVec4 textButton{ 0.9843f, 0.9843f, 0.9843f, 1.0f };
			float  textDisabledAlpha{ 0.62f };

			ImVec4 header{ 1.0f, 1.0f, 1.0f, 0.15f };
			ImVec4 headerHovered{ 1.0f, 1.0f, 1.0f, 0.1f };

			ImVec4 separator{ 0.569f, 0.545f, 0.506f, bgAlpha };
			float  separatorThickness{ 3.0f };

			ImVec4 button{ 1.0f, 1.0f, 1.0f, 1.0f };

			ImVec4 frameBG{ 0.0f, 0.0f, 0.0f, 1.0f };
			float  frameBorderSize{ 1.5f };

			ImVec4 scrollbarGrab{ 0.31f, 0.31f, 0.31f, 1.0f };
			ImVec4 scrollbarGrabHovered{ 0.41f, 0.41f, 0.41f, 1.0f };
			ImVec4 scrollbarGrabActive{ 0.51f, 0.51f, 0.51f, 1.0f };

			ImVec4 speakerName{ 0.208f, 0.784f, 0.992f, 1.0f };
			ImVec4 speakerLine{ 1.0f, 1.0f, 1.0f, 1.0f };

			ImVec4 playerName{ 0.992f, 0.847f, 0.208f, 1.0f };
			ImVec4 playerLine{ 1.0f, 1.0f, 1.0f, 1.0f };
		};

		Style def;
		Style user;

		bool refreshStyle{ false };
	};

	ImVec4 GetUserStyleColorVec4(USER_STYLE a_style);
	float  GetUserStyleVar(USER_STYLE a_style);

	template <class T>
	inline std::pair<T, bool> Styles::ToStyle(const std::string& a_str)
	{
		if constexpr (std::is_same_v<ImVec4, T>) {
			static srell::regex rgb_pattern("([0-9]+),([0-9]+),([0-9]+),([0-9]+)");
			static srell::regex hex_pattern("#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");

			srell::smatch rgb_matches;
			srell::smatch hex_matches;

			if (srell::regex_match(a_str, rgb_matches, rgb_pattern)) {
				auto red = std::stoi(rgb_matches[1]);
				auto green = std::stoi(rgb_matches[2]);
				auto blue = std::stoi(rgb_matches[3]);
				auto alpha = std::stoi(rgb_matches[4]);

				return { { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f }, false };
			} else if (srell::regex_match(a_str, hex_matches, hex_pattern)) {
				auto red = std::stoi(hex_matches[1], 0, 16);
				auto green = std::stoi(hex_matches[2], 0, 16);
				auto blue = std::stoi(hex_matches[3], 0, 16);
				auto alpha = std::stoi(hex_matches[4], 0, 16);

				return { { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f }, true };
			}

			return { T(), false };
		} else {
			return { string::to_num<T>(a_str), false };
		}
	}

	template <class T>
	inline std::string Styles::ToString(const T& a_style, bool a_hex)
	{
		if constexpr (std::is_same_v<ImVec4, T>) {
			if (a_hex) {
				return std::format("#{:02X}{:02X}{:02X}{:02X}", static_cast<std::uint8_t>(255.0f * a_style.x), static_cast<std::uint8_t>(255.0f * a_style.y), static_cast<std::uint8_t>(255.0f * a_style.z), static_cast<std::uint8_t>(255.0f * a_style.w));
			}
			return std::format("{},{},{},{}", static_cast<std::uint8_t>(255.0f * a_style.x), static_cast<std::uint8_t>(255.0f * a_style.y), static_cast<std::uint8_t>(255.0f * a_style.z), static_cast<std::uint8_t>(255.0f * a_style.w));
		} else {
			return std::format("{:.3f}", a_style);
		}
	}
}
