#pragma once

namespace Input
{
	enum class DEVICE
	{
		kKeyboard,
		kMouse,
		kGamepadDirectX,  // xbox
		kGamepadOrbis     // ps4
	};

	class Manager :
		public REX::Singleton<Manager>
	{
	public:
		static void Register();

		DEVICE GetInputDevice() const;
		void   ProcessInputEvents(RE::InputEvent* const* a_events);

	private:
		static ImGuiKey ToImGuiKey(KEY a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_DIRECTX a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_ORBIS a_key);

		// members
		DEVICE inputDevice{ DEVICE::kKeyboard };
	};
}
