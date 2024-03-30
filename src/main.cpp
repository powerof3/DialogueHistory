#include "GlobalHistory.h"
#include "Hooks.h"
#include "ImGui/Renderer.h"
#include "Input.h"
#include "LocalHistory.h"
#include "Papyrus.h"
#include "Settings.h"
#include "NPCNameProvider.h"

void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			logger::info("{:*^30}", "POST LOAD");
			Settings::GetSingleton()->LoadSettings();
			Hooks::Install();
		}
		break;
	case SKSE::MessagingInterface::kPostPostLoad:
		{
			logger::info("{:*^30}", "POST POST LOAD");
			NPCNameProvider::GetSingleton()->RequestAPI();
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			logger::info("{:*^30}", "DATA LOADED");
			MANAGER(LocalHistory)->Register();
			MANAGER(GlobalHistory)->Register();

			PhotoMode::activeGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>("PhotoMode_IsActive");

			MANAGER(Translation)->BuildTranslationMap();
		}
		break;
	case SKSE::MessagingInterface::kSaveGame:
		{
			std::string savePath{ static_cast<char*>(a_msg->data), a_msg->dataLen };
			MANAGER(GlobalHistory)->SaveToFile(savePath);
		}
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		{
			std::string savePath{ static_cast<char*>(a_msg->data), a_msg->dataLen };
			string::replace_last_instance(savePath, ".ess", "");

			MANAGER(GlobalHistory)->LoadFromFile(savePath);
		}
		break;
	case SKSE::MessagingInterface::kDeleteGame:
		{
			const std::string savePath({ static_cast<char*>(a_msg->data), a_msg->dataLen });
			MANAGER(GlobalHistory)->DeleteSavedFile(savePath);
		}
		break;
	case SKSE::MessagingInterface::kNewGame:
		MANAGER(GlobalHistory)->Clear();
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("DialogueHistory");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "DialogueHistory";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

	SKSE::Init(a_skse);

	ImGui::Renderer::Install();

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", OnInit);

	SKSE::GetPapyrusInterface()->Register(Papyrus::Register);

	return true;
}
