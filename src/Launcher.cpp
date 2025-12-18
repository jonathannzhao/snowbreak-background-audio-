// Launcher.cpp - Start game and inject WndProcHook.dll
// Compile: cl /O2 /EHsc Launcher.cpp /link user32.lib kernel32.lib shell32.lib

#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <iostream>

// Game path - modify if needed
const wchar_t* GAME_PATH = L"D:\\steam\\steamapps\\common\\SNOWBREAK\\Game\\Binaries\\Win64\\Game.exe";
const wchar_t* GAME_DIR = L"D:\\steam\\steamapps\\common\\SNOWBREAK\\Game\\Binaries\\Win64";
const char* DLL_NAME = "WndProcHook.dll";

DWORD FindProcessByName(const wchar_t* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);

    DWORD pid = 0;
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName) == 0) {
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return pid;
}

bool InjectDLL(DWORD pid, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cout << "[ERROR] Failed to open process. Run as Administrator.\n";
        return false;
    }

    size_t pathLen = strlen(dllPath) + 1;
    LPVOID remoteMem = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        std::cout << "[ERROR] Failed to allocate memory.\n";
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteMem, dllPath, pathLen, NULL)) {
        std::cout << "[ERROR] Failed to write memory.\n";
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)pLoadLibrary, remoteMem, 0, NULL);

    if (!hThread) {
        std::cout << "[ERROR] Failed to create remote thread.\n";
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return true;
}

std::string GetDllPath() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    std::string path(exePath);
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
        path = path.substr(0, pos + 1);
    }
    path += DLL_NAME;
    return path;
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "  Snowbreak Background Audio Launcher\n";
    std::cout << "==================================================\n\n";

    // Get DLL path
    std::string dllPath = GetDllPath();

    // Check if DLL exists
    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cout << "[ERROR] " << DLL_NAME << " not found!\n";
        std::cout << "[*] Expected: " << dllPath << "\n";
        std::cout << "[*] Place " << DLL_NAME << " next to this exe.\n";
        system("pause");
        return 1;
    }

    std::cout << "[*] DLL: " << dllPath << "\n";

    // Check if game is already running
    DWORD pid = FindProcessByName(L"Game.exe");

    if (pid) {
        std::cout << "[*] Game already running (PID: " << pid << ")\n";
        std::cout << "[*] Injecting hook...\n";
    } else {
        std::cout << "[*] Starting game...\n";

        // Start game via Steam
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"open";
        sei.lpFile = L"steam://rungameid/2668080";  // Snowbreak Steam ID
        sei.nShow = SW_SHOWNORMAL;

        if (!ShellExecuteExW(&sei)) {
            std::cout << "[ERROR] Failed to start game via Steam.\n";
            std::cout << "[*] Trying direct launch...\n";

            // Direct launch fallback
            sei.lpFile = GAME_PATH;
            sei.lpDirectory = GAME_DIR;
            if (!ShellExecuteExW(&sei)) {
                std::cout << "[ERROR] Failed to start game.\n";
                system("pause");
                return 1;
            }
        }

        std::cout << "[*] Waiting for game to start...\n";

        // Wait for game process
        for (int i = 0; i < 60; i++) {  // 60 seconds timeout
            Sleep(1000);
            pid = FindProcessByName(L"Game.exe");
            if (pid) {
                std::cout << "[*] Game started (PID: " << pid << ")\n";
                break;
            }
            std::cout << ".";
        }

        if (!pid) {
            std::cout << "\n[ERROR] Game did not start within 60 seconds.\n";
            system("pause");
            return 1;
        }

        // Wait a bit more for window to initialize
        std::cout << "[*] Waiting for game window...\n";
        Sleep(5000);
    }

    // Inject DLL
    std::cout << "[*] Injecting WndProcHook...\n";

    if (InjectDLL(pid, dllPath.c_str())) {
        std::cout << "\n[OK] Hook installed successfully!\n";
        std::cout << "[*] Game audio will now play in background.\n";
        std::cout << "[*] You can minimize this window.\n\n";
    } else {
        std::cout << "[ERROR] Injection failed.\n";
        system("pause");
        return 1;
    }

    std::cout << "Press any key to exit launcher...\n";
    system("pause >nul");
    return 0;
}
