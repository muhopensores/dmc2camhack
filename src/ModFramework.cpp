#include <spdlog/sinks/basic_file_sink.h>

#include <imgui/imgui.h>

// ours with XInput removed
#include "fw-imgui/imgui_impl_win32.h"
#include "fw-imgui/imgui_impl_dx11.h"

#include "utility/Module.hpp"

#include "sdk/REGlobals.hpp"
#include "Mods.hpp"

#include "LicenseStrings.hpp"
#include "ModFramework.hpp"

#include "Config.hpp"

std::unique_ptr<ModFramework> g_framework{};

ModFramework::ModFramework()
    : m_game_module{ GetModuleHandle(0) },
    m_logger{ spdlog::basic_logger_mt("ModFramework", LOG_FILENAME, true) }
{
    spdlog::set_default_logger(m_logger);
    spdlog::flush_on(spdlog::level::info);
    spdlog::info(LOG_ENTRY);

#ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    m_d3d11_hook = std::make_unique<D3D11Hook>();
    m_d3d11_hook->on_present([this](D3D11Hook& hook) { on_frame(); });
    m_d3d11_hook->on_resize_buffers([this](D3D11Hook& hook) { on_reset(); });

    m_valid = m_d3d11_hook->hook();

    if (m_valid) {
        spdlog::info("Hooked D3D11");
    }
}

ModFramework::~ModFramework() {

}

void ModFramework::on_frame() {
    spdlog::debug("on_frame");

    if (!m_initialized) {
        if (!initialize()) {
            spdlog::error("Failed to initialize ModFramework");
            return;
        }

        spdlog::info("ModFramework initialized");
        m_initialized = true;
        return;
    }

	auto& io = ImGui::GetIO();

	auto swap_chain = m_d3d11_hook->get_swap_chain();

	DXGI_SWAP_CHAIN_DESC swap_desc{};
	swap_chain->GetDesc(&swap_desc);

	if (swap_desc.BufferDesc.Width > 0 && swap_desc.BufferDesc.Height > 0) {
#if 0
		RECT rect;
		::GetClientRect(m_wnd, &rect);
		auto size = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
		auto fac  = ImVec2(
			(float)swap_desc.BufferDesc.Width  / size.x,
			(float)swap_desc.BufferDesc.Height / size.y);
		
		rect.left = (int)((float)rect.left * fac.x);
		rect.right = (int)((float)rect.right * fac.x);
		rect.top   = (int)((float)rect.top * fac.y);
		rect.bottom= (int)((float)rect.bottom * fac.y);
		
		AdjustWindowRect(&rect, GetWindowLong(m_wnd, GWL_STYLE), FALSE);
		SetWindowPos(m_wnd, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		UpdateWindow(m_wnd);
#endif

		/*io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
		//auto size = ImVec2((float)swap_desc.BufferDesc.Width, (float)swap_desc.BufferDesc.Height);
		io.DisplayFramebufferScale = ImVec2(
			(float)swap_desc.BufferDesc.Width  / io.DisplaySize.x,
			(float)swap_desc.BufferDesc.Height / io.DisplaySize.y);
		io.DisplaySize = ImVec2((float)(rect.right - rect.left) * io.DisplayFramebufferScale.x, (float)(rect.bottom - rect.top)  * io.DisplayFramebufferScale.y);*/
	}
	

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (m_error.empty() && m_game_data_initialized) {
        m_mods->on_frame();
    }

    draw_ui();

    ImGui::EndFrame();
    ImGui::Render();

    ID3D11DeviceContext* context = nullptr;
    m_d3d11_hook->get_device()->GetImmediateContext(&context);

    context->OMSetRenderTargets(1, &m_main_render_target_view, NULL);
	
	auto dd = ImGui::GetDrawData();

    ImGui_ImplDX11_RenderDrawData(dd);
}

void ModFramework::on_reset() {
    spdlog::info("Reset!");

    // Crashes if we don't release it at this point.
    cleanup_render_target();
    m_initialized = false;
}

bool ModFramework::on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param) {
    if (!m_initialized) {
        return true;
    }

    if (m_draw_ui && ImGui_ImplWin32_WndProcHandler(wnd, message, w_param, l_param) != 0) {
        // If the user is interacting with the UI we block the message from going to the game.
        auto& io = ImGui::GetIO();

        if (io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput) {
            return false;
        }
    }
	
    return true;
}

// this is unfortunate.
void ModFramework::on_direct_input_keys(const std::array<uint8_t, 256>& keys) {
    if (keys[m_menu_key] && m_last_keys[m_menu_key] == 0) {
        std::lock_guard _{ m_input_mutex };
        m_draw_ui = !m_draw_ui;

        // Save the config if we close the UI
        if (!m_draw_ui && m_game_data_initialized) {
            save_config();
        }
    }

    m_last_keys = keys;
}

void ModFramework::save_config() {
    spdlog::info("Saving config to file");

    utility::Config cfg{};

    for (auto& mod : m_mods->get_mods()) {
        mod->on_config_save(cfg);
    }

    if (!cfg.save(CONFIG_FILENAME)) {
        spdlog::info("Failed to save config");
        return;
    }

    spdlog::info("Saved config");
}

