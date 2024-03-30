#include "NPCNameProvider.h"
#include "NND_API.h"

namespace logger = SKSE::log;

const char* NPCNameProvider::GetName(RE::TESObjectREFR* ref) const
{
	if (NND) {
		if (auto actor = ref->As<RE::Actor>(); actor) {
			if (auto name = NND->GetName(actor, NND_API::NameContext::kDialogueHistory); !name.empty()) {
				return name.data();
			}
		}
	}

	return ref->GetDisplayFullName();
}

void NPCNameProvider::RequestAPI()
{
	if (!NND) {
		NND = static_cast<NND_API::IVNND1*>(NND_API::RequestPluginAPI(NND_API::InterfaceVersion::kV2));
		if (NND) {
			logger::info("Obtained NND API - {0:x}", reinterpret_cast<uintptr_t>(NND));
		} else {
			logger::warn("Failed to obtain NND API");
		}
	}
}
