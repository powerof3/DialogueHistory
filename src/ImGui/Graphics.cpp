#include "Graphics.h"

namespace ImGui
{
	Texture::Texture(std::wstring_view a_path) :
		path(a_path)
	{}

	Texture::Texture(std::wstring_view a_folder, std::wstring_view a_textureName)
	{
		path.append(a_folder).append(a_textureName).append(L".png");
	}

	Texture::~Texture()
	{
		Unload();
	}

	bool Texture::LoadImpl(float a_scale, const RE::BSGraphics::ScreenSize& a_size, bool a_resetImage)
	{
		bool result = false;

		image = std::make_shared<DirectX::ScratchImage>();
		HRESULT hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_FORCE_RGB | DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, *image);

		if (SUCCEEDED(hr)) {
			if (auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
				const auto get_resize_dimensions = [&]() -> std::pair<std::size_t, std::size_t> {
					const auto& meta = image->GetMetadata();
					if (a_size.width > 0 && a_size.height > 0 && (a_size.width != meta.width || a_size.height != meta.height)) {
						return { a_size.width, a_size.height };
					}
					if (a_scale != 1.0f) {
						return { static_cast<std::size_t>(meta.width * a_scale), static_cast<std::size_t>(meta.height * a_scale) };
					}
					return { 0, 0 };
				};

				if (auto [newWidth, newHeight] = get_resize_dimensions(); newWidth > 0 && newHeight > 0) {
					auto resized = std::make_shared<DirectX::ScratchImage>();
					if (SUCCEEDED(DirectX::Resize(*image->GetImage(0, 0, 0), newWidth, newHeight, DirectX::TEX_FILTER_FANT, *resized))) {
						image = std::move(resized);
					}
				}

				const auto device = reinterpret_cast<ID3D11Device*>(renderer->data.forwarder);
				hr = DirectX::CreateShaderResourceView(device, image->GetImages(), 1, image->GetMetadata(), &srView);
				result = SUCCEEDED(hr);

				size.x = static_cast<float>(image->GetMetadata().width);
				size.y = static_cast<float>(image->GetMetadata().height);

				if (a_resetImage) {
					image.reset();
				}
			}
		}

		return result;
	}

	void Texture::Unload()
	{
		srView.Reset();
		image.reset();
	}
}
