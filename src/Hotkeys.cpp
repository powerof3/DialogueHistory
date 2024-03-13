#include "Hotkeys.h"

#include "GlobalHistory.h"
#include "ImGui/IconsFonts.h"
#include "Input.h"
#include "LocalHistory.h"

namespace Hotkeys
{
	void Manager::LoadHotKeys(const CSimpleIniA& a_ini)
	{
		localHistory.LoadKeys(a_ini);
		globalHistory.LoadKeys(a_ini);
	}

	bool Manager::TryToggleDialogueHistory(const RE::InputEvent* const* a_event)
	{
		if (MANAGER(LocalHistory)->IsDialogueMenuOpen()) {
			return localHistory.ProcessKeyPress(a_event, []() {
				MANAGER(LocalHistory)->ToggleActive();
			});
		} else if (MANAGER(GlobalHistory)->IsValid()) {
			return globalHistory.ProcessKeyPress(a_event, []() {
				MANAGER(GlobalHistory)->ToggleActive();
			});
		}
		return false;
	}

	void Manager::KeyCombo::KeyComboImpl::LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting)
	{
		primary = a_ini.GetLongValue("Controls", a_setting.data(), primary);
		modifier = a_ini.GetLongValue("Controls", std::format("{}Modifier", a_setting).c_str(), modifier);

		keys.clear();
		if (primary != -1) {
			keys.insert(primary);
		}
		if (modifier != -1) {
			keys.insert(modifier);
		}
	}

	Manager::KeyCombo::KeyCombo(const std::string& a_type) :
		type(a_type)
	{}

	void Manager::KeyCombo::LoadKeys(const CSimpleIniA& a_ini)
	{
		keyboard.LoadKeys(a_ini, std::format("i{}Key", type).c_str());
		gamePad.LoadKeys(a_ini, std::format("i{}GamePad", type).c_str());
	}

	bool Manager::KeyCombo::IsInvalid() const
	{
		return keyboard.keys.empty() && gamePad.keys.empty();
	}

	std::set<std::uint32_t> Manager::KeyCombo::GetKeys() const
	{
		switch (MANAGER(Input)->GetInputDevice()) {
		case Input::DEVICE::kGamepadOrbis:
		case Input::DEVICE::kGamepadDirectX:
			return gamePad.keys;
		default:
			return keyboard.keys;
		}
	}

	bool Manager::KeyCombo::ProcessKeyPress(const RE::InputEvent* const* a_event, std::function<void()> a_callback)
	{
		std::set<std::uint32_t> pressed;

		for (auto event = *a_event; event; event = event->next) {
			const auto button = event->AsButtonEvent();
			if (!button || !button->HasIDCode()) {
				continue;
			}
			if (button->IsPressed()) {
				auto key = button->GetIDCode();
				switch (button->GetDevice()) {
				case RE::INPUT_DEVICE::kKeyboard:
					break;
				case RE::INPUT_DEVICE::kMouse:
					key += SKSE::InputMap::kMacro_MouseButtonOffset;
					break;
				case RE::INPUT_DEVICE::kGamepad:
					key = SKSE::InputMap::GamepadMaskToKeycode(key);
					break;
				default:
					continue;
				}
				pressed.insert(key);
			}
		}

		if (!pressed.empty() && (pressed == keyboard.keys || pressed == gamePad.keys)) {
			if (!triggered) {
				triggered = true;
				a_callback();
			}
		} else {
			triggered = false;
		}

		return triggered;
	}

	std::set<const IconFont::IconTexture*> Manager::LocalHistoryIcons() const
	{
		return MANAGER(IconFont)->GetIcons(localHistory.GetKeys());
	}

	std::uint32_t Manager::EscapeKey()
	{
		switch (MANAGER(Input)->GetInputDevice()) {
		case Input::DEVICE::kGamepadOrbis:
		case Input::DEVICE::kGamepadDirectX:
			return SKSE::InputMap::kGamepadButtonOffset_B;
		default:
			return KEY::kEscape;
		}
	}

	const IconFont::IconTexture* Manager::EscapeIcon() const
	{
		return MANAGER(IconFont)->GetIcon(EscapeKey());
	}

	std::set<const IconFont::IconTexture*> Manager::GlobalHistoryIcons() const
	{
		return MANAGER(IconFont)->GetIcons(globalHistory.GetKeys());
	}
}
