#pragma once

namespace IconFont
{
	struct IconTexture;
}

namespace Hotkeys
{
	class Manager : public ISingleton<Manager>
	{
	public:
		void LoadHotKeys(const CSimpleIniA& a_ini);

		bool TryToggleDialogueHistory(const RE::InputEvent* const* a_event);

		static std::uint32_t         EscapeKey();
		const IconFont::IconTexture* EscapeIcon() const;

		std::set<const IconFont::IconTexture*> GlobalHistoryIcons() const;
		std::set<const IconFont::IconTexture*> LocalHistoryIcons() const;

	private:
		struct KeyCombo
		{
			KeyCombo() = default;
			KeyCombo(const std::string& a_type);

			void LoadKeys(const CSimpleIniA& a_ini);

			bool                    IsInvalid() const;
			std::set<std::uint32_t> GetKeys() const;

			bool ProcessKeyPress(const RE::InputEvent* const* a_event, std::function<void()> a_callback);

		private:
			struct KeyComboImpl
			{
				void LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting);

				std::int32_t primary{ -1 };
				std::int32_t modifier{ -1 };

				std::set<std::uint32_t> keys{};
			};

			KeyComboImpl keyboard;
			KeyComboImpl gamePad;

			bool        triggered{ false };
			std::string type;
		};

		KeyCombo localHistory{ "LocalHistory" };
		KeyCombo globalHistory{ "GlobalHistory" };
	};
}
