#pragma once

#include "Dialogue.h"

namespace GlobalHistory
{
	inline std::string nameFilter{};
	inline std::string lastNameFilter{};

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

	using DialogueDate = TimeStampMap<TimeStampMap<Dialogue>>;
	using DialogueLocation = std::map<std::string, TimeStampMap<Dialogue>, comparator>;

	using MonologueDate = TimeStampMap<Monologues>;
	using MonologueLocation = std::map<std::string, TimeStampMap<Monologues>, comparator>;

	template <class T>
	struct DialogueMap
	{
		bool empty() const { return map.empty(); };

		const T& get_map()
		{
			if (nameFilter.empty()) {
				return map;
			}

			if (cachedFilter == nameFilter) {
				return filteredMap;
			}

			cachedFilter = nameFilter;
			filteredMap = map;

			if constexpr (std::is_same_v<T, MonologueDate>) {
				std::erase_if(filteredMap, [](auto& item) {
					auto& [root, monologueVec] = item;
					std::erase_if(monologueVec.monologues, [&](const auto& monologue) {
						return !string::icontains(monologue.speakerName, nameFilter);
					});
					return monologueVec.empty();
				});
			} else if constexpr (std::is_same_v<T, MonologueLocation>) {
				std::erase_if(filteredMap, [](auto& item) {
					auto& [root, monologueMap] = item;
					std::erase_if(monologueMap, [&](auto& item) {
						auto& [timeStamp, monologueVec] = item;
						std::erase_if(monologueVec.monologues, [&](const auto& monologue) {
							return !string::icontains(monologue.speakerName, nameFilter);
						});
						return monologueVec.empty();
					});
					return monologueMap.empty();
				});
			} else {
				std::erase_if(filteredMap, [](auto& item) {
					auto& [root, dialogueMap] = item;
					std::erase_if(dialogueMap, [&](auto& item) {
						const auto& [timeStamp, dialogue] = item;
						return !string::icontains(dialogue.speakerName, nameFilter);
					});
					return dialogueMap.empty();
				});
			}

			return filteredMap;
		}

		void clear_filter()
		{
			filteredMap.clear();
			cachedFilter.clear();
		}

		void clear()
		{
			map.clear();
			clear_filter();
		}

		// members
		T           map{};
		T           filteredMap{};
		std::string cachedFilter{};
	};

	template <class HistoryData, class DateMap, class LocationMap>
	struct BaseHistory
	{
		virtual ~BaseHistory() = default;

		virtual void DrawDateTree(){};
		virtual void DrawLocationTree(){};
		void         DrawTree(bool a_sortByLocation)
		{
			if (a_sortByLocation) {
				DrawLocationTree();
			} else {
				DrawDateTree();
			}
		}

		virtual void ClearCurrentHistory() { currentHistory = std::nullopt; };
		bool         CanDrawHistory() { return currentHistory.has_value(); }
		void         DrawHistory()
		{
			if (currentHistory) {
				currentHistory->Draw();
			}
		}

		virtual void SetCurrentHistory(const HistoryData& a_history)
		{
			currentHistory = a_history;
			currentHistory->RefreshContents();
		};
		virtual const char*                          GetType() { return nullptr; }
		virtual std::optional<std::filesystem::path> GetDirectory() { return std::nullopt; };
		std::optional<std::filesystem::path>         GetFile(const std::string& a_save)
		{
			auto jsonPath = GetDirectory();

			if (!jsonPath) {
				return {};
			}

			*jsonPath /= a_save;
			jsonPath->replace_extension(".json");

			return jsonPath;
		}
		void DeleteSavedFile(const std::string& a_save)
		{
			auto jsonPath = GetFile(a_save);
			if (!jsonPath) {
				return;
			}
			std::error_code ec;
			std::filesystem::remove(*jsonPath, ec);
		}
		void CleanupSavedFiles(const std::filesystem::path& a_saveDir)
		{
			std::uint32_t count = 0;

			if (auto dir = GetDirectory()) {
				std::error_code ec;

				for (const auto& entry : std::filesystem::directory_iterator(*dir)) {
					if (entry.exists() && entry.path().extension() == ".json"sv) {
						auto saveFileName = entry.path().stem().string();
						auto savePath = std::format("{}{}.ess", a_saveDir.string(), saveFileName);
						if (!std::filesystem::exists(savePath, ec)) {
							std::filesystem::remove(entry.path(), ec);
							count++;
						}
					}
				}
			}

			logger::info("{} : Cleaned up {} unused history files.", GetType(), count);
		}

