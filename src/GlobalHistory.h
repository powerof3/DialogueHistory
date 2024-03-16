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

		bool Use12HourFormat() const;

		void SaveDialogueHistory(RE::TESObjectREFR* a_speaker, const std::tm& a_time, const Dialogue& a_dialogue);
		void RefreshTimeStamps();

		void SaveToFile(const std::string& a_save);
		void LoadFromFile(const std::string& a_save);
		void FinishLoadFromFile();
		void DeleteSavedFile(const std::string& a_save);
		void Clear();

		void PlayVoiceline(const std::string& a_voiceline);

	private:
		struct comparator
		{
			// greater than
			bool operator()(const TimeStamp& a_lhs, const TimeStamp& a_rhs) const
			{
				return a_lhs > a_rhs;
			}
			// lesser than
			bool operator()(const std::string& a_lhs, const std::string& a_rhs) const
			{
				return a_lhs < a_rhs;
			}
		};

		template <class D>
		using TimeStampMap = std::map<TimeStamp, D, comparator>;
		using DialogueDateMap = TimeStampMap<TimeStampMap<Dialogue>>;
		using DialogueLocationMap = std::map<std::string, TimeStampMap<Dialogue>, comparator>;

		template <class T>
		void DrawDialogueTree(const std::map<T, TimeStampMap<Dialogue>, comparator>& a_map);

		std::optional<std::filesystem::path> GetSaveDirectory();
		std::optional<std::filesystem::path> GetDialogueHistoryFile(const std::string& a_save);

		EventResult ProcessEvent(const RE::TESLoadGameEvent* a_evn, RE::BSTEventSource<RE::TESLoadGameEvent>*) override;

		// members
		bool                                 globalHistoryOpen{ false };
		bool                                 menuOpenedJustNow{ false };
		DialogueDateMap                      dialoguesByDate{};      // 8th of Last Seed, 4E 201 -> 13:53, Lydia
		DialogueLocationMap                  dialoguesByLocation{};  // Dragonsreach -> Lydia
		std::vector<Dialogue>                dialogues{};
		std::optional<Dialogue>              currentDialogue{ std::nullopt };
		RE::BSSoundHandle                    voiceHandle{};
		bool                                 finishLoading{ false };
		std::optional<std::filesystem::path> saveDirectory;
		bool                                 use12HourFormat{ false };
		bool                                 sortByLocation{ false };
	};

	template <class T>
	inline void Manager::DrawDialogueTree(const std::map<T, TimeStampMap<Dialogue>, comparator>& a_map)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
		{
			for (auto& [root, leafMap] : a_map) {
				if (menuOpenedJustNow) {
					ImGui::SetNextItemOpen(true);
				}
				bool rootOpen;
				if constexpr (std::is_same_v<std::string, T>) {
					rootOpen = ImGui::TreeNodeEx(root.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
				} else {
					rootOpen = ImGui::TreeNodeEx(root.format.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
				}
				if (ImGui::IsItemToggledOpen()) {
					currentDialogue = std::nullopt;
					menuOpenedJustNow = false;
				}
				if (rootOpen) {
					for (auto& [leaf, dialogue] : leafMap) {
						auto leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
						auto is_selected = currentDialogue && currentDialogue == dialogue;
						if (is_selected) {
							leafFlags |= ImGuiTreeNodeFlags_Selected;
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
						}
						ImGui::TreeNodeEx(leaf.format.c_str(), leafFlags);
						if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
							if (dialogue != currentDialogue) {
								currentDialogue = dialogue;
								RE::PlaySound("UIMenuFocus");
							}
						}
						if (is_selected) {
							ImGui::PopStyleColor();
						}
					}
					ImGui::TreePop();
				}
			}
		}
		ImGui::PopStyleVar();
	}
}
