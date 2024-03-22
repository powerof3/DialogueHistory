#include "Renderer.h"

#include "GlobalHistory.h"
#include "IconsFonts.h"
#include "Input.h"
#include "LocalHistory.h"
#include "Styles.h"

namespace ImGui::Renderer
{
	struct CreateD3DAndSwapChain
	{
		static void thunk()
		{
			func();

			if (const auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
				const auto swapChain = (IDXGISwapChain*)renderer->data.renderWindows[0].swapChain;
				if (!swapChain) {
					logger::error("couldn't find swapChain");
					return;
				}

				DXGI_SWAP_CHAIN_DESC desc{};
				if (FAILED(swapChain->GetDesc(std::addressof(desc)))) {
					logger::error("IDXGISwapChain::GetDesc failed.");
					return;
				}

				const auto device = (ID3D11Device*)renderer->data.forwarder;
				const auto context = (ID3D11DeviceContext*)renderer->data.context;

				logger::info("Initializing ImGui..."sv);

				ImGui::CreateContext();

				auto& io = ImGui::GetIO();
				io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NoMouseCursorChange;
				io.IniFilename = nullptr;

				if (!SKSE::ImGui_ImplWin32_Init(desc.OutputWindow)) {
					logger::error("ImGui initialization failed (Win32)");
					return;
				}
				if (!ImGui_ImplDX11_Init(device, context)) {
					logger::error("ImGui initialization failed (DX11)"sv);
					return;
				}

				logger::info("ImGui initialized.");

				MANAGER(IconFont)->LoadIcons();

				initialized.store(true);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// IMenu::PostDisplay
	struct PostDisplay
	{
		static void thunk(RE::IMenu* a_menu)
		{
			// Skip if Imgui is not loaded
			if (!initialized.load()) {
				return func(a_menu);
			}
		
			if (renderMenus.load()) {
				// refresh style
				ImGui::Styles::GetSingleton()->OnStyleRefresh();

				ImGui_ImplDX11_NewFrame();
				SKSE::ImGui_ImplWin32_NewFrame();
				{
					//trick imgui into rendering at game's real resolution (ie. if upscaled with Display Tweaks)
					static const auto screenSize = RE::BSGraphics::Renderer::GetScreenSize();

					auto& io = ImGui::GetIO();
					io.DisplaySize.x = (float)screenSize.width;
					io.DisplaySize.y = (float)screenSize.height;
				}
				ImGui::NewFrame();
				{
					MANAGER(LocalHistory)->Draw();
					MANAGER(GlobalHistory)->Draw();
				}
				ImGui::EndFrame();
				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			}

			func(a_menu);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx{ 0x6 };
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(75595, 77226), OFFSET(0x9, 0x275) };  // BSGraphics::InitD3D
		stl::write_thunk_call<CreateD3DAndSwapChain>(target.address());

		stl::write_vfunc<RE::HUDMenu, PostDisplay>();
	}

	void RenderMenus(bool a_render)
	{
		renderMenus = a_render;
	}
}
