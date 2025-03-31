#pragma once

#include "NND_API.h"

class NPCNameProvider : public REX::Singleton<NPCNameProvider>
{
public:
	const char* GetName(RE::TESObjectREFR* actor) const;

	void RequestAPI();

private:
	NND_API::IVNND1* NND{ nullptr };
};
