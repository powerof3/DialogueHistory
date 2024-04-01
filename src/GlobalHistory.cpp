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
	void Manager::Register()
	{
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(GetSingleton());
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		use12HourFormat = a_ini.GetBoolValue("Settings", "b12HourFormat", use12HourFormat);
		unpauseMenu = a_ini.GetBoolValue("Settings", "bUnpauseGlobalHistory", unpauseMenu);
		blurMenu = a_ini.GetBoolValue("Settings", "bBlurGlobalHistory", blurMenu);

		if (!dialoguesByDate.empty()) {
			RefreshTimeStamps();
		}
	}

	bool Manager::IsValid()
	{
		if (IsGlobalHistoryOpen()) {
			return true;
		}

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
			!controlMap || controlMap->contextPriorityStack.back() != RE::UserEvents::INPUT_CONTEXT_ID::kGameplay) {
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

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
		{
			constexpr auto windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

			ImGui::SetNextWindowPos(ImGui::GetNativeViewportCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

			ImGui::PushFont(MANAGER(IconFont)->GetGlobalHistoryFont());

			ImGui::BeginChild("##GlobalHistory", ImGui::GetNativeViewportSize() * 0.8f, ImGuiChildFlags_Border, windowFlags);
			{
				ImGui::ExtendWindowPastBorder();

				ImGui::PushFont(MANAGER(IconFont)->GetHeaderFont());
				{
					ImGui::Indent();
					ImGui::TextUnformatted("$DH_Title"_T);
					ImGui::Unindent();
				}
				ImGui::PopFont();

				ImGui::Spacing(2);
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
				ImGui::Spacing(2);

				auto childSize = ImGui::GetContentRegionAvail();

				float toggleHeight = ImGui::GetFrameHeight() / 1.5f;
				float toggleButtonOffset = ImGui::CalcTextSize("$DH_Date_Text"_T).x + ImGui::GetStyle().ItemSpacing.x + toggleHeight * 0.5f;

				ImGui::BeginGroup();
				{
					auto startPos = childSize.x * 0.25f;                                                        // search box end
					auto endPos = (childSize.x * 0.5f) - toggleButtonOffset - ImGui::GetStyle().ItemSpacing.x;  // "By Date" text start

					ImGui::BeginChild("##Map", { (startPos + endPos) * 0.5f, childSize.y * 0.9125f }, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
					{
						if (sortByLocation) {
							DrawDialogueTree(dialoguesByLocation);
						} else {
							DrawDialogueTree(dialoguesByDate);
						}
					}
					ImGui::EndChild();

					ImGui::SameLine();
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
					ImGui::SameLine();

					childSize = ImGui::GetContentRegionAvail();

					if (currentDialogue) {
						ImGui::BeginChild("##History", ImVec2(0, childSize.y * 0.9125f), ImGuiChildFlags_None, windowFlags | ImGuiWindowFlags_NoBackground);
						{
							currentDialogue->Draw();
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

					ImGui::SameLine();
					ImGui::SetCursorPosY(childSize.y * 0.125f);
					ImGui::SetNextItemWidth(childSize.x * 0.25f);
					if (ImGui::InputTextWithHint("##Name", "$DH_Name_Text"_T, &nameFilter)) {
						currentDialogue = std::nullopt;
					}

					ImGui::SetCursorPosX(childSize.x * 0.5f - toggleButtonOffset);
					ImGui::SetCursorPosY(childSize.y * 0.25f);

					ImGui::BeginGroup();
					{
						auto cursorY = ImGui::GetCursorPosY();

						ImGui::SetCursorPosY(cursorY - (toggleHeight * 0.25f));
						ImGui::BeginDisabled(sortByLocation);
						ImGui::TextUnformatted("$DH_Date_Text"_T);
						ImGui::EndDisabled();
						ImGui::SameLine();

						ImGui::SetCursorPosY(cursorY);
						if (ImGui::ToggleButton("##MapToggle", &sortByLocation)) {
							currentDialogue = std::nullopt;
						}

						ImGui::SameLine();
						ImGui::SetCursorPosY(cursorY - (toggleHeight * 0.25f));
						ImGui::BeginDisabled(!sortByLocation);
						ImGui::TextUnformatted("$DH_Location_Text"_T);
						ImGui::EndDisabled();
					}
					ImGui::EndGroup();
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();

			ImGui::PopFont();

			ImGui::PushFont(MANAGER(IconFont)->GetButtonFont());
			{
				const auto icon = MANAGER(Hotkeys)->EscapeIcon();

				// exit button position (1784,1015) + offset (32) at 1080p
				static const auto windowSize = RE::BSGraphics::Renderer::GetScreenSize();
				static float      posY = 0.93981481481f * windowSize.height;
				static float      posX = 0.92916666666f * windowSize.width;

				ImGui::SetCursorScreenPos({ posX, posY });
				ImGui::ButtonIconWithLabel("$DH_Exit_Button"_T, icon);
				if (ImGui::IsItemClicked()) {
					SetGlobalHistoryOpen(false);
				}
			}
			ImGui::PopFont();
		}
		ImGui::End();

		if (ImGui::IsKeyReleased(ImGuiKey_Escape) || ImGui::IsKeyReleased(ImGuiKey_NavGamepadCancel)) {
			SetGlobalHistoryOpen(false);
		}
	}

	bool Manager::IsGlobalHistoryOpen() const
	{
		return globalHistoryOpen;
	}

	void Manager::SetGlobalHistoryOpen(bool a_open)
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
			RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);

			RE::PlaySound("UIMenuOK");

		} else {
			currentDialogue = std::nullopt;

			nameFilter.clear();
			dialoguesByDate.clear_filter();
			dialoguesByLocation.clear_filter();

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
		if (!IsValid()) {
			return;
		}

		SetGlobalHistoryOpen(!IsGlobalHistoryOpen());
	}

	bool Manager::Use12HourFormat() const
	{
		return use12HourFormat;
	}

	void Manager::SaveDialogueHistory(const std::tm& a_time, const Dialogue& a_dialogue)
	{
		dialogues.push_back(a_dialogue);

		TimeStamp date;
		date.FromYearMonthDay(a_time.tm_year, a_time.tm_mon, a_time.tm_mday);

		TimeStamp hourMin;
		hourMin.FromHourMin(a_time.tm_hour, a_time.tm_min, a_dialogue.speakerName, use12HourFormat);

		dialoguesByDate.map[date][hourMin] = a_dialogue;

		TimeStamp speaker(a_dialogue.timeStamp, a_dialogue.speakerName);
		dialoguesByLocation.map[a_dialogue.locName][speaker] = a_dialogue;
	}

	void Manager::RefreshTimeStamps()
	{
		for (auto& [dayMonth, hourMinMap] : dialoguesByDate.map) {
			for (auto it = hourMinMap.begin(); it != hourMinMap.end(); it++) {
				it->second.timeAndLoc.clear();

				auto node = hourMinMap.extract(it);
				node.key().SwitchHourFormat(use12HourFormat);
				hourMinMap.insert(std::move(node));
			}
		}
	}

	// My Documents/My Games/Skyrim Special Edition/Saves/Dialogue History
	std::optional<std::filesystem::path> Manager::GetSaveDirectory()
	{
		if (!saveDirectory) {
			logger::info("{:*^30}", "LOAD GAME");
			
			try {
				wchar_t*                                               buffer{ nullptr };
				const auto                                             result = ::SHGetKnownFolderPath(::FOLDERID_Documents, ::KNOWN_FOLDER_FLAG::KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
				std::unique_ptr<wchar_t[], decltype(&::CoTaskMemFree)> knownPath(buffer, ::CoTaskMemFree);
				if (!knownPath || result != S_OK) {
					logger::error("failed to get known folder path"sv);
					return std::nullopt;
				}

				std::filesystem::path path = knownPath.get();
				path /= "My Games"sv;
				path /= std::filesystem::exists("steam_api64.dll") ? "Skyrim Special Edition" : "Skyrim Special Edition GOG";
				path /= "Saves"sv;
				path /= "DialogueHistory"sv;

				if (!std::filesystem::exists(path)) {
					std::filesystem::create_directory(path);
				}

				saveDirectory = path;

				logger::info("Save directory : {}", path.string());
			}
			catch (std::filesystem::filesystem_error& e) {
				logger::error("error: {}", e.what());
			}
		}

		return saveDirectory;
	}

	std::optional<std::filesystem::path> Manager::GetDialogueHistoryFile(const std::string& a_save)
	{
		auto jsonPath = GetSaveDirectory();

		if (!jsonPath) {
			return {};
		}

		*jsonPath /= a_save;
		jsonPath->replace_extension(".json");

		logger::info("Loading save file : {}", jsonPath->string());

		return jsonPath;
	}

	EventResult Manager::ProcessEvent(const RE::TESLoadGameEvent* a_evn, RE::BSTEventSource<RE::TESLoadGameEvent>*)
	{
		if (a_evn && finishLoading) {
			FinishLoadFromFile();
		}

		return EventResult::kContinue;
	}

	void Manager::SaveToFile(const std::string& a_save)
	{
		const auto& jsonPath = GetDialogueHistoryFile(a_save);
		if (!jsonPath) {
			return;
		}

		auto ec = glz::write_file_json(dialogues, jsonPath->string(), std::string());
		if (ec) {
			logger::info("Failed to save dialogue history");
		}
	}

	void Manager::LoadFromFile(const std::string& a_save)
	{
		const auto& jsonPath = GetDialogueHistoryFile(a_save);
		if (!jsonPath) {
			return;
		}

		Clear();

		if (std::filesystem::exists(*jsonPath)) {
			glz::read_file_json(dialogues, jsonPath->string(), std::string());
		}

		finishLoading = true;
	}

	void Manager::FinishLoadFromFile()
	{
		if (!finishLoading) {
			return;
		}

		std::string playerName = RE::PlayerCharacter::GetSingleton()->GetDisplayFullName();

		if (!dialogues.empty()) {
			std::erase_if(dialogues, [&](auto& dialogue) {
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

				for (auto& [line, voice, name, isPlayer, hovered] : dialogue.dialogue) {
					isPlayer = voice.empty();
					name = !isPlayer ? dialogue.speakerName : playerName;
					if (line.empty() || line == " ") {
						line = "...";
					}
					hovered = false;
				}

				std::uint32_t year, month, day, hour, min;
				dialogue.ExtractTimeStamp(year, month, day, hour, min);

				TimeStamp date;
				date.FromYearMonthDay(year, month, day);

				TimeStamp hourMin;
				hourMin.FromHourMin(hour, min, dialogue.speakerName, use12HourFormat);

				TimeStamp speaker(dialogue.timeStamp, dialogue.speakerName);

				dialoguesByDate.map[date][hourMin] = dialogue;
				dialoguesByLocation.map[dialogue.locName][speaker] = dialogue;

				return false;
			});
		}

		finishLoading = true;
	}

	void Manager::DeleteSavedFile(const std::string& a_save)
	{
		auto jsonPath = GetDialogueHistoryFile(a_save);
		if (!jsonPath) {
			return;
		}

		std::filesystem::remove(*jsonPath);
	}

	void Manager::Clear()
	{
		dialogues.clear();
		dialoguesByDate.clear();
		dialoguesByLocation.clear();
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
