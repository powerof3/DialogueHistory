#include "Settings.h"

#include "Dialogue.h"
#include "GlobalHistory.h"
#include "Hotkeys.h"
#include "ImGui/IconsFonts.h"
#include "ImGui/Renderer.h"
#include "LocalHistory.h"

void Settings::SerializeINI(const wchar_t* a_path, const std::function<void(CSimpleIniA&)> a_func, bool a_generate)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (const auto rc = ini.LoadFile(a_path); !a_generate && rc < SI_OK) {
		return;
	}

	a_func(ini);

	(void)ini.SaveFile(a_path);
}

void Settings::LoadSettings() const
{
	SerializeINI(defaultDisplayTweaksPath, userDisplayTweaksPath, [](auto& ini) {
		DisplayTweaks::LoadSettings(ini);
	});

	LoadMCMSettings();
}

void Settings::LoadMCMSettings() const
{
	constexpr auto load_mcm_settings = [](auto& ini) {
		MANAGER(Hotkeys)->LoadHotKeys(ini);
		MANAGER(IconFont)->LoadMCMSettings(ini);       // button scheme
		MANAGER(LocalHistory)->LoadMCMSettings(ini);   // menu
		MANAGER(GlobalHistory)->LoadMCMSettings(ini);  // time format, menu
	};

	SerializeINI(defaultMCMPath, userMCMPath, load_mcm_settings);
}

void Settings::SerializeINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, std::function<void(CSimpleIniA&)> a_func)
{
	SerializeINI(a_defaultPath, a_func);
	SerializeINI(a_userPath, a_func);
}

void Settings::SerializeStyles(std::function<void(CSimpleIniA&)> a_func) const
{
	SerializeINI(stylesPath, a_func, true);
}

void Settings::SerializeFonts(std::function<void(CSimpleIniA&)> a_func) const
{
	SerializeINI(fontsPath, a_func, true);
}
