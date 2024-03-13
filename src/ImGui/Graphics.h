#pragma once

namespace ImGui
{
	struct Texture
	{
		Texture() = delete;
		Texture(std::wstring_view a_textureFolder, std::wstring_view a_textureName);
		~Texture();

		bool Load();

		// members
		std::wstring                     name{};
		std::wstring                     path{};
		ComPtr<ID3D11ShaderResourceView> srView{ nullptr };
		ImVec2                           size{};
	};
}
