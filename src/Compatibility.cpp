#include "Compatibility.h"

float DisplayTweaks::GetResolutionScale()
{
	static auto height = RE::BSGraphics::Renderer::GetScreenSize().height;

	return borderlessUpscale ?
	           resolutionScale :
	           height / 1080.0f;
}

void DisplayTweaks::LoadSettings(const CSimpleIniA& a_ini)
{
	resolutionScale = static_cast<float>(a_ini.GetDoubleValue("Render", "ResolutionScale", resolutionScale));
	borderlessUpscale = static_cast<float>(a_ini.GetBoolValue("Render", "BorderlessUpscale", borderlessUpscale));
}

bool PhotoMode::IsPhotoModeActive()
{
	return activeGlobal && activeGlobal->value == 1;
}