		void Clear()
		{
			dateMap.clear();
			locationMap.clear();
		}
		void ClearFilters()
		{
			dateMap.clear_filter();
			locationMap.clear_filter();
		}

		// members
		DialogueMap<DateMap>                 dateMap{};      // 8th of Last Seed, 4E 201 -> 13:53, Lydia
		DialogueMap<LocationMap>             locationMap{};  // Dragonsreach -> Lydia
		std::optional<HistoryData>           currentHistory{ std::nullopt };
		std::optional<std::filesystem::path> directory;

	protected:
		template <class T>
		bool LoadHistoryFromFileImpl(T&& a_history, const std::string& a_save);
		template <class T>
		void SaveHistoryToFileImpl(T&& a_history, const std::string& a_save);

		std::optional<std::filesystem::path> GetDirectoryImpl();
	};

	// Dialogue between player and NPC
	struct DialogueHistory : public BaseHistory<Dialogue, DialogueDate, DialogueLocation>
	{
	public:
		virtual ~DialogueHistory() override = default;

		void        RefreshTimeStamps(bool a_use12HourFormat);
		void        DrawDateTree() override;
		void        DrawLocationTree() override;
		const char* GetType() override { return "DialogueHistory"; }
		void        SaveHistory(const std::tm& a_tm, const Dialogue& a_history, bool a_use12HourFormat);
		void        SaveHistoryToFile(const std::string& a_save);
		bool        LoadHistoryFromFile(const std::string& a_save);

		std::optional<std::filesystem::path> GetDirectory() override;

		void InitHistory();

		//members
		std::vector<Dialogue> history{};

	private:
		template <class T>
		void DrawTreeImpl(DialogueMap<T>& a_map);
	};

	// Standalone NPC dialogue
	struct ConversationHistory : public BaseHistory<Monologues, MonologueDate, MonologueLocation>
	{
	public:
		virtual ~ConversationHistory() override = default;

		void LoadMCMSettings(const CSimpleIniA& a_ini);

		void RefreshTimeStamps();
		void DrawDateTree() override;
		void DrawLocationTree() override;

		void ClearCurrentHistory() override;
		void SetCurrentHistory(const Monologues& a_history) override;
		void RefreshCurrentHistory();
		void RevertCurrentHistory();

		const char* GetType() override { return "ConversationHistory"; }

		void SaveHistory(const std::tm& a_tm, const Monologue& a_history);
		void SaveHistoryToFile(const std::string& a_save);
		bool LoadHistoryFromFile(const std::string& a_save);

		std::optional<std::filesystem::path> GetDirectory() override;

		void InitHistory();

		bool CanShowDialogue(std::int32_t a_dialogueType) const;
		void RefreshHistoryMaps();

		// members
		Monologues                history{};
		std::optional<Monologues> currentFixedHistory{ std::nullopt };  // for search filter

		bool showScene{ true };
		bool showCombat{ true };
		bool showFavor{ true };
		bool showDetection{ true };
		bool showMisc{ true };

	private:
		template <class T>
		void DrawTreeImpl(DialogueMap<T>& a_map);
	};

