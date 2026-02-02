#include <windows.h>

#pragma comment(linker, "/EXPORT:DirectInput8Create=_DirectInput8Create@20")

HMODULE gDInput8 = NULL;

typedef HRESULT(WINAPI* tDirectInput8Create)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

tDirectInput8Create pDirectInput8Create = NULL;

static volatile LONG gInitialized = 0;

void LoadDInput8()
{
    if (gDInput8) return;

    wchar_t path[MAX_PATH];
    GetSystemDirectoryW(path, MAX_PATH);
    lstrcatW(path, L"\\dinput8.dll");

    gDInput8 = LoadLibraryW(path);
}

DWORD WINAPI ModThread(LPVOID param)
{
    uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
    uintptr_t procAddr = base + 0x186D8;
    uintptr_t endpAddr = base + 0x187BF;

    DWORD oldProtect;
    VirtualProtect((LPVOID)procAddr, 8, PAGE_EXECUTE_READWRITE, &oldProtect);

    int rel = (int)(endpAddr - (procAddr + 5));

    BYTE patch[5];
    patch[0] = 0xE9;
    memcpy(&patch[1], &rel, 4);

    memcpy((void*)procAddr, patch, 5);

    VirtualProtect((LPVOID)procAddr, 8, oldProtect, &oldProtect);

    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)procAddr, 5);

    return 0;
}

__declspec(dllexport)
HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
    LoadDInput8();
    if (gDInput8 == NULL)
    {
        MessageBoxA(0, "Failed to load the real dinput8.dll!", "Umi Mod", 0);
        return E_FAIL;
    }

    pDirectInput8Create = (tDirectInput8Create)GetProcAddress(gDInput8, "DirectInput8Create");
    if (pDirectInput8Create == NULL)
    {
        MessageBoxA(0, "GetProcAddress failed for DirectInput8Create!", "Umi Mod", 0);
        return E_FAIL;
    }

    if (InterlockedCompareExchange(&gInitialized, 1, 0) == 0)
    {
        CreateThread(NULL, 0, ModThread, NULL, 0, NULL);
    }

    return pDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

