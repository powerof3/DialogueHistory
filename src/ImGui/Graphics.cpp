#include "Graphics.h"

namespace ImGui
{
	Texture::Texture(std::wstring_view a_textureFolder, std::wstring_view a_textureName) :
		name(a_textureName)
	{
		path.append(a_textureFolder)
			.append(a_textureName)
			.append(L".png");
	}

	Texture::~Texture()
	{
		if (srView) {
			srView.Reset();
		}
	}

	bool Texture::Load()
	{
		bool result = false;

		auto image = std::make_shared<DirectX::ScratchImage>();
		auto hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, *image);

		if (SUCCEEDED(hr)) {
			if (auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
				ComPtr<ID3D11Resource> pTexture{};
				ID3D11Device*          device = (ID3D11Device*)renderer->data.forwarder;

				hr = DirectX::CreateTexture(device, image->GetImages(), 1, image->GetMetadata(), pTexture.GetAddressOf());

				if (SUCCEEDED(hr)) {
					D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
					srvDesc.Format = image->GetMetadata().format;
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = 1;
					srvDesc.Texture2D.MostDetailedMip = 0;

					hr = device->CreateShaderResourceView(pTexture.Get(), &srvDesc, srView.GetAddressOf());
					result = SUCCEEDED(hr);
				}

				size.x = static_cast<float>(image->GetMetadata().width);
				size.y = static_cast<float>(image->GetMetadata().height);

				pTexture.Reset();
			}
		}

		if (image) {
			image.reset();
		}

		return result;
	}
}
