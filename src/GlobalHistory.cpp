#include "GlobalHistory.h"

#include "Hooks.h"
#include "Hotkeys.h"
#include "ImGui/IconsFonts.h"
#include "ImGui/Renderer.h"
#include "ImGui/Styles.h"
#include "ImGui/Util.h"
#include "NPCNameProvider.h"

namespace GlobalHistory
{
	void DialogueHistory::RefreshTimeStamps(bool a_use12HourFormat)
	{
		if (dateMap.empty()) {
			return;
		}

		for (auto& [dayMonth, hourMinMap] : dateMap.map) {
			for (auto it = hourMinMap.begin(); it != hourMinMap.end(); it++) {
				it->second->timeAndLoc.clear();

				auto node = hourMinMap.extract(it);
				node.key().SwitchHourFormat(a_use12HourFormat);
				hourMinMap.insert(std::move(node));
			}
		}
	}

	void DialogueHistory::SaveHistory(const std::tm& a_tm, Dialogue a_history, bool a_use12HourFormat)
	{
		auto& dialogue = history.emplace_back(std::move(a_history));

		TimeStamp date;
		date.FromYearMonthDay(a_tm.tm_year, a_tm.tm_mon, a_tm.tm_mday);

		TimeStamp hourMin;
		hourMin.FromHourMin(a_tm.tm_hour, a_tm.tm_min, dialogue.speakerName, a_use12HourFormat);

		dateMap.map[date][hourMin] = &dialogue;

		TimeStamp speaker(dialogue.timeStamp, dialogue.speakerName);
		locationMap.map[dialogue.locName][speaker] = &dialogue;
	}

	void DialogueHistory::InitHistory()
	{
		ClearMaps();

		if (!history.empty()) {
			std::erase_if(history, [&](auto& dialogue) { return !dialogue.Resolve(); });

			std::uint64_t lastDay = std::numeric_limits<std::uint64_t>::max();
			TimeStamp     date;
			bool          use12HourFormat = MANAGER(GlobalHistory)->Use12HourFormat();

			for (auto& dialogue : history) {
				auto time = dialogue.ExtractTimeStamp();

				if (const auto day = dialogue.timeStamp / 10000; day != lastDay) {
					date.FromYearMonthDay(time.tm_year, time.tm_mon, time.tm_mday);
					lastDay = day;
				}

				TimeStamp hourMin;
				hourMin.FromHourMin(time.tm_hour, time.tm_min, dialogue.speakerName, use12HourFormat);

				TimeStamp speaker(dialogue.timeStamp, dialogue.speakerName);

				dateMap.map[date][hourMin] = &dialogue;
				locationMap.map[dialogue.locName][speaker] = &dialogue;
			}
		}
	}

	void ConversationHistory::RefreshTimeStamps()
	{
		for (auto& monologue : history) {
			monologue.hourMinTimeStamp.clear();
		}
	}

	void ConversationHistory::ClearCurrentHistory()
	{
		BaseHistory::ClearCurrentHistory();
		currentFixedHistory = nullptr;
		currentFiltered.monologues.clear();
	}

	void ConversationHistory::SetCurrentHistory(Monologues* a_history)
	{
		currentFixedHistory = a_history;
		RefreshCurrentHistory();
	}

	void ConversationHistory::RefreshCurrentHistory()
	{
		if (!currentFixedHistory) {
			currentHistory = nullptr;
			return;
		}

		if (nameFilter.empty()) {
			currentHistory = currentFixedHistory;
		} else {
			currentFiltered.monologues = currentFixedHistory->monologues;
			std::erase_if(currentFiltered.monologues, [&](const auto* monologue) {
				return !REX::STR::ICONTAINS(monologue->speakerName, nameFilter);
			});
			currentHistory = &currentFiltered;
		}
		currentHistory->RefreshContents();
	}

	void ConversationHistory::SaveHistory(const std::tm& a_tm, Monologue a_history)
	{
		auto& monologue = history.emplace_back(std::move(a_history));

		if (MANAGER(GlobalHistory)->IsGlobalHistoryOpen() && CanShowDialogue(monologue.dialogueType)) {
			TimeStamp date;
			date.FromYearMonthDay(a_tm.tm_year, a_tm.tm_mon, a_tm.tm_mday);

			dateMap.map[date].monologues.push_back(&monologue);
			locationMap.map[monologue.locName][date].monologues.push_back(&monologue);

			dateMap.clear_filter();
			locationMap.clear_filter();

			if (currentFixedHistory) {
				RefreshCurrentHistory();
			}
		}
	}

