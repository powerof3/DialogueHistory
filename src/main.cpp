#include "GlobalHistory.h"
#include "Hooks.h"
#include "ImGui/Renderer.h"
#include "Input.h"
#include "LocalHistory.h"
#include "NPCNameProvider.h"
#include "Papyrus.h"
#include "Settings.h"

void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			REX::INFO("{:*^50}", "POST LOAD");
			Settings::GetSingleton()->LoadMCMSettings();
			Hooks::Install();
		}
		break;
	case SKSE::MessagingInterface::kPostPostLoad:
		{
			REX::INFO("{:*^50}", "POST POST LOAD");
			NPCNameProvider::GetSingleton()->RequestAPI();
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			REX::INFO("{:*^50}", "DATA LOADED");
			MANAGER(LocalHistory)->Register();
			MANAGER(GlobalHistory)->Register();

			PhotoMode::activeGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>("PhotoMode_IsActive");
			MANAGER(Translation)->BuildTranslationMap();

			REX::INFO("{:*^50}", "FILE CLEANUP");
			MANAGER(GlobalHistory)->CleanupSavedFiles();
		}
		break;
	case SKSE::MessagingInterface::kSaveGame:
		{
			std::string savePath{ static_cast<char*>(a_msg->data), a_msg->dataLen };
			MANAGER(GlobalHistory)->SaveFiles(savePath);
		}
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		{
			std::string savePath{ static_cast<char*>(a_msg->data), a_msg->dataLen };
			REX::STR::REPLACE_LAST_INSTANCE(savePath, ".ess", "");

			REX::INFO("{:*^50}", "LOAD GAME");
			MANAGER(GlobalHistory)->LoadFiles(savePath);
		}
		break;
	case SKSE::MessagingInterface::kDeleteGame:
		{
			const std::string savePath({ static_cast<char*>(a_msg->data), a_msg->dataLen });
			MANAGER(GlobalHistory)->DeleteSavedFiles(savePath);
		}
		break;
	case SKSE::MessagingInterface::kNewGame:
		MANAGER(GlobalHistory)->Clear();
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_SUPPORT_AE
SKSE_PLUGIN_VERSION = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(REL::Version{ Version::MAJOR, Version::MINOR, Version::PATCH });
	v.PluginName("Crafting Recipe Distributor");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesNoStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });

	if constexpr (SKSE::RUNTIME_SSE_LATEST < Runtime::MIN_ADDRESS_LIBRARY_V5) {
		v.MinimumRequiredXSEVersion(REL::Version{ 2, 2, 5 });
	} else {
		v.MinimumRequiredXSEVersion(REL::Version{ 2, 3, 0 });
	}

	return v;
}();
#else
SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Crafting Recipe Distributor";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		REX::CRITICAL("Loaded in editor, marking as incompatible");
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver
#	ifndef SKYRIMVR
		< SKSE::RUNTIME_SSE_1_5_39
#	else
		> SKSE::RUNTIME_VR_1_4_15_1
#	endif
	) {
		REX::CRITICAL("Unsupported runtime version {}", ver.string());
		return false;
	}

	return true;
}
#endif

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, { .log = true,
						   .logName = Version::PROJECT.data(),
						   .trampoline = true,
						   .trampolineSize = 14 * 9 });

	auto runtimeVersion = a_skse->RuntimeVersion();

	REX::INFO("Game version : {} (built for {})", runtimeVersion, SKSE::RUNTIME_SSE_LATEST);

#ifdef SKYRIM_SUPPORT_AE
	if constexpr (SKSE::RUNTIME_SSE_LATEST < Runtime::MIN_ADDRESS_LIBRARY_V5) {
		if (runtimeVersion >= Runtime::MIN_ADDRESS_LIBRARY_V5) {
			REX::FAIL(
				"You are using a newer version of Skyrim than this version of {0} supports.\n"
				"Install the correct version of {0} for your game version.\n"
				"Runtime: {1}\n"
				"Supported: 1.6.1170 (Steam) / 1.6.1179 (GOG)",
				Version::PROJECT, runtimeVersion);
		}
	}
#endif

	Settings::GetSingleton()->Load(FileType::kDisplayTweaks, [](auto& ini) {
		DisplayTweaks::LoadSettings(ini);  // display tweaks scaling
	});

	ImGui::Renderer::Install();

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", OnInit);

	SKSE::GetPapyrusInterface()->Register(Papyrus::Register);

	return true;
}
