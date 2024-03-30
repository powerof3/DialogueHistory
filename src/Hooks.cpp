#include "Hooks.h"
#include "GlobalHistory.h"
#include "Input.h"
#include "LocalHistory.h"

namespace Hooks
{
	struct ProcessInputQueue
	{
		static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_events)
		{
			if (a_events) {
				MANAGER(Input)->ProcessInputEvents(a_events);
			}

			if (MANAGER(GlobalHistory)->IsGlobalHistoryOpen()) {
				constexpr RE::InputEvent* const dummy[] = { nullptr };
				func(a_dispatcher, dummy);
			} else {
				func(a_dispatcher, a_events);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct UpdateSelectedResponse
	{
		static void thunk(RE::MenuTopicManager* a_this, bool a_unk01)
		{
			func(a_this, a_unk01);

			if (a_this && a_this->selectedResponseNode) {
				if (auto dialogue = a_this->selectedResponseNode->item) {
					MANAGER(LocalHistory)->AddDialogue(RE::PlayerCharacter::GetSingleton(), dialogue->topicText.c_str(), {});
				}
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct ShowSubtitle
	{
		static void thunk(RE::SubtitleManager* a_this, RE::TESObjectREFR* a_speaker, const char* a_subtitle, bool a_alwaysDisplay)
		{
			func(a_this, a_speaker, a_subtitle, a_alwaysDisplay);

			if (a_speaker && !a_speaker->IsPlayerRef()) {
				std::string subtitle = a_subtitle;
				std::string voice;
				if (auto topic = RE::MenuTopicManager::GetSingleton()->lastSelectedDialogue) {
					if (auto response = topic->currentResponse; response && response->item) {
						if (voice = response->item->voice; !voice.empty()) {
							// Strip "Data\"
							voice.erase(0, 5);
						}
					}
				}
				MANAGER(LocalHistory)->AddDialogue(a_speaker, (subtitle.empty() || subtitle == " ") ? "..." : a_subtitle, voice);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			std::array targets{
				std::make_pair(RELOCATION_ID(19119, 19521), 0x2B2),
				std::make_pair(RELOCATION_ID(36543, 37544), OFFSET(0x8EC, 0x8C2)),
			};
			for (auto& [id, offset] : targets) {
				REL::Relocation<std::uintptr_t> target(id, offset);
				stl::write_thunk_call<ShowSubtitle>(target.address());
			}
		}
	};

	struct PushHUDMode
	{
		static void thunk(const char* a_mode)
		{
			func(a_mode);

			MANAGER(LocalHistory)->SetDialogueMenuOpen(true);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			std::array targets{
				std::make_pair(RELOCATION_ID(50610, 51504), 0xA2),
				std::make_pair(RELOCATION_ID(50612, 51506), OFFSET(0x2AF, 0x3AE)),
			};
			for (auto& [id, offset] : targets) {
				REL::Relocation<std::uintptr_t> target(id, offset);
				stl::write_thunk_call<PushHUDMode>(target.address());
			}
		}
	};

	struct PopHUDMode
	{
		static void thunk(const char* a_mode)
		{
			func(a_mode);

			MANAGER(LocalHistory)->SetDialogueMenuOpen(false);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			std::array targets{
				std::make_pair(RELOCATION_ID(50617, 51511), 0xA5),
				std::make_pair(RELOCATION_ID(50612, 51506), OFFSET(0x2A8, 0x3A7)),
				std::make_pair(RELOCATION_ID(50612, 51506), OFFSET(0xDE, 0xE1)),
			};
			for (auto& [id, offset] : targets) {
				REL::Relocation<std::uintptr_t> target(id, offset);
				stl::write_thunk_call<PopHUDMode>(target.address());
			}
		}
	};

	struct ProcessMessage
	{
		static RE::UI_MESSAGE_RESULTS thunk(RE::DialogueMenu* a_this, RE::UIMessage& a_message)
		{
			if (a_message.type == RE::UI_MESSAGE_TYPE::kScaleformEvent && MANAGER(LocalHistory)->IsLocalHistoryOpen()) {
				return RE::UI_MESSAGE_RESULTS::kIgnore;
			}

			return func(a_this, a_message);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx = 0x4;
	};

	struct IsMenuOpen
	{
		static bool thunk(RE::UI* a_this, const RE::BSFixedString& a_menu)
		{
			auto result = func(a_this, a_menu);

			if (!result) {
				result = MANAGER(GlobalHistory)->IsGlobalHistoryOpen();
			}

			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct TakeScreenshot
	{
		static void thunk(char const* a_path, RE::BSGraphics::TextureFileFormat a_format)
		{
			func(a_path, a_format);

			if (MANAGER(GlobalHistory)->IsGlobalHistoryOpen()) {
				// reshow cursor after Debug.Notification hides it
				SKSE::GetTaskInterface()->AddUITask([] {
					RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
				});
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> inputUnk(RELOCATION_ID(67315, 68617), 0x7B);
		stl::write_thunk_call<ProcessInputQueue>(inputUnk.address());

		REL::Relocation<std::uintptr_t> topicClicked(RELOCATION_ID(50615, 51509), 0x5A);
		stl::write_thunk_call<UpdateSelectedResponse>(topicClicked.address());

		ShowSubtitle::Install();
		PushHUDMode::Install();
		PopHUDMode::Install();

		stl::write_vfunc<RE::DialogueMenu, ProcessMessage>();

		REL::Relocation<std::uintptr_t> hudMenuUserEvent(RELOCATION_ID(50748, 51643), 0x1E);
		stl::write_thunk_call<IsMenuOpen>(hudMenuUserEvent.address());

		REL::Relocation<std::uintptr_t> take_ss{ RELOCATION_ID(35556, 36555), OFFSET(0x48E, 0x454) };  // Main::Swap
		stl::write_thunk_call<TakeScreenshot>(take_ss.address());

		logger::info("Installed dialogue hooks");
	}
}
