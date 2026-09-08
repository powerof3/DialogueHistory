#include "MenuIntegration.h"

#include "Input.h"
#include "LocalHistory.h"

namespace MenuIntegration
{
	void Manager::Register()
	{
		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(this);

		if (REX::W32::GetModuleHandleA("TweenMenuOverhaul.dll") != nullptr) {
			SKSE::GetModCallbackEventSource()->AddEventSink(this);
			REX::INFO("Registered for mod callback event");
		}
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		globalHistory.openFromPause = a_ini.GetBoolValue("Settings", "bOpenFromPauseMenu", globalHistory.openFromPause);
	}

	void Manager::UpdateListVisuals(RE::GFxMovieView* a_view, RE::GFxValue& a_listObj, std::uint32_t a_numItems)
	{
		RE::GFxValue maxShownV;
		if (!a_listObj.GetMember("iMaxItemsShown", &maxShownV) || !maxShownV.IsNumber()) {
			return;
		}
		const auto curClips = static_cast<std::uint32_t>(maxShownV.GetNumber());
		if (curClips == 0 || a_numItems <= curClips) {
			return;
		}
		for (std::uint32_t i = curClips; i < a_numItems; ++i) {
			RE::GFxValue src;
			if (!a_listObj.GetMember(std::format("Entry{}", i - 1).c_str(), &src)) {
				break;
			}
			const auto         newName = std::format("Entry{}", i);
			const RE::GFxValue dupArgs[2] = { RE::GFxValue(newName.c_str()), RE::GFxValue(static_cast<double>(20000 + i)) };
			src.Invoke("duplicateMovieClip", nullptr, dupArgs, 2);

			RE::GFxValue clip;
			if (a_listObj.GetMember(newName.c_str(), &clip)) {
				clip.SetMember("clipIndex", RE::GFxValue(static_cast<double>(i)));
				RE::GFxValue fnPress, fnRoll;
				a_view->CreateFunction(&fnPress, &entryPressHandler);
				a_view->CreateFunction(&fnRoll, &entryRollOverHandler);
				clip.SetMember("onPress", fnPress);
				clip.SetMember("onRollOver", fnRoll);
			}
		}
		a_listObj.SetMember("iMaxItemsShown", RE::GFxValue(static_cast<double>(a_numItems)));
	}

	void Manager::SetupJournalMenu()
	{
		const auto menu = RE::UI::GetSingleton()->GetMenu<RE::JournalMenu>();
		const auto view = menu ? menu->systemTab.view : nullptr;

		RE::GFxValue page;
		if (!view || !view->GetVariable(&page, "_root.QuestJournalFader.Menu_mc.SystemFader.Page_mc")) {
			return;
		}

		if (page.HasMember("UpdateIndices")) {
			if (globalHistory.openFromPause) {
				globalHistory.pressHandler.InjectJournalEntry(view.get(), page);
			}
		}
	}

	EventResult Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		if (a_evn->menuName == RE::JournalMenu::MENU_NAME) {
			if (a_evn->opening && globalHistory.openFromPause) {
					SetupJournalMenu();
			}
			if (MANAGER(LocalHistory)->IsLocalHistoryOpen()) {
				MANAGER(LocalHistory)->SetupLocalHistoryMenu(!a_evn->opening, false);
			}
		} else if (a_evn->menuName == RE::DialogueMenu::MENU_NAME) {
			MANAGER(LocalHistory)->SetDialogueMenuOpen(a_evn->opening);
		} else if (a_evn->menuName == RE::RaceSexMenu::MENU_NAME) {
			if (!a_evn->opening) {
				MANAGER(GlobalHistory)->RefreshPlayerName();
			}
		} else if (a_evn->opening) {
			switch (REX::STR::CONST_HASH(a_evn->menuName)) {
			case REX::STR::CONST_HASH(RE::MainMenu::MENU_NAME):
			case REX::STR::CONST_HASH(RE::LoadingMenu::MENU_NAME):
			case "CustomMenu"_h:
				{
					if (MANAGER(LocalHistory)->IsDialogueMenuOpen()) {
						MANAGER(LocalHistory)->SetDialogueMenuOpen(false);
					}
				}
				break;
			default:
				break;
			}
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(const SKSE::ModCallbackEvent* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		globalHistory.SetTweenMenuOpen(a_evn->eventName);

		return EventResult::kContinue;
	}
}
