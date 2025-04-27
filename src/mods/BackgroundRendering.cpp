#include "BackgroundRendering.hpp"
#include "MinHook.h"

// expose functions/variables you need to call from outside in mods.h

typedef HWND (WINAPI *GETFOCUS)(VOID);
typedef HWND (WINAPI *GETACTIVEWINDOW)(VOID);
typedef HWND (WINAPI *GETFOREGROUNDWINDOW)(VOID);

GETFOCUS            fpGetFocus            = NULL;
GETACTIVEWINDOW     fpGetActiveWindow     = NULL;
GETFOREGROUNDWINDOW fpGetForegroundWindow = NULL;

HWND modGameWindow{ 0 };
bool modEnabled{ false };

HWND WINAPI DetourGetActiveWindow() {
	if (modEnabled) {
		return modGameWindow;
	}
	else
		return fpGetActiveWindow();
}

HWND WINAPI DetourGetForegroundWindow() {
	if (modEnabled) {
		return modGameWindow;
	}
	else
		return fpGetForegroundWindow();
}

HWND WINAPI DetourGetFocus() {
	if (modEnabled) {
		return modGameWindow;
	}
	else
		return fpGetForegroundWindow();
}



std::optional<std::string> BackgroundRendering::on_initialize() {
	modGameWindow = g_framework->get_main_window_handle();
	if (MH_CreateHookApi(L"user32", "GetForegroundWindow", &DetourGetForegroundWindow, (LPVOID*)&fpGetForegroundWindow) == MH_OK) {
		spdlog::info("[BackgroundRendering]: CreateHookApi(user32, GetForegroundWindow) returned MH_OK\n");
	}
	else {
		spdlog::info("[BackgroundRendering]: CreateHookApi(user32, GetForegroundWindow) failed\n");
		return "[BackgroundRendering]: CreateHookApi(user32, GetForegroundWindow) failed";
	}
	if (MH_EnableHook(&GetForegroundWindow) == MH_OK) {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetForegroundWindow) returned MH_OK\n");
	}
	else {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetForegroundWindow) failed\n");
		return "[BackgroundRendering]: EnableHook(&GetForegroundWindow) failed";
	}

	if (MH_CreateHookApi(L"user32", "GetFocus", &DetourGetFocus, (LPVOID*)&fpGetFocus) == MH_OK) {
		spdlog::info("[BackgroundRendering]: CreateHookApi(user32, GetFocus) returned MH_OK\n");
	}
	else {
		spdlog::info("[BackgroundRendering]: CreateHookApi(user32, GetFocus) failed\n");
		return "[BackgroundRendering]: CreateHookApi(user32, GetFocus) failed";
	}
	if (MH_EnableHook(&GetFocus) == MH_OK) {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetFocus) returned MH_OK\n");
	}
	else {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetFocus) failed\n");
		return "[BackgroundRendering]: EnableHook(&GetFocus) failed";
	}

	if (MH_CreateHookApi(L"user32", "GetActiveWindow", &DetourGetActiveWindow, (LPVOID*)&fpGetActiveWindow) == MH_OK) {
		spdlog::info("[BackgroundRendering]: CreateHookApi(user32, GetActiveWindow) returned MH_OK\n");
	}
	else {
		spdlog::info("[BackgroundRendering]: CreateHookApi(user32, GetActiveWindow) failed\n");
		return "[BackgroundRendering]: CreateHookApi(user32, GetActiveWindow) failed";
	}
	if (MH_EnableHook(&GetActiveWindow) == MH_OK) {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetActiveWindow) returned MH_OK\n");
	}
	else {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetActiveWindow) failed\n");
		return "[BackgroundRendering]: EnableHook(&GetActiveWindow) failed";
	}

	return Mod::on_initialize();
}

void BackgroundRendering::on_config_load(const utility::Config& cfg) {
	modEnabled = cfg.get<bool>("enable_focus_patch").value_or(false);
};

void BackgroundRendering::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("enable_focus_patch", modEnabled);
};

void BackgroundRendering::on_draw_ui() {
	ImGui::Checkbox("Focus patch (background input)", &modEnabled);
}

