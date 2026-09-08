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

	using DialogueDate = TimeStampMap<TimeStampMap<Dialogue*>>;
	using DialogueLocation = std::map<std::string, TimeStampMap<Dialogue*>, comparator>;

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
			filteredMap.clear();

			constexpr auto pass_filter = [](const auto& speech) {
				return REX::STR::ICONTAINS(speech->speakerName, nameFilter);
			};

			if constexpr (std::is_same_v<T, MonologueDate>) {
				// date -> Monologues
				for (const auto& [date, monologues] : map) {
					Monologues* out = nullptr;
					for (auto* monologue : monologues.monologues) {
						if (pass_filter(monologue)) {
							if (!out) {
								out = &filteredMap[date];
							}
							out->monologues.push_back(monologue);
						}
					}
				}
			} else if constexpr (std::is_same_v<T, MonologueLocation>) {
				// location -> date -> Monologues
				for (const auto& [loc, dateMap] : map) {
					for (const auto& [date, monologues] : dateMap) {
						Monologues* out = nullptr;
						for (auto* monologue : monologues.monologues) {
							if (pass_filter(monologue)) {
								if (!out) {
									out = &filteredMap[loc][date];
								}
								out->monologues.push_back(monologue);
							}
						}
					}
				}
			} else {
				// root -> leaf -> Dialogue*
				for (const auto& [root, leafMap] : map) {
					for (const auto& [leaf, dialogue] : leafMap) {
						if (pass_filter(dialogue)) {
							filteredMap[root][leaf] = dialogue;
						}
					}
				}
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

	template <class Derived, class Entry, class HistoryData, class DateMap, class LocationMap>
	struct BaseHistory
	{
		void DrawTree(bool a_sortByLocation)
		{
			if (a_sortByLocation) {
				derived().DrawTreeImpl(locationMap);
			} else {
				derived().DrawTreeImpl(dateMap);
			}
		}

		void ClearCurrentHistory() { currentHistory = nullptr; }

		void Clear()
		{
			ClearMaps();
			history.clear();
		}

		bool CanDrawHistory() const { return currentHistory != nullptr; }
		void DrawHistory()
		{
			if (currentHistory) {
				currentHistory->Draw();
			}
		}

		void SetCurrentHistory(HistoryData a_history)
		{
			currentHistory = a_history;
			currentHistory->RefreshContents();
		}

		static constexpr std::string_view GetType() { return Derived::TYPE; }

		std::optional<std::filesystem::path> GetDirectory();

		void SaveHistoryToFile(const std::string& a_save);
		bool LoadHistoryFromFile(const std::string& a_save);

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
			REX::INFO("{} : Cleaned up {} unused history files.", GetType(), count);
		}

		void ClearFilters()
		{
			dateMap.clear_filter();
			locationMap.clear_filter();
		}

		// members
		DialogueMap<DateMap>     dateMap{};      // 8th of Last Seed, 4E 201 -> 13:53, Lydia
		DialogueMap<LocationMap> locationMap{};  // Dragonsreach -> Lydia
		HistoryData              currentHistory{ nullptr };
		std::deque<Entry>        history{};

	protected:
		~BaseHistory() = default; 

		void ClearMaps()
		{
			dateMap.clear();
			locationMap.clear();
			derived().ClearCurrentHistory();
		}

	private:
		Derived&       derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }

		std::optional<std::filesystem::path> GetFile(const std::string& a_save)
		{
			auto jsonPath = GetDirectory();
			if (!jsonPath) {
				return {};
			}
			*jsonPath /= a_save;
			jsonPath->replace_extension(".json");
			return jsonPath;
		}

		std::optional<std::filesystem::path> directory;
	};

	// Dialogue between player and NPC
	struct DialogueHistory : public BaseHistory<DialogueHistory, Dialogue, Dialogue*, DialogueDate, DialogueLocation>
	{
	public:
		static constexpr std::string_view TYPE = "DialogueHistory"sv;

		void RefreshTimeStamps(bool a_use12HourFormat);
		void SaveHistory(const std::tm& a_tm, Dialogue a_history, bool a_use12HourFormat);
		void InitHistory();

	private:
		friend BaseHistory;

		template <class T>
		void DrawTreeImpl(DialogueMap<T>& a_map);
	};

	struct ConversationHistory : public BaseHistory<ConversationHistory, Monologue, Monologues*, MonologueDate, MonologueLocation>
	{
	public:
		static constexpr std::string_view TYPE = "ConversationHistory"sv;

		void LoadMCMSettings(const CSimpleIniA& a_ini);

		void RefreshTimeStamps();

		void ClearCurrentHistory();
		void SetCurrentHistory(Monologues* a_history);
		void RefreshCurrentHistory();

		void SaveHistory(const std::tm& a_tm, Monologue a_history);
		void InitHistory();

		bool CanShowDialogue(std::int32_t a_dialogueType) const;
		void RefreshHistoryMaps();

		// members
		Monologues* currentFixedHistory{ nullptr };
		Monologues  currentFiltered{};

		bool showScene{ true };
		bool showCombat{ true };
		bool showFavor{ true };
		bool showDetection{ true };
		bool showMisc{ true };

	private:
		friend BaseHistory;

		template <class T>
		void DrawTreeImpl(DialogueMap<T>& a_map);
	};

	class Manager :
		public REX::TSingleton<Manager>,
		public RE::BSTEventSink<RE::TESLoadGameEvent>,
		public RE::BSTEventSink<RE::TESTopicInfoEvent>
	{
	public:
		void Register();

		void LoadMCMSettings(const CSimpleIniA& a_ini);

		bool IsValid() const;
		void Draw();

		bool IsGlobalHistoryOpen() const;
		void SetGlobalHistoryOpen(bool a_open, bool a_showCursor = true);
		void ToggleActive();

		bool WasMenuOpenJustNow() const;
		void SetMenuOpenJustNow(bool a_open);

		bool ShouldAutoSelectFirstEntry() const;
		void SetAutoSelectFirstEntry(bool a_select);

		bool Use12HourFormat() const;

		void SaveDialogueHistory(const std::tm& a_time, Dialogue a_dialogue);

		void SaveFiles(const std::string& a_save);
		void LoadFiles(const std::string& a_save);
		void DeleteSavedFiles(const std::string& a_save);
		void CleanupSavedFiles();
		void Clear();

		void PlayVoiceline(const std::string& a_voiceline);

		const std::string& GetPlayerName() const;
		void               RefreshPlayerName();

		bool IsLineHovered(const void* a_line) const;
		void SetLineHovered(const void* a_line, bool a_hovered);

	private:
		void AddConversation(const RE::TESObjectREFRPtr& a_speaker, RE::TESTopicInfo* a_info);

		EventResult ProcessEvent(const RE::TESLoadGameEvent* a_evn, RE::BSTEventSource<RE::TESLoadGameEvent>*) override;
		EventResult ProcessEvent(const RE::TESTopicInfoEvent* a_evn, RE::BSTEventSource<RE::TESTopicInfoEvent>*) override;

		// members
		DialogueHistory     dialogueHistory;
		ConversationHistory conversationHistory;
		std::string         playerName;
		RE::BSSoundHandle   voiceHandle{};
		bool                drawConversation{ false };
		bool                finishLoading{ false };
		bool                sortByLocation{ false };
		bool                use12HourFormat{ false };
		bool                unpauseMenu{ false };
		bool                blurMenu{ true };
		bool                hideButton{ false };
		bool                globalHistoryOpen{ false };
		bool                menuOpenedJustNow{ false };
		bool                autoSelectFirstEntry{ false };
		const void*         hoveredLine{ nullptr };
	};

	template <class Derived, class Entry, class HistoryData, class DateMap, class LocationMap>
	inline std::optional<std::filesystem::path> BaseHistory<Derived, Entry, HistoryData, DateMap, LocationMap>::GetDirectory()
	{
		if (directory) {
			return directory;
		}

		auto dir = SKSE::log::log_directory();
		if (!dir) {
			REX::ERROR("Unable to access {} directory", GetType());
			return std::nullopt;
		}

		dir->remove_filename();  // remove "SKSE"
		*dir /= "Saves";
		*dir /= GetType();

		std::error_code ec;
		if (!std::filesystem::exists(*dir, ec)) {
			if (!std::filesystem::create_directories(*dir, ec)) {
				REX::ERROR("Failed to create {} directory: {}", GetType(), ec.message());
				return std::nullopt;
			}
		}

		directory = std::move(dir);
		return directory;
	}

	template <class Derived, class Entry, class HistoryData, class DateMap, class LocationMap>
	inline bool BaseHistory<Derived, Entry, HistoryData, DateMap, LocationMap>::LoadHistoryFromFile(const std::string& a_save)
	{
		const auto& jsonPath = GetFile(a_save);
		if (!jsonPath) {
			return false;
		}

		Clear();

		REX::INFO("Loading {} file : {}", GetType(), jsonPath->string());

		std::error_code err;
		if (std::filesystem::exists(*jsonPath, err)) {
			std::string buffer;
			auto        ec = glz::read_file_json<glz::opts{ .error_on_unknown_keys = false }>(history, jsonPath->string(), buffer);
			if (ec) {
				REX::INFO("\tFailed to read {} file (error: {})", GetType(), glz::format_error(ec, buffer));
			}
		} else {
			REX::INFO("\tFailed to load {} file (error: {})", GetType(), err.message());
		}

		return true;
	}

	template <class Derived, class Entry, class HistoryData, class DateMap, class LocationMap>
	inline void BaseHistory<Derived, Entry, HistoryData, DateMap, LocationMap>::SaveHistoryToFile(const std::string& a_save)
	{
		const auto& jsonPath = GetFile(a_save);
		if (!jsonPath) {
			return;
		}
		REX::INFO("Saving {} file : {}", GetType(), jsonPath->string());

		std::string buffer;
		auto        ec = glz::write_file_json(history, jsonPath->string(), buffer);

		if (ec) {
			REX::INFO("\tFailed to save {} file: (error: {})", GetType(), glz::format_error(ec, buffer));
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
					ImGui::ForEachVisibleMapEntry(leafMap, [&](const auto& leaf, const auto& dialogue) {
						if (MANAGER(GlobalHistory)->ShouldAutoSelectFirstEntry()) {
							SetCurrentHistory(dialogue);
							MANAGER(GlobalHistory)->SetAutoSelectFirstEntry(false);
						}
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
					});
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
					ImGui::ForEachVisibleMapEntry(leafMap, [&](const auto& leaf, const auto& monologue) {
						if (MANAGER(GlobalHistory)->ShouldAutoSelectFirstEntry()) {
							SetCurrentHistory(&a_map.map.at(root).at(leaf));
							MANAGER(GlobalHistory)->SetAutoSelectFirstEntry(false);
						}
						auto leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;
						auto is_selected = currentHistory && *currentHistory == monologue;
						if (is_selected) {
							leafFlags |= ImGuiTreeNodeFlags_Selected;
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
						}
						if (ImGui::TreeNodeEx(leaf.format.c_str(), leafFlags)) {
							ImGui::TreePop();
						}
						if (ImGui::IsItemSelected() && !ImGui::IsItemToggledOpen()) {
							if (!is_selected) {
								SetCurrentHistory(&a_map.map.at(root).at(leaf));
								RE::PlaySound("UIMenuFocus");
							}
						}
						if (is_selected) {
							ImGui::PopStyleColor();
						}
					});
					ImGui::TreePop();
				}
			}
		} else {
			ImGui::ForEachVisibleMapEntry(map, [&](const auto& leaf, const auto& monologue) {
				if (MANAGER(GlobalHistory)->ShouldAutoSelectFirstEntry()) {
					SetCurrentHistory(&a_map.map.at(leaf));
					MANAGER(GlobalHistory)->SetAutoSelectFirstEntry(false);
				}
				auto leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;
				auto is_selected = currentHistory && *currentHistory == monologue;
				if (is_selected) {
					leafFlags |= ImGuiTreeNodeFlags_Selected;
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
				}
				if (ImGui::TreeNodeEx(leaf.format.c_str(), leafFlags)) {
					ImGui::TreePop();
				}
				if (ImGui::IsItemSelected() && !ImGui::IsItemToggledOpen()) {
					if (!is_selected) {
						SetCurrentHistory(&a_map.map.at(leaf));
						RE::PlaySound("UIMenuFocus");
					}
				}
				if (is_selected) {
					ImGui::PopStyleColor();
				}
			});
		}

		ImGui::PopStyleVar();
	}
}
