#include "LocalHistory.h"

#include "GlobalHistory.h"
#include "Hotkeys.h"
#include "ImGui/Renderer.h"
#include "ImGui/Styles.h"

namespace LocalHistory
{
	void Manager::Register()
	{
		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(GetSingleton());
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		unpauseMenu = a_ini.GetBoolValue("Settings", "bUnpauseLocalHistory", unpauseMenu);
		blurMenu = a_ini.GetBoolValue("Settings", "bBlurLocalHistory", blurMenu);
		hideButton = a_ini.GetBoolValue("Settings", "bHideButtonLocalHistory", hideButton);
	}

	void Manager::Draw()
	{
		if (!ShouldDraw()) {
			return;
		}

		ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
		ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

		auto localHistoryOpen = IsLocalHistoryOpen();

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
		{
			auto localHistoryFont = MANAGER(IconFont)->GetLocalHistoryFont();
			ImGui::PushFont(localHistoryFont, localHistoryFont->LegacySize);

			if (localHistoryOpen) {
				ImGui::SetNextWindowPos(ImGui::GetNativeViewportCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
				ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize() * 0.8f);

				ImGui::Begin("##LocalHistory", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
				{
					ImGui::ExtendWindowPastBorder();

					auto headerFont = MANAGER(IconFont)->GetHeaderFont();
					ImGui::PushFont(headerFont, headerFont->LegacySize);
					{
						ImGui::CenteredText(gameTimeString.c_str(), false);
					}
					ImGui::PopFont();

					ImGui::Spacing(2);
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
					ImGui::Spacing(2);

					localDialogue.Draw();
				}
				ImGui::End();
			}

			ImGui::PopFont();

			if (!hideButton) {
				auto buttonFont = MANAGER(IconFont)->GetButtonFont();
				ImGui::PushFont(buttonFont, buttonFont->LegacySize);
				{
					const auto& icons = MANAGER(Hotkeys)->LocalHistoryIcons();

					// exit button position (1784,1015) + offset (32) at 1080p
					static const auto windowSize = RE::BSGraphics::Renderer::GetScreenSize();
					static float      posY = 0.93981481481f * windowSize.height;
					float             posX;
					if (!localHistoryOpen) {
						// calculate position backwards
						static float exitButtonPos = 0.9125f * windowSize.width;
						static float textSize = ImGui::CalcTextSize("$DH_Title"_T).x;
						static float innerSpacing = ImGui::GetStyle().ItemInnerSpacing.x * 0.40f;

						posX = exitButtonPos;
						posX -= textSize;
						posX -= innerSpacing;
						for (auto& icon : icons) {
							posX -= icon->size.x;
						}

					} else {
						posX = 0.92916666666f * windowSize.width;
						if (icons.size() > 1) {
							posX -= (*icons.begin())->size.x;
						}
					}

					ImGui::SetCursorScreenPos({ posX, posY });
					ImGui::ButtonIconWithLabel(localHistoryOpen ? "$DH_Exit_Button"_T : "$DH_Title"_T, icons);
					// no dialogue menu click because the real menu registers as a click too
					if (ImGui::IsItemSelected() && localHistoryOpen) {
						SetLocalHistoryOpen(false);
					}
				}
				ImGui::PopFont();
			}
		}
		ImGui::End();
	}

	bool Manager::ShouldDraw()
	{
		return IsDialogueMenuOpen() && !ShouldHide();
	}

	bool Manager::IsDialogueMenuOpen() const
	{
		return dialogueMenuOpen;
	}

	bool Manager::ShouldHide()
	{
		static constexpr std::array badMenus{
			// if hitting esc
			RE::JournalMenu::MENU_NAME,
			// accessible via dialogue
			RE::BarterMenu::MENU_NAME,
			RE::GiftMenu::MENU_NAME,
			RE::ContainerMenu::MENU_NAME,
			RE::TrainingMenu::MENU_NAME,
			// console
			RE::Console::MENU_NAME
		};

		const auto UI = RE::UI::GetSingleton();
		if (!UI || std::ranges::any_of(badMenus, [&](const auto& menuName) { return UI->IsMenuOpen(menuName); }) || !UI->IsShowingMenus()) {
			return true;
		}

		return false;
	}

	bool Manager::IsLocalHistoryOpen() const
	{
		return localHistoryMenuOpen;
	}

	void Manager::ToggleActive()
	{
		if (!IsDialogueMenuOpen()) {
			return;
		}

		SetLocalHistoryOpen(!IsLocalHistoryOpen());
	}

	void Manager::SetDialogueMenuOpen(bool a_opened)
	{
		dialogueMenuOpen = a_opened;

		if (!dialogueMenuOpen) {
			if (ShouldHide()) {  // if dialogue menu is temporarily interrupted by inventory, console etc
				tempClosed = true;
				return;
			}
			if (IsLocalHistoryOpen()) {
				SetLocalHistoryOpen(false);
			}
			SaveDialogueHistory();
			localDialogue.Clear();
			tempClosed = false;
		} else {
			UpdateDialogue();
			ImGui::Styles::GetSingleton()->RefreshStyle();
		}

		ImGui::Renderer::RenderMenus(a_opened);
	}

	void Manager::SetLocalHistoryOpen(bool a_opened)
	{
		localHistoryMenuOpen = a_opened;
		SetupLocalHistoryMenu(localHistoryMenuOpen);
	}

	void Manager::SetupLocalHistoryMenu(bool a_opened, bool a_blurBG)
	{
		if (a_blurBG) {
			a_blurBG = blurMenu;
		}

		if (a_opened) {
			RE::PlaySound("UIMenuOK");
			if (a_blurBG) {
				RE::UIBlurManager::GetSingleton()->IncrementBlurCount();
			}
		} else {
			if (a_blurBG) {
				RE::UIBlurManager::GetSingleton()->DecrementBlurCount();
			}
			RE::PlaySound("UIMenuCancel");
		}

		if (auto dialogueMenu = RE::UI::GetSingleton()->GetMenu<RE::DialogueMenu>()) {
			if (auto& movie = dialogueMenu->uiMovie) {
				// hiding TopicListHolder causes dialogue menu to show defaults even when reset
				movie->SetVariable("_root.DialogueMenu_mc.TopicListHolder.PanelCopy_mc._visible", !a_opened);
				movie->SetVariable("_root.DialogueMenu_mc.TopicListHolder.ListPanel._visible", !a_opened);
				movie->SetVariable("_root.DialogueMenu_mc.TopicListHolder.TextCopy_mc._visible", !a_opened);
				movie->SetVariable("_root.DialogueMenu_mc.TopicListHolder.List_mc._visible", !a_opened);
				movie->SetVariable("_root.DialogueMenu_mc.TopicListHolder.ScrollIndicators._visible", !a_opened);

				movie->SetVariable("_root.DialogueMenu_mc.ExitButton._visible", !a_opened);
				movie->SetVariable("_root.DialogueMenu_mc.SubtitleText._visible", !a_opened);
				movie->SetVariable("_root.DialogueMenu_mc.SpeakerName._visible", !a_opened);
			}
		}

		if (!unpauseMenu) {
			RE::Main::GetSingleton()->freezeTime = a_opened;
		}
	}

	void Manager::AddDialogue(RE::TESObjectREFR* a_speaker, const std::string& a_response, const std::string& a_voice)
	{
		if (!a_speaker->IsPlayerRef()) {
			if (!currentSpeaker) {
				currentSpeaker = a_speaker;
			}
			if (currentSpeaker != a_speaker) {
				return;
			}
		}

		localDialogue.AddDialogue(a_speaker, a_response, a_voice);

		// erase duplicate opening lines
		if (auto& dialogue = localDialogue.dialogue; dialogue.size() == 2 &&
													 dialogue[0].line == dialogue[1].line &&
													 dialogue[0].voice == dialogue[1].voice) {
			dialogue.erase(dialogue.begin());
		}
	}

	void Manager::SaveDialogueHistory()
	{
		if (localDialogue.empty()) {
			return;
		}

		MANAGER(GlobalHistory)->SaveDialogueHistory(gameTime, localDialogue);
	}

	void Manager::UpdateDialogue()
	{
		auto calendar = RE::Calendar::GetSingleton();

		auto target = RE::MenuTopicManager::GetSingleton()->speaker.get();
		if (target) {
			currentSpeaker = target.get();
		} else {
			currentSpeaker = nullptr;
		}
		localDialogue.Initialize(currentSpeaker);

		char timeStamp[MAX_PATH]{};
		calendar->GetTimeDateString(timeStamp, MAX_PATH, false);
		gameTimeString = timeStamp;

		gameTime = calendar->GetTime();
		localDialogue.Initialize(gameTime);

		localDialogue.RefreshContents();
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		if (a_evn->menuName == RE::DialogueMenu::MENU_NAME) {
			SetDialogueMenuOpen(a_evn->opening);
		} else if (a_evn->menuName == RE::JournalMenu::MENU_NAME) {
			if (IsLocalHistoryOpen()) {
				SetupLocalHistoryMenu(!a_evn->opening, false);
			}
		} else if (a_evn->opening) {
			switch (string::const_hash(a_evn->menuName)) {
			case string::const_hash(RE::MainMenu::MENU_NAME):
			case string::const_hash(RE::LoadingMenu::MENU_NAME):
			case "CustomMenu"_h:
				{
					if (IsDialogueMenuOpen()) {
						SetDialogueMenuOpen(false);
					}
				}
				break;
			default:
				break;
			}
		}

		return EventResult::kContinue;
	}
}