	void ConversationHistory::InitHistory()
	{
		ClearMaps();

		if (!history.empty()) {
			std::erase_if(history, [&](auto& monologue) { return !monologue.Resolve(); });
		}
	}

	void ConversationHistory::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		showScene = a_ini.GetBoolValue("Settings", "bSceneDialogueConversationHistory", showScene);
		showCombat = a_ini.GetBoolValue("Settings", "bCombatDialogueConversationHistory", showCombat);
		showFavor = a_ini.GetBoolValue("Settings", "bFavorDialogueConversationHistory", showFavor);
		showDetection = a_ini.GetBoolValue("Settings", "bDetectionDialogueConversationHistory", showDetection);
		showMisc = a_ini.GetBoolValue("Settings", "bMiscDialogueConversationHistory", showMisc);
	}

	bool ConversationHistory::CanShowDialogue(std::int32_t a_dialogueType) const
	{
		switch (a_dialogueType) {
		case RE::DIALOGUE_TYPE::kSceneDialogue:
			return showScene;
		case RE::DIALOGUE_TYPE::kCombat:
			return showCombat;
		case RE::DIALOGUE_TYPE::kFavors:
			return showFavor;
		case RE::DIALOGUE_TYPE::kDetection:
			return showDetection;
		case RE::DIALOGUE_TYPE::kMiscellaneous:
			return showMisc;
		default:
			return true;
		}
	}

	void ConversationHistory::RefreshHistoryMaps()
	{
		ClearMaps();

		std::uint64_t lastDay = std::numeric_limits<std::uint64_t>::max();
		TimeStamp     date;
		Monologues*   monologuesOnThisDate = nullptr;

		for (auto& monologue : history) {
			if (!CanShowDialogue(monologue.dialogueType)) {
				continue;
			}

			const auto day = monologue.timeStamp / 10000;  // strip hhmm
			if (day != lastDay) {
				auto time = monologue.ExtractTimeStamp();
				date.FromYearMonthDay(time.tm_year, time.tm_mon, time.tm_mday);
				monologuesOnThisDate = &dateMap.map[date];
				lastDay = day;
			}

			monologuesOnThisDate->monologues.push_back(&monologue);
			locationMap.map[monologue.locName][date].monologues.push_back(&monologue);
		}
	}

	void Manager::Register()
	{
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESLoadGameEvent>(this);
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESTopicInfoEvent>(this);
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		use12HourFormat = a_ini.GetBoolValue("Settings", "b12HourFormat", use12HourFormat);
		unpauseMenu = a_ini.GetBoolValue("Settings", "bUnpauseGlobalHistory", unpauseMenu);
		blurMenu = a_ini.GetBoolValue("Settings", "bBlurGlobalHistory", blurMenu);
		hideButton = a_ini.GetBoolValue("Settings", "bHideButtonGlobalHistory", hideButton);

		conversationHistory.LoadMCMSettings(a_ini);

		dialogueHistory.RefreshTimeStamps(use12HourFormat);
		conversationHistory.RefreshTimeStamps();
	}

	bool Manager::IsValid() const
	{
		static constexpr std::array badMenus{
			RE::MainMenu::MENU_NAME,
			RE::MistMenu::MENU_NAME,
			RE::LoadingMenu::MENU_NAME,
			RE::FaderMenu::MENU_NAME,
			"LootMenu"sv,
			"CustomMenu"sv
		};

		if (const auto UI = RE::UI::GetSingleton();
			!UI || !UI->IsShowingMenus() || std::ranges::any_of(badMenus, [&](const auto& menuName) { return UI->IsMenuOpen(menuName); })) {
			return false;
		}

		if (const auto* controlMap = RE::ControlMap::GetSingleton();
			!controlMap || controlMap->contextPriorityStack.back() != RE::UserEvents::INPUT_CONTEXT_ID::kGameplay || controlMap->textEntryCount) {
			return false;
		}

		if (PhotoMode::IsPhotoModeActive()) {
			return false;
		}

		return true;
	}

	void Manager::Draw()
	{
		if (!IsGlobalHistoryOpen()) {
			return;
		}

		ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
		ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
		{
			constexpr auto windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
			static float   toggleHeight = ImGui::GetFrameHeight() / 1.5f;
			float          itemSpacing = ImGui::GetStyle().ItemSpacing.x;

			ImGui::SetNextWindowPos(ImGui::GetNativeViewportCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

			auto [globalFont, globalFontSize] = MANAGER(IconFont)->GetGlobalHistoryFont();
			ImGui::PushFont(globalFont, globalFontSize);

			ImGui::BeginChild("##GlobalHistory", ImGui::GetNativeViewportSize() * 0.8f, ImGuiChildFlags_Borders, windowFlags);
			{
				ImGui::ExtendWindowPastBorder();

				ImGui::Spacing(2);

				auto [headerFont, headerFontSize] = MANAGER(IconFont)->GetHeaderFont();
				ImGui::PushFont(headerFont, headerFontSize);
				{
					static float width = ImGui::CalcTextSize("$DH_Title"_T).x + (itemSpacing * 2) + (toggleHeight * 0.5f) + ImGui::CalcTextSize("$DH_Title_Conversation"_T).x;
					ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - width) * 0.5f);

					auto cursorY = ImGui::GetCursorPosY();
					ImGui::SetCursorPosY(cursorY - (toggleHeight * 0.25f));

					ImGui::BeginDisabled(drawConversation);
					ImGui::TextUnformatted("$DH_Title"_T);
					ImGui::EndDisabled();
					ImGui::SameLine();

					ImGui::SetCursorPosY(cursorY);
					if (ImGui::ToggleButton("##DialogueToggle", &drawConversation)) {
						OnTreeSwitch();
					}

					ImGui::SameLine();
					ImGui::SetCursorPosY(cursorY - (toggleHeight * 0.25f));
					ImGui::BeginDisabled(!drawConversation);
					ImGui::TextUnformatted("$DH_Title_Conversation"_T);
					ImGui::EndDisabled();
				}
				ImGui::PopFont();

				ImGui::Spacing(2);
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
				ImGui::Spacing(2);

				auto childSize = ImGui::GetContentRegionAvail();

				float toggleButtonOffset = ImGui::CalcTextSize("$DH_Date_Text"_T).x + itemSpacing + toggleHeight * 0.5f;

				ImGui::BeginGroup();
				{
					auto startPos = childSize.x * 0.25f;                                    // search box end
					auto endPos = (childSize.x * 0.5f) - toggleButtonOffset - itemSpacing;  // "By Date" text start

					ImGui::BeginChild(drawConversation ? "##MapConversation" : "##MapDialogue", { (startPos + endPos) * 0.5f, childSize.y * 0.9125f }, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
					{
						if (drawConversation) {
							conversationHistory.DrawTree(sortByLocation);
						} else {
							dialogueHistory.DrawTree(sortByLocation);
						}
					}
					ImGui::EndChild();

					ImGui::SameLine();
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
					ImGui::SameLine();

					childSize = ImGui::GetContentRegionAvail();

					if (drawConversation ? conversationHistory.CanDrawHistory() : dialogueHistory.CanDrawHistory()) {
						ImGui::BeginChild("##History", ImVec2(0, childSize.y * 0.9125f), ImGuiChildFlags_None, windowFlags | ImGuiWindowFlags_NoBackground);
						{
							if (drawConversation) {
								conversationHistory.DrawHistory();
							} else {
								dialogueHistory.DrawHistory();
							}
						}
						ImGui::EndChild();
					}
				}
				ImGui::EndGroup();

				ImGui::Spacing(2);
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));

				childSize = ImGui::GetContentRegionAvail();

				ImGui::BeginChild("##BottomBar", ImVec2(childSize.x, childSize.y), ImGuiChildFlags_None, windowFlags | ImGuiWindowFlags_NoBackground);
				{
					childSize = ImGui::GetContentRegionMax();

					// search box
					ImGui::SameLine();
					ImGui::SetCursorPosY(childSize.y * 0.125f);
					ImGui::SetNextItemWidth(childSize.x * 0.25f);
					if (drawConversation) {
						lastNameFilter = nameFilter;
					}
					if (ImGui::InputTextWithHint("##Name", "$DH_Name_Text"_T, &nameFilter)) {
						if (drawConversation) {
							conversationHistory.RefreshCurrentHistory();
						} else {
							dialogueHistory.ClearCurrentHistory();
						}
					}
					if (drawConversation) {
						if (!lastNameFilter.empty() && nameFilter.empty()) {
							conversationHistory.RefreshCurrentHistory();
						}
					}
					ImGui::SetCursorPosX(childSize.x * 0.5f - toggleButtonOffset);
					ImGui::SetCursorPosY(childSize.y * 0.25f);

					// location toggle
					auto cursorY = ImGui::GetCursorPosY();
					ImGui::SetCursorPosY(cursorY - (toggleHeight * 0.25f));
					ImGui::BeginDisabled(sortByLocation);
					ImGui::TextUnformatted("$DH_Date_Text"_T);
					ImGui::EndDisabled();
					ImGui::SameLine();

					ImGui::SetCursorPosY(cursorY);
					if (ImGui::ToggleButton("##MapToggle", &sortByLocation)) {
						if (drawConversation) {
							conversationHistory.ClearCurrentHistory();
						}
						OnTreeSwitch();
					}

					ImGui::SameLine();
					ImGui::SetCursorPosY(cursorY - (toggleHeight * 0.25f));
					ImGui::BeginDisabled(!sortByLocation);
					ImGui::TextUnformatted("$DH_Location_Text"_T);
					ImGui::EndDisabled();
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();

			ImGui::PopFont();

			if (!hideButton) {
				auto [buttonFont, buttonFontSize] = MANAGER(IconFont)->GetButtonFont();
				ImGui::PushFont(buttonFont, buttonFontSize);
				{
					const auto icon = MANAGER(Hotkeys)->EscapeIcon();

					// exit button position (1784,1015) + offset (32) at 1080p
					static const auto windowSize = RE::BSGraphics::Renderer::GetScreenSize();
					static double     posY = 0.93981481481 * windowSize.height;
					static double     posX = 0.92916666666 * windowSize.width;

					ImGui::SetCursorScreenPos(ImVec2{ (float)posX, (float)posY });
					ImGui::ButtonIconWithLabel("$DH_Exit_Button"_T, icon);
					if (ImGui::IsItemSelected()) {
						SetGlobalHistoryOpen(false);
					}
				}
				ImGui::PopFont();
			}
		}
		ImGui::End();

		if (ImGui::IsKeyReleased(ImGuiKey_Escape) || ImGui::IsKeyReleased(ImGuiKey_GamepadFaceRight)) {
			SetGlobalHistoryOpen(false);
		}
	}

	bool Manager::IsGlobalHistoryOpen() const
	{
		return globalHistoryOpen;
	}

	void Manager::SetGlobalHistoryOpen(bool a_open, bool a_showCursor)
	{
		if (globalHistoryOpen == a_open) {
			return;
		}

		globalHistoryOpen = a_open;
		menuOpenedJustNow = a_open;
		autoSelectFirstEntry = a_open;

		if (a_open) {
			if (blurMenu) {
				RE::UIBlurManager::GetSingleton()->IncrementBlurCount();
			}

			// hides compass but not notifications
			RE::SendHUDMessage::PushHUDMode("WorldMapMode");
			if (a_showCursor) {
				RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
			}

			conversationHistory.RefreshHistoryMaps();

			RE::PlaySound("UIMenuOK");

		} else {
			dialogueHistory.ClearCurrentHistory();
			conversationHistory.ClearCurrentHistory();

			dialogueHistory.ClearFilters();
			conversationHistory.ClearFilters();

			nameFilter.clear();
			lastNameFilter.clear();

			voiceHandle.Stop();

			hoveredLine = nullptr;

			if (blurMenu) {
				RE::UIBlurManager::GetSingleton()->DecrementBlurCount();
			}

			RE::SendHUDMessage::PopHUDMode("WorldMapMode");
			RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

			RE::PlaySound("UIMenuCancel");
		}

		if (!unpauseMenu) {
			RE::Main::GetSingleton()->freezeTime = a_open;
		}

		ImGui::Renderer::RenderMenus(a_open);
	}

	void Manager::ToggleActive()
	{
		if (!IsGlobalHistoryOpen() && !IsValid()) {
			return;
		}

		SetGlobalHistoryOpen(!IsGlobalHistoryOpen());
	}

	bool Manager::WasMenuOpenJustNow() const
	{
		return menuOpenedJustNow;
	}

	void Manager::SetMenuOpenJustNow(bool a_open)
	{
		menuOpenedJustNow = a_open;
	}

	bool Manager::ShouldAutoSelectFirstEntry() const
	{
		return autoSelectFirstEntry;
	}

	void Manager::SetAutoSelectFirstEntry(bool a_select)
	{
		autoSelectFirstEntry = a_select;
	}

	bool Manager::Use12HourFormat() const
	{
		return use12HourFormat;
	}

	void Manager::SaveDialogueHistory(const std::tm& a_time, Dialogue a_dialogue)
	{
		dialogueHistory.SaveHistory(a_time, std::move(a_dialogue), use12HourFormat);
	}

	void Manager::AddConversation(const RE::TESObjectREFRPtr& a_speaker, RE::TESTopicInfo* a_info)
	{
		if (!a_speaker || !a_info) {
			return;
		}

		if (a_speaker->IsPlayerRef() || a_speaker->GetDistance(RE::PlayerCharacter::GetSingleton()) > *"fTalkingDistance:LOD"_ini || RE::MenuTopicManager::GetSingleton()->speaker.get() == a_speaker) {
			return;
		}

		auto dialogueItem = a_info->GetDialogueData(a_speaker.get());
		if (auto currentResponse = !dialogueItem.responses.empty() ? dialogueItem.responses.front() : nullptr) {
			if (!currentResponse->text.empty() && currentResponse->text != " ") {
				auto time = RE::Calendar::GetSingleton()->GetTime();

				std::string text = currentResponse->text.c_str();
				std::string voice = currentResponse->voice.c_str();
				if (!voice.empty()) {
					// Strip "Data\"
					voice.erase(0, 5);
				}

				conversationHistory.SaveHistory(time, Monologue(time, a_speaker.get(), std::move(text), std::move(voice), dialogueItem.topic));
			}
		}
	}

	void Manager::OnTreeSwitch()
	{
		const bool hasSelection = drawConversation ?
		                              conversationHistory.CanDrawHistory() :
		                              dialogueHistory.CanDrawHistory();
		if (!hasSelection) {
			SetMenuOpenJustNow(true);
			SetAutoSelectFirstEntry(true);
		}
	}

	EventResult Manager::ProcessEvent(const RE::TESLoadGameEvent* a_evn, RE::BSTEventSource<RE::TESLoadGameEvent>*)
	{
		if (a_evn && finishLoading) {
			RefreshPlayerName();
			dialogueHistory.InitHistory();
			conversationHistory.InitHistory();
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(const RE::TESTopicInfoEvent* a_evn, RE::BSTEventSource<RE::TESTopicInfoEvent>*)
	{
		if (a_evn && a_evn->type == RE::TESTopicInfoEvent::TopicInfoEventType::kTopicEnd) {
			AddConversation(a_evn->speakerRef, RE::TESForm::LookupByID<RE::TESTopicInfo>(a_evn->topicInfoFormID));
		}

		return EventResult::kContinue;
	}

	void Manager::SaveFiles(const std::string& a_save)
	{
		dialogueHistory.SaveHistoryToFile(a_save);
		conversationHistory.SaveHistoryToFile(a_save);
	}

	void Manager::LoadFiles(const std::string& a_save)
	{
		finishLoading |= dialogueHistory.LoadHistoryFromFile(a_save);
		finishLoading |= conversationHistory.LoadHistoryFromFile(a_save);
	}

	void Manager::DeleteSavedFiles(const std::string& a_save)
	{
		dialogueHistory.DeleteSavedFile(a_save);
		conversationHistory.DeleteSavedFile(a_save);
	}

	void Manager::CleanupSavedFiles()
	{
		constexpr auto get_save_directory = []() -> std::optional<std::filesystem::path> {
			if (auto path = SKSE::log::log_directory()) {
				path->remove_filename();  // remove "/SKSE"
				path->append("sLocalSavePath:General"_ini.value());
				return path;
			}
			return std::nullopt;
		};

		auto saveDir = get_save_directory();
		if (!saveDir) {
			return;
		}

		dialogueHistory.CleanupSavedFiles(*saveDir);
		conversationHistory.CleanupSavedFiles(*saveDir);
	}

	void Manager::Clear()
	{
		dialogueHistory.Clear();
		conversationHistory.Clear();
	}

	void Manager::PlayVoiceline(const std::string& a_voiceline)
	{
#undef GetObject

		if (a_voiceline.empty()) {
			return;
		}

		if (voiceHandle.IsPlaying()) {
			voiceHandle.FadeOutAndRelease(500);
		}

		RE::BSResource::ID file;
		file.GenerateFromPath(a_voiceline.c_str());

		RE::BSAudioManager::GetSingleton()->GetSoundHandleByFile(voiceHandle, file, 128 | 0x10, 128);

		auto soundOutput = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSSoundOutput>(RE::DEFAULT_OBJECTS::kDialogueOutputModel2D);
		if (soundOutput) {
			voiceHandle.SetOutputModel(soundOutput);
		}

		voiceHandle.Play();
	}

	const std::string& Manager::GetPlayerName() const
	{
		if (playerName.empty()) {
			playerName = RE::PlayerCharacter::GetSingleton()->GetDisplayFullName();
		}
		return playerName;
	}

	void Manager::RefreshPlayerName()
	{
		playerName = RE::PlayerCharacter::GetSingleton()->GetDisplayFullName();
	}

	bool Manager::IsLineHovered(const void* a_line) const
	{
		return hoveredLine == a_line;
	}

	void Manager::SetLineHovered(const void* a_line, bool a_hovered)
	{
		if (a_hovered) {
			hoveredLine = a_line;
		} else if (hoveredLine == a_line) {
			hoveredLine = nullptr;
		}
	}
}
