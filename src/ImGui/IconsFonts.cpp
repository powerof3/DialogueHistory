#include "IconsFonts.h"

#include "ImGui/Styles.h"
#include "Input.h"

namespace IconFont
{
	IconTexture::IconTexture(std::wstring_view a_iconName) :
		ImGui::Texture(LR"(Data/Interface/ImGui_Lib/Icons/)", a_iconName)
	{}

	bool IconTexture::Load()
	{
		const bool result = ImGui::Texture::Load();

		if (result) {
			imageSize = size;
		}

		return result;
	}

	void IconTexture::Resize(float a_scale)
	{
		auto scale = a_scale / 1080;  // standard window height
		size = imageSize * (scale * RE::BSGraphics::Renderer::GetScreenSize().height);
	}

	void Font::LoadSettings(const CSimpleIniA& a_ini, const char* a_section)
	{
		name = a_ini.GetValue(a_section, "sFont", "Jost-Regular.ttf");
		name = R"(Data/Interface/ImGui_Lib/Fonts/)" + name;

		const auto resolutionScale = DisplayTweaks::GetResolutionScale();
		size = static_cast<float>(a_ini.GetLongValue(a_section, "iSize", 30.0)) * resolutionScale;

		spacing = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSpacing", -1.5));
	}

	void Font::LoadFont(const ImVector<ImWchar>& a_ranges)
	{
		const auto& io = ImGui::GetIO();

		ImFontConfig font_config;
		font_config.GlyphExtraSpacing.x = spacing;

		font = io.Fonts->AddFontFromFileTTF(name.c_str(), size, &font_config, a_ranges.Data);
	}

	void Manager::LoadSettings(CSimpleIniA& a_ini)
	{
		smallFont.LoadSettings(a_ini, "Font");
		largeFont.LoadSettings(a_ini, "LargeFont");
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		buttonScheme = static_cast<BUTTON_SCHEME>(a_ini.GetLongValue("Controls", "iButtonScheme", std::to_underlying(buttonScheme)));
	}

	void Manager::LoadIcons()
	{
		unknownKey.Load();
		upKey.Load();
		downKey.Load();
		leftKey.Load();
		rightKey.Load();

		std::for_each(keyboard.begin(), keyboard.end(), [](auto& Icon) {
			Icon.second.Load();
		});
		std::for_each(gamePad.begin(), gamePad.end(), [](auto& Icon) {
			auto& [xbox, ps4] = Icon.second;
			xbox.Load();
			ps4.Load();
		});
		std::for_each(mouse.begin(), mouse.end(), [](auto& Icon) {
			Icon.second.Load();
		});
	}

	void Manager::ResizeIcons()
	{
		float buttonScale = ImGui::GetUserStyleVar(ImGui::USER_STYLE::kButtons);

		unknownKey.Resize(buttonScale);
		upKey.Resize(buttonScale);
		downKey.Resize(buttonScale);
		leftKey.Resize(buttonScale);
		rightKey.Resize(buttonScale);

		std::for_each(keyboard.begin(), keyboard.end(), [&](auto& Icon) {
			Icon.second.Resize(buttonScale);
		});
		std::for_each(gamePad.begin(), gamePad.end(), [&](auto& Icon) {
			auto& [xbox, ps4] = Icon.second;
			xbox.Resize(buttonScale);
			ps4.Resize(buttonScale);
		});
		std::for_each(mouse.begin(), mouse.end(), [&](auto& Icon) {
			Icon.second.Resize(buttonScale);
		});
	}

	void Manager::ReloadFonts()
	{
		auto& io = ImGui::GetIO();
		io.Fonts->Clear();

		ImVector<ImWchar> ranges;

		ImFontGlyphRangesBuilder builder;
		builder.AddText(RE::BSScaleformManager::GetSingleton()->validNameChars.c_str());
		builder.BuildRanges(&ranges);

		smallFont.LoadFont(ranges);
		largeFont.LoadFont(ranges);

		io.Fonts->Build();

		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();

		io.FontDefault = smallFont.font;
	}

