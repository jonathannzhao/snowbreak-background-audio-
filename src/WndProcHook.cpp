// WndProcHook.cpp - Hook WndProc to intercept WM_ACTIVATEAPP messages
// Compile: cl /LD /O2 WndProcHook.cpp /link user32.lib

#include <windows.h>

// Global variables
HWND g_GameWindow = nullptr;
WNDPROC g_OriginalWndProc = nullptr;
DWORD g_GameProcessId = 0;
HMODULE g_hModule = nullptr;

// Our custom WndProc
LRESULT CALLBACK HookedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ACTIVATEAPP:
        // wParam: TRUE = activated, FALSE = deactivated
        if (wParam == FALSE) {
            // Block deactivation message - pretend we're still active
            return 0;
        }
        break;

    case WM_ACTIVATE:
        // LOWORD(wParam): WA_INACTIVE, WA_ACTIVE, WA_CLICKACTIVE
        if (LOWORD(wParam) == WA_INACTIVE) {
            // Block inactive message
            return 0;
        }
        break;

    case WM_KILLFOCUS:
        // Block kill focus message
        return 0;

    case WM_SETFOCUS:
        // Always allow set focus
        break;
    }

    // Call original WndProc for all other messages
    return CallWindowProcW(g_OriginalWndProc, hwnd, msg, wParam, lParam);
}

// Find main game window callback
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == g_GameProcessId) {
        if (IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr) {
            RECT rect;
            GetWindowRect(hwnd, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            // Must be a reasonably sized window (game window)
            if (width > 640 && height > 480) {
                g_GameWindow = hwnd;
                return FALSE; // Stop enumeration
            }
        }
    }
    return TRUE;
}

// Install WndProc hook
bool InstallHook() {
    if (!g_GameWindow) return false;

    // Replace WndProc using SetWindowLongPtrW
    g_OriginalWndProc = (WNDPROC)SetWindowLongPtrW(
        g_GameWindow,
        GWLP_WNDPROC,
        (LONG_PTR)HookedWndProc
    );

    return g_OriginalWndProc != nullptr;
}

// Remove WndProc hook
void RemoveHook() {
    if (g_GameWindow && g_OriginalWndProc) {
        SetWindowLongPtrW(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
        g_OriginalWndProc = nullptr;
    }
}

// Thread to find window and install hook
DWORD WINAPI HookThread(LPVOID lpParam) {
    // Wait for game window to be created
    for (int retry = 0; retry < 30; retry++) {
        Sleep(1000);

        g_GameWindow = nullptr;
        EnumWindows(EnumWindowsProc, 0);

        if (g_GameWindow) {
            if (InstallHook()) {
                // Hook installed successfully
                return 0;
            }
        }
    }
    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        g_hModule = hModule;
        g_GameProcessId = GetCurrentProcessId();

        // Start hook thread
        CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        RemoveHook();
        break;
    }
    return TRUE;
}
