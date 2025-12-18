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
				it->second.timeAndLoc.clear();

				auto node = hourMinMap.extract(it);
				node.key().SwitchHourFormat(a_use12HourFormat);
				hourMinMap.insert(std::move(node));
			}
		}
	}

	void DialogueHistory::DrawDateTree()
	{
		DrawTreeImpl(dateMap);
	}

	void DialogueHistory::DrawLocationTree()
	{
		DrawTreeImpl(locationMap);
	}

	void DialogueHistory::SaveHistory(const std::tm& a_tm, const Dialogue& a_history, bool a_use12HourFormat)
	{
		history.push_back(a_history);

		TimeStamp date;
		date.FromYearMonthDay(a_tm.tm_year, a_tm.tm_mon, a_tm.tm_mday);

		TimeStamp hourMin;
		hourMin.FromHourMin(a_tm.tm_hour, a_tm.tm_min, a_history.speakerName, a_use12HourFormat);

		dateMap.map[date][hourMin] = a_history;

		TimeStamp speaker(a_history.timeStamp, a_history.speakerName);
		locationMap.map[a_history.locName][speaker] = a_history;
	}

	void DialogueHistory::SaveHistoryToFile(const std::string& a_save)
	{
		BaseHistory::SaveHistoryToFileImpl(history, a_save);
	}

	bool DialogueHistory::LoadHistoryFromFile(const std::string& a_save)
	{
		return BaseHistory::LoadHistoryFromFileImpl(history, a_save);
	}

	std::optional<std::filesystem::path> DialogueHistory::GetDirectory()
	{
		if (!directory) {
			directory = GetDirectoryImpl();
		}
		return directory;
	}

	void DialogueHistory::InitHistory()
	{
		std::string playerName = RE::PlayerCharacter::GetSingleton()->GetDisplayFullName();

		if (!history.empty()) {
			std::erase_if(history, [&](auto& dialogue) {
				auto speakerActor = RE::TESForm::LookupByID<RE::Actor>(dialogue.id.GetNumericID());
				if (!speakerActor) {
					return true;
				}

				if (auto cellOrLoc = RE::TESForm::LookupByID(dialogue.loc.GetNumericID())) {
					dialogue.locName = cellOrLoc->GetName();
					if (dialogue.locName.empty()) {
						dialogue.locName = "$DH_UnknownLocation"_T;
					}
				} else {
					dialogue.locName = "???";
				}

				dialogue.speakerName = NPCNameProvider::GetSingleton()->GetName(speakerActor);
				dialogue.playerName = playerName;

				for (auto& line : dialogue.dialogue) {
					line.isPlayer = line.voice.empty();
					line.name = !line.isPlayer ? dialogue.speakerName : playerName;
					if (line.line.empty() || line.line == " ") {
						line.line = "...";
					}
					line.hovered = false;
				}

				auto time = dialogue.ExtractTimeStamp();

				TimeStamp date;
				date.FromYearMonthDay(time.tm_year, time.tm_mon, time.tm_mday);

				TimeStamp hourMin;
				hourMin.FromHourMin(time.tm_hour, time.tm_min, dialogue.speakerName, MANAGER(GlobalHistory)->Use12HourFormat());

				TimeStamp speaker(dialogue.timeStamp, dialogue.speakerName);

				dateMap.map[date][hourMin] = dialogue;
				locationMap.map[dialogue.locName][speaker] = dialogue;

				return false;
			});
		}
	}

	void ConversationHistory::RefreshTimeStamps()
	{
		if (dateMap.empty()) {
			return;
		}

		for (auto& [dayMonth, monologues] : dateMap.map) {
			for (auto it = monologues.monologues.begin(); it != monologues.monologues.end(); it++) {
				it->hourMinTimeStamp.clear();
			}
		}
	}

	void ConversationHistory::DrawDateTree()
	{
		DrawTreeImpl(dateMap);
	}

	void ConversationHistory::DrawLocationTree()
	{
		DrawTreeImpl(locationMap);
	}

	void ConversationHistory::ClearCurrentHistory()
	{
		BaseHistory::ClearCurrentHistory();
		currentFixedHistory = std::nullopt;
	}

	void ConversationHistory::SetCurrentHistory(const Monologues& a_history)
	{
		BaseHistory::SetCurrentHistory(a_history);

		currentFixedHistory = a_history;
		currentFixedHistory->RefreshContents();
	}

	void ConversationHistory::RefreshCurrentHistory()
	{
		currentHistory = currentFixedHistory;
		if (currentHistory) {
			if (!nameFilter.empty()) {
				std::erase_if(currentHistory->monologues, [&](const auto& monologue) {
					return !string::icontains(monologue.speakerName, nameFilter);
				});
			}
			currentHistory->RefreshContents();
		}
	}

	void ConversationHistory::RevertCurrentHistory()
	{
		currentHistory = currentFixedHistory;
		if (currentHistory) {
			currentHistory->RefreshContents();
		}
	}

	void ConversationHistory::SaveHistory(const std::tm& a_tm, const Monologue& a_history)
	{
		history.monologues.push_back(a_history);

		if (MANAGER(GlobalHistory)->IsGlobalHistoryOpen() && CanShowDialogue(a_history.dialogueType)) {
			TimeStamp date;
			date.FromYearMonthDay(a_tm.tm_year, a_tm.tm_mon, a_tm.tm_mday);

			dateMap.map[date].monologues.push_back(a_history);
			locationMap.map[a_history.locName][date].monologues.push_back(a_history);
			if (currentFixedHistory) {
				currentFixedHistory->monologues.push_back(a_history);
				RefreshCurrentHistory();
			}
		}
	}

	void ConversationHistory::SaveHistoryToFile(const std::string& a_save)
	{
		BaseHistory::SaveHistoryToFileImpl(history.monologues, a_save);
	}

	bool ConversationHistory::LoadHistoryFromFile(const std::string& a_save)
	{
		return BaseHistory::LoadHistoryFromFileImpl(history.monologues, a_save);
	}

	std::optional<std::filesystem::path> ConversationHistory::GetDirectory()
	{
		if (!directory) {
			directory = GetDirectoryImpl();
		}
		return directory;
	}

	void ConversationHistory::InitHistory()
	{
		if (!history.empty()) {
			std::erase_if(history.monologues, [&](auto& monologue) {
				auto speakerActor = RE::TESForm::LookupByID<RE::Actor>(monologue.id.GetNumericID());
				if (!speakerActor) {
					return true;
				}

				if (auto cellOrLoc = RE::TESForm::LookupByID(monologue.loc.GetNumericID())) {
					monologue.locName = cellOrLoc->GetName();
					if (monologue.locName.empty()) {
						monologue.locName = "$DH_UnknownLocation"_T;
					}
				} else {
					monologue.locName = "???";
				}

				monologue.speakerName = NPCNameProvider::GetSingleton()->GetName(speakerActor);

				if (auto topic = RE::TESForm::LookupByID<RE::TESTopic>(monologue.topic.GetNumericID())) {
					monologue.dialogueType = topic->data.type.underlying();
				}

				auto& line = monologue.line;
				if (line.line.empty() || line.line == " ") {
					line.line = "...";
				}
				line.hovered = false;

				return false;
			});
		}
	}

	void ConversationHistory::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		showScene = a_ini.GetBoolValue("Settings", "bSceneDialogueConversationHistory", showMisc);
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
		dateMap.clear();
		locationMap.clear();
		for (auto& monologue : history.monologues) {
			auto time = monologue.ExtractTimeStamp();

			TimeStamp date;
			date.FromYearMonthDay(time.tm_year, time.tm_mon, time.tm_mday);

			if (CanShowDialogue(monologue.dialogueType)) {
				dateMap.map[date].monologues.push_back(monologue);
				locationMap.map[monologue.locName][date].monologues.push_back(monologue);
			}
		}
	}

	void Manager::Register()
	{
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESLoadGameEvent>(this);
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESTopicInfoEvent>(this);
		if (GetModuleHandle(L"TweenMenuOverhaul") != nullptr) {
			SKSE::GetModCallbackEventSource()->AddEventSink(this);
			skyrimSoulsInstalled = GetModuleHandle(L"SkyrimSoulsRE.dll") != nullptr;
		}
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

			auto globalFont = MANAGER(IconFont)->GetGlobalHistoryFont();
			ImGui::PushFont(globalFont, globalFont->LegacySize);

			ImGui::BeginChild("##GlobalHistory", ImGui::GetNativeViewportSize() * 0.8f, ImGuiChildFlags_Border, windowFlags);
			{
				ImGui::ExtendWindowPastBorder();

				ImGui::Spacing(2);

				auto headerFont = MANAGER(IconFont)->GetHeaderFont();
				ImGui::PushFont(headerFont, headerFont->LegacySize);
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
						dialogueHistory.ClearCurrentHistory();
						conversationHistory.ClearCurrentHistory();
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

					ImGui::BeginChild("##Map", { (startPos + endPos) * 0.5f, childSize.y * 0.9125f }, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
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
							conversationHistory.RevertCurrentHistory();
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
						} else {
							dialogueHistory.ClearCurrentHistory();
						}
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
				auto buttonFont = MANAGER(IconFont)->GetButtonFont();
				ImGui::PushFont(buttonFont, buttonFont->LegacySize);
				{
					const auto icon = MANAGER(Hotkeys)->EscapeIcon();

					// exit button position (1784,1015) + offset (32) at 1080p
					static const auto windowSize = RE::BSGraphics::Renderer::GetScreenSize();
					static float      posY = 0.93981481481f * windowSize.height;
					static float      posX = 0.92916666666f * windowSize.width;

					ImGui::SetCursorScreenPos({ posX, posY });
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
		globalHistoryOpen = a_open;
		menuOpenedJustNow = a_open;

		if (a_open) {
			ImGui::Styles::GetSingleton()->RefreshStyle();

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

	bool Manager::TryOpenFromTweenMenu(bool a_showCursor)
	{
		if (openFromTweenMenu) {
			openFromTweenMenu = false;
			SetGlobalHistoryOpen(true, a_showCursor);
			return true;
		}

		return false;
	}

	bool Manager::WasMenuOpenJustNow() const
	{
		return menuOpenedJustNow;
	}

	void Manager::SetMenuOpenJustNow(bool a_open)
	{
		menuOpenedJustNow = a_open;
	}

	bool Manager::Use12HourFormat() const
	{
		return use12HourFormat;
	}

	void Manager::SaveDialogueHistory(const std::tm& a_time, const Dialogue& a_dialogue)
	{
		dialogueHistory.SaveHistory(a_time, a_dialogue, use12HourFormat);
	}

	void Manager::AddConversation(const RE::TESObjectREFRPtr& a_speaker, RE::TESTopicInfo* a_info)
	{
		if (!a_speaker || !a_info) {
			return;
		}

		if (a_speaker->IsPlayerRef() || a_speaker->GetDistance(RE::PlayerCharacter::GetSingleton()) > "fTalkingDistance:LOD"_ini.value() || RE::MenuTopicManager::GetSingleton()->speaker.get() == a_speaker) {
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

				Monologue monologue(time, a_speaker.get(), text, voice, dialogueItem.topic);
				conversationHistory.SaveHistory(time, monologue);
			}
		}
	}

	EventResult Manager::ProcessEvent(const RE::TESLoadGameEvent* a_evn, RE::BSTEventSource<RE::TESLoadGameEvent>*)
	{
		if (a_evn && finishLoading) {
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

	EventResult Manager::ProcessEvent(const SKSE::ModCallbackEvent* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		if (a_evn && a_evn->eventName == "OpenTween_DialogueHistory" && !IsGlobalHistoryOpen()) {
			openFromTweenMenu = true;
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
			if (auto path = logger::log_directory()) {
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

		RE::BSAudioManager::GetSingleton()->BuildSoundDataFromFile(voiceHandle, file, 128 | 0x10, 128);

		auto soundOutput = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSSoundOutput>(RE::DEFAULT_OBJECTS::kDialogueOutputModel2D);
		if (soundOutput) {
			voiceHandle.SetOutputModel(soundOutput);
		}

		voiceHandle.Play();
	}
}