	ImFont* Manager::GetLargeFont() const
	{
		return largeFont.font;
	}

	ImFont* Manager::GetSmallFont() const
	{
		return smallFont.font;
	}

	const IconTexture* Manager::GetIcon(std::uint32_t key)
	{
		switch (key) {
		case KEY::kUp:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_UP:
			return &upKey;
		case KEY::kDown:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_DOWN:
			return &downKey;
		case KEY::kLeft:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_LEFT:
			return &leftKey;
		case KEY::kRight:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_RIGHT:
			return &rightKey;
		default:
			{
				if (auto device = MANAGER(Input)->GetInputDevice(); device == Input::DEVICE::kKeyboard || device == Input::DEVICE::kMouse) {
					if (key >= SKSE::InputMap::kMacro_MouseButtonOffset) {
						if (const auto it = mouse.find(key); it != mouse.end()) {
							return &it->second;
						}
					} else if (const auto it = keyboard.find(static_cast<KEY>(key)); it != keyboard.end()) {
						return &it->second;
					}
				} else {
					if (const auto it = gamePad.find(key); it != gamePad.end()) {
						return GetGamePadIcon(it->second);
					}
				}
				return &unknownKey;
			}
		}
	}

	std::set<const IconTexture*> Manager::GetIcons(const std::set<std::uint32_t>& keys)
	{
		std::set<const IconTexture*> icons{};
		if (keys.empty()) {
			icons.insert(&unknownKey);
		} else {
			for (auto& key : keys) {
				icons.insert(GetIcon(key));
			}
		}
		return icons;
	}

	const IconTexture* Manager::GetGamePadIcon(const GamepadIcon& a_icons) const
	{
		switch (buttonScheme) {
		case BUTTON_SCHEME::kAutoDetect:
			return MANAGER(Input)->GetInputDevice() == Input::DEVICE::kGamepadOrbis ? &a_icons.ps4 : &a_icons.xbox;
		case BUTTON_SCHEME::kXbox:
			return &a_icons.xbox;
		case BUTTON_SCHEME::kPS4:
			return &a_icons.ps4;
		default:
			return &a_icons.xbox;
		}
	}
}

namespace ImGui
{
	ImVec2 ButtonIcon(std::uint32_t a_key)
	{
		return ButtonIcon(MANAGER(IconFont)->GetIcon(a_key));
	}

	ImVec2 ButtonIcon(const IconTexture* a_IconData)
	{
		ImGui::Image(a_IconData->srView.Get(), a_IconData->size);
		return a_IconData->size;
	}

	ImVec2 ButtonIcon(const std::set<const IconTexture*>& a_textures)
	{
		ImVec2 size;
		BeginGroup();
		for (auto& iconData : a_textures) {
			auto pos = ImGui::GetCursorPos();
			size = ImGui::ButtonIcon(iconData);
			ImGui::SetCursorPos({ pos.x + size.x, pos.y });
		}
		EndGroup();
		return size;
	}

	void ButtonIconWithLabel(const char* a_text, const IconTexture* a_IconData)
	{
		auto size = ButtonIcon(a_IconData);
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x * 0.40);

		auto posY = ImGui::GetCursorPosY();
		auto textSize = ImGui::CalcTextSize(a_text);

		ImGui::SetCursorPosY(posY + (size.y - textSize.y) / 2);
		ImGui::Text(a_text);
		ImGui::SetCursorPosY(posY);
	}

	void ButtonIconWithLabel(const char* a_text, const std::set<const IconTexture*>& a_textures)
	{
		auto size = ImGui::ButtonIcon(a_textures);
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x * 0.40);

		auto posY = ImGui::GetCursorPosY();
		auto textSize = ImGui::CalcTextSize(a_text);

		ImGui::SetCursorPosY(posY + (size.y - textSize.y) / 2);
		ImGui::Text(a_text);
		ImGui::SetCursorPosY(posY);
	}
}