	class Manager :
		public REX::Singleton<Manager>,
		public RE::BSTEventSink<RE::TESLoadGameEvent>,
		public RE::BSTEventSink<RE::TESTopicInfoEvent>,
		public RE::BSTEventSink<SKSE::ModCallbackEvent>
	{
	public:
		void Register();

		void LoadMCMSettings(const CSimpleIniA& a_ini);

		bool IsValid() const;
		void Draw();

		bool IsGlobalHistoryOpen() const;
		void SetGlobalHistoryOpen(bool a_open, bool a_showCursor = true);
		void ToggleActive();
		bool TryOpenFromTweenMenu(bool a_showCursor = true);

		bool WasMenuOpenJustNow() const;
		void SetMenuOpenJustNow(bool a_open);

		bool Use12HourFormat() const;

		void SaveDialogueHistory(const std::tm& a_time, const Dialogue& a_dialogue);

		void SaveFiles(const std::string& a_save);
		void LoadFiles(const std::string& a_save);
		void DeleteSavedFiles(const std::string& a_save);
		void CleanupSavedFiles();
		void Clear();

		void PlayVoiceline(const std::string& a_voiceline);

	private:
		void AddConversation(const RE::TESObjectREFRPtr& a_speaker, RE::TESTopicInfo* a_info);

		EventResult ProcessEvent(const RE::TESLoadGameEvent* a_evn, RE::BSTEventSource<RE::TESLoadGameEvent>*) override;
		EventResult ProcessEvent(const RE::TESTopicInfoEvent* a_evn, RE::BSTEventSource<RE::TESTopicInfoEvent>*) override;
		EventResult ProcessEvent(const SKSE::ModCallbackEvent* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override;

		// members
		bool                globalHistoryOpen{ false };
		bool                menuOpenedJustNow{ false };
		bool                openFromTweenMenu{ false };
		bool                skyrimSoulsInstalled{ false };
		DialogueHistory     dialogueHistory;
		ConversationHistory conversationHistory;
		bool                drawConversation{ false };
		RE::BSSoundHandle   voiceHandle{};
		bool                finishLoading{ false };
		bool                sortByLocation{ false };
		bool                use12HourFormat{ false };
		bool                unpauseMenu{ false };
		bool                blurMenu{ true };
		bool                hideButton{ false };
	};

	template <class HistoryData, class DateMap, class LocationMap>
	inline std::optional<std::filesystem::path> BaseHistory<HistoryData, DateMap, LocationMap>::GetDirectoryImpl()
	{
		if (auto dir = logger::log_directory()) {
			dir->remove_filename();
			*dir /= "Saves";
			*dir /= GetType();
			std::error_code ec;
			if (!std::filesystem::exists(*dir, ec)) {
				std::filesystem::create_directory(*dir, ec);
			}
			return dir;
		}
		logger::error("Unable to access {} directory", GetType());
		return std::nullopt;
	}

	template <class HistoryData, class DateMap, class LocationMap>
	template <class T>
	inline bool BaseHistory<HistoryData, DateMap, LocationMap>::LoadHistoryFromFileImpl(T&& a_history, const std::string& a_save)
	{
		const auto& jsonPath = GetFile(a_save);
		if (!jsonPath) {
			return false;
		}

		Clear();

		logger::info("Loading {} file : {}", GetType(), jsonPath->string());

		std::error_code err;
		if (std::filesystem::exists(*jsonPath, err)) {
			std::string buffer;
			auto        ec = glz::read_file_json(a_history, jsonPath->string(), buffer);
			if (ec) {
				logger::info("\tFailed to read {} file (error: {})", GetType(), glz::format_error(ec, buffer));
			}
		} else {
			logger::info("\tFailed to load {} file (error: {})", GetType(), err.message());
		}

		return true;
	}

	template <class HistoryData, class DateMap, class LocationMap>
	template <class T>
	inline void BaseHistory<HistoryData, DateMap, LocationMap>::SaveHistoryToFileImpl(T&& a_history, const std::string& a_save)
	{
		const auto& jsonPath = GetFile(a_save);
		if (!jsonPath) {
			return;
		}

		logger::info("Saving {} file : {}", GetType(), jsonPath->string());

		std::string buffer;
		auto        ec = glz::write_file_json(a_history, jsonPath->string(), buffer);

		if (ec) {
			logger::info("\tFailed to save {} file: (error: {})", GetType(), glz::format_error(ec, buffer));
		}
	}

	template <class T>
	inline void DialogueHistory::DrawTreeImpl(DialogueMap<T>& a_map)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
		{
			const auto& map = a_map.get_map();
			for (auto& [root, leafMap] : map) {
				if (MANAGER(GlobalHistory)->WasMenuOpenJustNow()) {
					ImGui::SetNextItemOpen(true);
				}
				bool rootOpen;
				if constexpr (std::is_same_v<DialogueLocation, T>) {
					rootOpen = ImGui::TreeNodeEx(root.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
				} else {
					rootOpen = ImGui::TreeNodeEx(root.format.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
				}
				if (ImGui::IsItemToggledOpen()) {
					ClearCurrentHistory();
					MANAGER(GlobalHistory)->SetMenuOpenJustNow(false);
				}
				if (rootOpen) {
					for (auto& [leaf, dialogue] : leafMap) {
						auto leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
						auto is_selected = currentHistory && currentHistory == dialogue;
						if (is_selected) {
							leafFlags |= ImGuiTreeNodeFlags_Selected;
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
						}
						if (ImGui::TreeNodeEx(leaf.format.c_str(), leafFlags)) {
							ImGui::TreePop();
						}
						if (ImGui::IsItemSelected() && !ImGui::IsItemToggledOpen()) {
							if (dialogue != currentHistory) {
								SetCurrentHistory(dialogue);
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

	template <class T>
	inline void ConversationHistory::DrawTreeImpl(DialogueMap<T>& a_map)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

		const auto& map = a_map.get_map();
		if constexpr (std::is_same_v<MonologueLocation, T>) {
			for (auto& [root, leafMap] : map) {
				if (MANAGER(GlobalHistory)->WasMenuOpenJustNow()) {
					ImGui::SetNextItemOpen(true);
				}
				bool rootOpen = ImGui::TreeNodeEx(root.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
				if (ImGui::IsItemToggledOpen()) {
					ClearCurrentHistory();
					MANAGER(GlobalHistory)->SetMenuOpenJustNow(false);
				}
				if (rootOpen) {
					for (auto& [leaf, monologue] : leafMap) {
						auto leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;
						auto is_selected = currentHistory && currentHistory == monologue;
						if (is_selected) {
							leafFlags |= ImGuiTreeNodeFlags_Selected;
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
						}
						if (ImGui::TreeNodeEx(leaf.format.c_str(), leafFlags)) {
							ImGui::TreePop();
						}
						if (ImGui::IsItemSelected() && !ImGui::IsItemToggledOpen()) {
							if (monologue != currentHistory) {
								SetCurrentHistory(monologue);
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
		} else {
			if (MANAGER(GlobalHistory)->WasMenuOpenJustNow()) {
				ImGui::SetNextItemOpen(true);
			}
			if (ImGui::IsItemToggledOpen()) {
				ClearCurrentHistory();
				MANAGER(GlobalHistory)->SetMenuOpenJustNow(false);
			}
			for (auto& [leaf, monologue] : map) {
				auto leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;
				auto is_selected = currentHistory && currentHistory == monologue;
				if (is_selected) {
					leafFlags |= ImGuiTreeNodeFlags_Selected;
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
				}
				if (ImGui::TreeNodeEx(leaf.format.c_str(), leafFlags)) {
					ImGui::TreePop();
				}
				if (ImGui::IsItemSelected() && !ImGui::IsItemToggledOpen()) {
					if (monologue != currentHistory) {
						SetCurrentHistory(monologue);
						RE::PlaySound("UIMenuFocus");
					}
				}
				if (is_selected) {
					ImGui::PopStyleColor();
				}
			}
		}

		ImGui::PopStyleVar();
	}
}
