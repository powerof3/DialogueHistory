#pragma once

namespace ImGui
{
	enum class USER_STYLE
	{
		kButtons,
		kSpeakerName,
		kSpeakerLine,
		kPlayerName,
		kPlayerLine,
		kDisabledTextAlpha,
		kSeparatorThickness,
	};

	class Styles : public ISingleton<Styles>
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
		T ToStyle(const std::string& a_str);
		template <class T>
		std::string ToString(const T& a_style);

		// members
		// unused, helpers
		float bgAlpha{ 0.68f };
		float disabledAlpha{ 0.30f };

		float buttonScale{ 0.6f };
		float indentSpacing{ 8.0f };

		ImVec4 background{ 0.0f, 0.0f, 0.0f, bgAlpha };

		ImVec4 border{ 0.396f, 0.404f, 0.412f, bgAlpha };
		float  borderSize{ 3.5f };

		ImVec4 text{ 1.0f, 1.0f, 1.0f, 1.0f };
		float  textDisabledAlpha{ 0.62f };

		ImVec4 header{ 1.0f, 1.0f, 1.0f, 0.15f };
		ImVec4 headerHovered{ 1.0f, 1.0f, 1.0f, 0.1f };

		ImVec4 separator{ 0.396f, 0.404f, 0.412f, bgAlpha };
		float  separatorThickness{ 3.5f };

		ImVec4 speakerName{ 1.0f, 1.0f, 1.0f, 1.0f };
		ImVec4 speakerLine{ 1.0f, 1.0f, 1.0f, 1.0f };

		ImVec4 playerName{ 1.0f, 1.0f, 1.0f, 1.0f };
		ImVec4 playerLine{ 1.0f, 1.0f, 1.0f, 1.0f };

		bool refreshStyle{ false };
	};

	ImVec4 GetUserStyleColorVec4(USER_STYLE a_style);
	float  GetUserStyleVar(USER_STYLE a_style);

	template <class T>
	inline T Styles::ToStyle(const std::string& a_str)
	{
		if constexpr (std::is_same_v<ImVec4, T>) {
			static srell::regex pattern("([0-9]+),([0-9]+),([0-9]+),([0-9]+)");
			srell::smatch       matches;
			if (srell::regex_match(a_str, matches, pattern)) {
				auto red = std::stoi(matches[1]);
				auto green = std::stoi(matches[2]);
				auto blue = std::stoi(matches[3]);
				auto alpha = std::stoi(matches[4]);
				return { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f };
			}
			return T{};
		} else {
			return string::to_num<T>(a_str);
		}
	}

	template <class T>
	inline std::string Styles::ToString(const T& a_style)
	{
		if constexpr (std::is_same_v<ImVec4, T>) {
			return std::format("{},{},{},{}", std::round(255.0f * a_style.x), std::round(255.0f * a_style.y), std::round(255.0f * a_style.z), std::round(255.0f * a_style.w));
		} else {
			return std::format("{:.3f}", a_style);
		}
	}
}
