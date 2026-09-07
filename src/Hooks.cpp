#include "Hooks.h"
#include "GlobalHistory.h"
#include "Input.h"

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

	struct StopTweenCamera
	{
		static void thunk(RE::PlayerCamera* a_this)
		{
			func(a_this);

			MANAGER(GlobalHistory)->TryOpenFromTweenMenu();
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct CursorMenu_ProcessMessage
	{
		static RE::UI_MESSAGE_RESULTS thunk(RE::CursorMenu* a_this, RE::UIMessage& a_message)
		{
			if (a_message.type == RE::UI_MESSAGE_TYPE::kHide) {
				if (MANAGER(GlobalHistory)->TryOpenFromTweenMenu(false) || MANAGER(GlobalHistory)->IsGlobalHistoryOpen()) {
					return RE::UI_MESSAGE_RESULTS::kIgnore;
				}
			}

			return func(a_this, a_message);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            idx{ 0x04 };
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> inputUnk(RELOCATION_ID(67315, 68617), 0x7B);
		stl::write_thunk_call<ProcessInputQueue>(inputUnk.address());

		REL::Relocation<std::uintptr_t> topicClicked(RELOCATION_ID(50615, 51509), 0x5A);
		stl::write_thunk_call<UpdateSelectedResponse>(topicClicked.address());

		REL::Relocation<std::uintptr_t> showSub_0(RELOCATION_ID(19119, 19521), 0x2B2);
		stl::write_thunk_call<ShowSubtitle<0>>(showSub_0.address());
		REL::Relocation<std::uintptr_t> showSub_1(RELOCATION_ID(36543, 37544), OFFSET(0x8EC, 0x8C2));
		stl::write_thunk_call<ShowSubtitle<1>>(showSub_1.address());

		REL::Relocation<std::uintptr_t> pushhud_0(RELOCATION_ID(50610, 51504), 0xA2);
		stl::write_thunk_call<PushHUDMode<0>>(pushhud_0.address());
		REL::Relocation<std::uintptr_t> pushhud_1(RELOCATION_ID(50612, 51506), OFFSET(0x2AF, 0x3AE));
		stl::write_thunk_call<PushHUDMode<1>>(pushhud_1.address());
	
		REL::Relocation<std::uintptr_t> pophud_0(RELOCATION_ID(50617, 51511), 0xA5);
		stl::write_thunk_call<PopHUDMode<0>>(pophud_0.address());
		
		REL::Relocation<std::uintptr_t> pophud_1(RELOCATION_ID(50612, 51506));
		stl::write_thunk_call<PopHUDMode<1>>(pophud_1.address() + OFFSET(0x2A8, 0x3A7));
		stl::write_thunk_call<PopHUDMode<2>>(pophud_1.address() + OFFSET(0xDE, 0xE1));

		stl::write_vfunc<RE::DialogueMenu, ProcessMessage>();

		REL::Relocation<std::uintptr_t> hudMenuUserEvent(RELOCATION_ID(50748, 51643), 0x1E);
		stl::write_thunk_call<IsMenuOpen>(hudMenuUserEvent.address());

		REL::Relocation<std::uintptr_t> take_ss{ RELOCATION_ID(35556, 36555), OFFSET(0x48E, 0x454) };  // Main::Swap
		stl::write_thunk_call<TakeScreenshot>(take_ss.address());

		if (GetModuleHandle(L"TweenMenuOverhaul") != nullptr) {
			if (GetModuleHandle(L"SkyrimSoulsRE.dll") == nullptr) {
				REL::Relocation<std::uintptr_t> tweenCameraUpdate{ RELOCATION_ID(49985, 50925), OFFSET(0xC8, 0x1C7) };  // TweenMenuCameraState::Update
				stl::write_thunk_call<StopTweenCamera>(tweenCameraUpdate.address());
			} else {
				stl::write_vfunc<RE::CursorMenu, CursorMenu_ProcessMessage>();
			}
		}

		REX::INFO("Installed dialogue hooks");
	}
}
