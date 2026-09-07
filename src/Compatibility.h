#pragma once

namespace DisplayTweaks
{
	void  LoadSettings(const CSimpleIniA& a_ini);
	float GetResolutionScale();

	// members
	inline float resolutionScale{ 1.0f };
	inline bool  borderlessUpscale{ false };
}

namespace PhotoMode
{
	void InitGlobals();
	
	bool IsPhotoModeActive();

	// members
	inline RE::TESGlobal* activeGlobal{ nullptr };
	inline RE::TESGlobal* activeGalleryGlobal{ nullptr };

}