void ModFramework::draw_ui() {
    std::lock_guard _{ m_input_mutex };

    if (!m_draw_ui) {
        m_dinput_hook->acknowledge_input();
        ImGui::GetIO().MouseDrawCursor = false;
        return;
    }

    auto& io = ImGui::GetIO();

    if (io.WantCaptureKeyboard) {
        m_dinput_hook->ignore_input();
    }
    else {
        m_dinput_hook->acknowledge_input();
    }

    ImGui::GetIO().MouseDrawCursor = true;

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_::ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_::ImGuiCond_Once);

    ImGui::Begin("ModFramework", &m_draw_ui);
	ImGui::Text("Version: %s", GIT_HASH);
	ImGui::Text("Date: %s", GIT_DATE);
    ImGui::Text("Menu Key: Insert");

    draw_about();

    if (m_error.empty() && m_game_data_initialized) {
        m_mods->on_draw_ui();
    }
    else if (!m_game_data_initialized) {
        ImGui::TextWrapped("ModFramework is currently initializing...");
    }
    else if(!m_error.empty()) {
        ImGui::TextWrapped("ModFramework error: %s", m_error.c_str());
    }

    ImGui::End();
}

void ModFramework::draw_about() {
    if (!ImGui::CollapsingHeader("About")) {
        return;
    }

    ImGui::TreePush("About");

    ImGui::Text("Author: praydog");
    ImGui::Text("Inspired by the Kanan project.");
    ImGui::Text("https://github.com/praydog/RE2-Mod-Framework");

    if (ImGui::CollapsingHeader("Licenses")) {
        ImGui::TreePush("Licenses");

        if (ImGui::CollapsingHeader("glm")) {
            ImGui::TextWrapped(license::glm);
        }

        if (ImGui::CollapsingHeader("imgui")) {
            ImGui::TextWrapped(license::imgui);
        }

        if (ImGui::CollapsingHeader("minhook")) {
            ImGui::TextWrapped(license::minhook);
        }

        if (ImGui::CollapsingHeader("spdlog")) {
            ImGui::TextWrapped(license::spdlog);
        }

        ImGui::TreePop();
    }

    ImGui::TreePop();
}

bool ModFramework::initialize() {
    if (m_initialized) {
        return true;
    }

    spdlog::info("Attempting to initialize");

    auto device = m_d3d11_hook->get_device();
    auto swap_chain = m_d3d11_hook->get_swap_chain();

    // Wait.
    if (device == nullptr || swap_chain == nullptr) {
        spdlog::info("Device or SwapChain null. DirectX 12 may be in use. A crash may occur.");
        return false;
    }

    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);

    DXGI_SWAP_CHAIN_DESC swap_desc{};
    swap_chain->GetDesc(&swap_desc);

    m_wnd = swap_desc.OutputWindow;

	//m_mouse = std::make_unique<DirectX::Mouse>();
	//m_mouse->SetWindow(m_wnd);

    // Explicitly call destructor first
    m_windows_message_hook.reset();
    m_windows_message_hook = std::make_unique<WindowsMessageHook>(m_wnd);
    m_windows_message_hook->on_message = [this](auto wnd, auto msg, auto wParam, auto lParam) {
        return on_message(wnd, msg, wParam, lParam);
    };

    // just do this instead of rehooking because there's no point.
    if (m_first_frame) {
        m_dinput_hook = std::make_unique<DInputHook>(m_wnd);
    }
    else {
        m_dinput_hook->set_window(m_wnd);
    }

    spdlog::info("Creating render target");

    create_render_target();

    spdlog::info("Window Handle: {0:x}", (uintptr_t)m_wnd);
    spdlog::info("Initializing ImGui");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    spdlog::info("Initializing ImGui Win32");

    if (!ImGui_ImplWin32_Init(m_wnd)) {
        spdlog::error("Failed to initialize ImGui.");
        return false;
    }

    spdlog::info("Initializing ImGui D3D11");

    if (!ImGui_ImplDX11_Init(device, context)) {
        spdlog::error("Failed to initialize ImGui.");
        return false;
    }

    ImGui::StyleColorsDark();

    if (m_first_frame) {
        m_first_frame = false;

        spdlog::info("Starting game data initialization thread");

        // Game specific initialization stuff
        std::thread init_thread([this]() {
            m_mods = std::make_unique<Mods>();

            auto e = m_mods->on_initialize();

            if (e) {
                if (e->empty()) {
                    m_error = "An unknown error has occurred.";
                }
                else {
                    m_error = *e;
                }
            }

            m_game_data_initialized = true;
        });

        init_thread.detach();
    }

    return true;
}

void ModFramework::create_render_target() {
    cleanup_render_target();

    ID3D11Texture2D* back_buffer{ nullptr };
    if (m_d3d11_hook->get_swap_chain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&back_buffer) == S_OK) {
        m_d3d11_hook->get_device()->CreateRenderTargetView(back_buffer, NULL, &m_main_render_target_view);
        back_buffer->Release();
    }
}

void ModFramework::cleanup_render_target() {
    if (m_main_render_target_view != nullptr) {
        m_main_render_target_view->Release();
        m_main_render_target_view = nullptr;
    }
}

