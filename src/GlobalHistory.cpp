#include "GlobalHistory.h"

#include "Hooks.h"
#include "Hotkeys.h"
#include "ImGui/IconsFonts.h"
#include "ImGui/Renderer.h"
#include "ImGui/Styles.h"
#include "ImGui/Util.h"

namespace GlobalHistory
{
	void Manager::Register()
	{
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(GetSingleton());
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		use12HourFormat = a_ini.GetBoolValue("Settings", "b12HourFormat", use12HourFormat);

		if (!sortedDialogues.empty()) {
			RefreshTimeStamps();
		}
	}

	bool Manager::IsValid()
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

			ImGui::BeginChild("##GlobalHistory", ImGui::GetNativeViewportSize() / 1.25, ImGuiChildFlags_Border, windowFlags);
			{
				ImGui::ExtendWindowPastBorder();

				ImGui::PushFont(MANAGER(IconFont)->GetLargeFont());
				{
					ImGui::CenteredText("$DH_Title"_T, false);
				}
				ImGui::PopFont();

				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
				ImGui::Spacing();
				ImGui::Spacing();

				auto childSize = ImGui::GetContentRegionAvail();
				ImGui::BeginChild("##Map", { childSize.x / 2.75f, childSize.y }, ImGuiChildFlags_None, windowFlags | ImGuiWindowFlags_NoBackground);
				{
					ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
					{
						for (auto it = sortedDialogues.begin(); it != sortedDialogues.end(); it++) {
							auto& [date, timeMap] = *it;
							auto rootOpen = ImGui::TreeNodeEx(date.format.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
							if (ImGui::IsItemToggledOpen()) {
								currentDialogue = std::nullopt;
							}
							if (rootOpen) {
								for (auto& [hourMin, dialogue] : timeMap) {
									auto leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
									auto is_selected = currentDialogue && currentDialogue == dialogue;
									if (is_selected) {
										leafFlags |= ImGuiTreeNodeFlags_Selected;
										ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
									}
									ImGui::TreeNodeEx(hourMin.format.c_str(), leafFlags);
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
				ImGui::EndChild();

				ImGui::SameLine();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
				ImGui::SameLine();

				if (currentDialogue) {
					ImGui::BeginChild("##History", ImVec2(0, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_None, windowFlags | ImGuiWindowFlags_NoBackground);
					{
						currentDialogue->Draw();
					}
					ImGui::EndChild();
				}
			}
			ImGui::EndChild();

			ImGui::PushFont(MANAGER(IconFont)->GetLargeFont());
			{
				const auto icon = MANAGER(Hotkeys)->EscapeIcon();

				// exit button position (1784,1015) + offset (32) at 1080p
				static const auto windowSize = RE::BSGraphics::Renderer::GetScreenSize();
				static float      posY = 0.93981481481f * windowSize.height;
				static float      posX = 0.92916666666f * windowSize.width;

				ImGui::SetCursorScreenPos({ posX, posY });
				ImGui::ButtonIconWithLabel("$DH_Exit_Button"_T, icon);
			}
			ImGui::PopFont();
		}
		ImGui::End();

		if ((ImGui::IsKeyReleased(ImGuiKey_Escape) || ImGui::IsKeyReleased(ImGuiKey_NavGamepadCancel))) {
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

			RE::UIBlurManager::GetSingleton()->IncrementBlurCount();
			RE::SendHUDMessage::PushHUDMode("DialogueMode");
			RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);

			RE::PlaySound("UIMenuOK");

		} else {
			currentDialogue = std::nullopt;
			voiceHandle.Stop();

			RE::UIBlurManager::GetSingleton()->DecrementBlurCount();
			RE::SendHUDMessage::PopHUDMode("DialogueMode");
			RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

			RE::PlaySound("UIMenuCancel");
		}

		RE::Main::GetSingleton()->freezeTime = a_open;

		ImGui::Renderer::RenderMenus(a_open);
	}

	void Manager::ToggleActive()
	{
		auto open = !IsGlobalHistoryOpen();
		SetGlobalHistoryOpen(open);
	}

	void Manager::SaveDialogueHistory(RE::TESObjectREFR* a_speaker, const std::tm& a_time, const Dialogue& a_dialogue)
	{
		TimeStamp date;
		date.FromYearMonthDay(a_time.tm_year, a_time.tm_mon, a_time.tm_mday);

		TimeStamp hourMin;
		hourMin.FromHourMin(a_time.tm_hour, a_time.tm_min, a_speaker->GetDisplayFullName(), use12HourFormat);

		dialogues.push_back(a_dialogue);
		sortedDialogues[date][hourMin] = a_dialogue;
	}

	void Manager::RefreshTimeStamps()
	{
		for (auto& [dayMonth, hourMinMap] : sortedDialogues) {
			for (auto it = hourMinMap.begin(); it != hourMinMap.end(); it++) {
				auto node = hourMinMap.extract(it);
				node.key().SwitchHourFormat(use12HourFormat);
				hourMinMap.insert(std::move(node));
			}
		}
	}

	std::optional<std::filesystem::path> Manager::GetSaveDirectory()
	{
		if (!saveDirectory) {
			// My Documents/My Games/Skyrim Special Edition/Saves/Dialogue History
			auto directory = logger::log_directory();
			if (directory) {
				directory->remove_filename();
				*directory /= "Saves";
				*directory /= "DialogueHistory\\"sv;

				if (!std::filesystem::exists(*directory)) {
					std::filesystem::create_directory(*directory);
				}
			}
			saveDirectory = directory;
		}

		return saveDirectory;
	}

	std::optional<std::filesystem::path> Manager::GetDialogueHistoryFile(const std::string& a_save)
	{
		auto jsonPath = GetSaveDirectory();

		if (!jsonPath) {
			return {};
		}

		*jsonPath += a_save;
		jsonPath->replace_extension(".json");

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

		logger::info("{}", jsonPath->string());

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

		if (!dialogues.empty()) {
			std::erase_if(dialogues, [&](auto& dialogue) {
				std::string speakerName;
				bool        missingActor = false;

				for (auto& [id, line, voice, name, isPlayer, hovered] : dialogue.dialogue) {
					if (auto actor = RE::TESForm::LookupByID<RE::Actor>(id.GetNumericID())) {
						isPlayer = actor->IsPlayerRef();
						name = actor->GetDisplayFullName();
						if (speakerName.empty() && !actor->IsPlayerRef()) {
							speakerName = name;
						}
					} else {
						missingActor = true;
						break;
					}
					if (line.empty() || line == " ") {
						line = "...";
					}
					hovered = false;
				}

				if (missingActor) {
					return true;
				}

				std::uint32_t year, month, day, hour, min;
				dialogue.ExtractTimeStamp(year, month, day, hour, min);

				TimeStamp date;
				date.FromYearMonthDay(year, month, day);
				dialogue.date = date.time;

				TimeStamp hourMin;
				hourMin.FromHourMin(hour, min, speakerName, use12HourFormat);
				dialogue.hourMin = hourMin.time;

				sortedDialogues[date][hourMin] = dialogue;

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
		sortedDialogues.clear();
	}

	void Manager::PlayVoiceline(const std::string& a_voiceline)
	{
		if (a_voiceline.empty()) {
			return;
		}

		if (voiceHandle.IsPlaying()) {
			voiceHandle.FadeOutAndRelease(500);
		}

		RE::BSResource::ID file;
		file.GenerateFromPath(a_voiceline.c_str());

		RE::BSAudioManager::GetSingleton()->BuildSoundDataFromFile(voiceHandle, file, 128 | 0x10, 128);
		voiceHandle.Play();
	}
}
