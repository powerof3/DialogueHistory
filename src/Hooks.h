#pragma once

#include "LocalHistory.h"

namespace Hooks
{
	template <std::size_t N>
	struct ShowSubtitle
	{
		static void thunk(RE::SubtitleManager* a_this, RE::TESObjectREFR* a_speaker, const char* a_subtitle, bool a_alwaysDisplay)
		{
			func(a_this, a_speaker, a_subtitle, a_alwaysDisplay);

			if (a_speaker && !a_speaker->IsPlayerRef()) {
				std::string subtitle = a_subtitle;
				std::string voice;
				if (auto topic = RE::MenuTopicManager::GetSingleton()->lastSelectedDialogue) {
					if (auto response = topic->currentResponse; response && response->item) {
						if (voice = response->item->voice; !voice.empty()) {
							// Strip "Data\"
							voice.erase(0, 5);
						}
					}
				}
				MANAGER(LocalHistory)->AddDialogue(a_speaker, (subtitle.empty() || subtitle == " ") ? "..." : a_subtitle, voice);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	template <std::size_t N>
	struct PushHUDMode
	{
		static void thunk(const char* a_mode)
		{
			func(a_mode);

			MANAGER(LocalHistory)->SetDialogueMenuOpen(true);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	template <std::size_t N>
	struct PopHUDMode
	{
		static void thunk(const char* a_mode)
		{
			func(a_mode);

			MANAGER(LocalHistory)->SetDialogueMenuOpen(false);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
	
	void Install();
}
