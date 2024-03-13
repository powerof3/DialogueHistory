#pragma once

#include "Dialogue.h"

namespace GlobalHistory
{
	class Manager :
		public ISingleton<Manager>,
		public RE::BSTEventSink<RE::TESLoadGameEvent>
	{
	public:
		static void Register();

		void LoadMCMSettings(const CSimpleIniA& a_ini);

		bool IsValid();
		void Draw();

		bool IsGlobalHistoryOpen() const;
		void SetGlobalHistoryOpen(bool a_open);
		void ToggleActive();

		void SaveDialogueHistory(RE::TESObjectREFR* a_speaker, const std::tm& a_time, const Dialogue& a_dialogue);
		void RefreshTimeStamps();

		void SaveToFile(const std::string& a_save);
		void LoadFromFile(const std::string& a_save);
		void FinishLoadFromFile();
		void DeleteSavedFile(const std::string& a_save);
		void Clear();

		void PlayVoiceline(const std::string& a_voiceline);

	private:
		template <class D>
		using OrderedDateMap = std::map<TimeStamp, D, std::greater<TimeStamp>>;
		using SortedDialogueMap = OrderedDateMap<OrderedDateMap<Dialogue>>;

		std::optional<std::filesystem::path> GetSaveDirectory();
		std::optional<std::filesystem::path> GetDialogueHistoryFile(const std::string& a_save);

		EventResult ProcessEvent(const RE::TESLoadGameEvent* a_evn, RE::BSTEventSource<RE::TESLoadGameEvent>*) override;

		// members
		bool globalHistoryOpen{ false };
		bool menuOpenedJustNow{ false };

		SortedDialogueMap       sortedDialogues{};  // 8th of Last Seed, 4E 201 -> 13:53, Lydia
		std::vector<Dialogue>   dialogues{};
		std::optional<Dialogue> currentDialogue{ std::nullopt };

		RE::BSSoundHandle voiceHandle{};

		bool                                 finishLoading{ false };
		std::optional<std::filesystem::path> saveDirectory;

		bool use12HourFormat{ false };
	};
}
