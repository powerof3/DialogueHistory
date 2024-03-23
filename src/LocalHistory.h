#pragma once

#include "Dialogue.h"

namespace LocalHistory
{
	class Manager :
		public ISingleton<Manager>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		static void Register();

		void LoadMCMSettings(const CSimpleIniA& a_ini);

		void Draw();
		bool ShouldDraw();

		bool IsDialogueMenuOpen() const;
		bool IsLocalHistoryOpen() const;
		bool ShouldHide();

		void ToggleActive();
		void SetDialogueMenuOpen(bool a_opened);
		void SetLocalHistoryOpen(bool a_opened);

		void AddDialogue(RE::TESObjectREFR* a_speaker, const std::string& a_response, const std::string& a_voice);
		void SaveDialogueHistory();

	private:
		void UpdateDialogue();

		void SetupLocalHistoryMenu(bool a_opened, bool a_blurBG = true);

		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		// members
		bool dialogueMenuOpen{ false };
		bool localHistoryMenuOpen{ false };
		bool tempClosed{ false };

		std::string        gameTimeString;
		std::tm            gameTime;
		RE::TESObjectREFR* currentSpeaker;

		Dialogue localDialogue{};

		bool unpauseMenu{ false };
		bool blurMenu{ true };
	};
}
