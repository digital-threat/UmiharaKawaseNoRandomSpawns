#include <windows.h>

#pragma comment(linker, "/EXPORT:DirectInput8Create=_DirectInput8Create@20")

HMODULE realDInput8 = NULL;

typedef HRESULT(WINAPI* tDirectInput8Create)(
    HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

tDirectInput8Create pDirectInput8Create = NULL;

void LoadRealDInput8()
{
    if (realDInput8) return;

    wchar_t path[MAX_PATH];
    GetSystemDirectoryW(path, MAX_PATH);
    lstrcatW(path, L"\\dinput8.dll");

    realDInput8 = LoadLibraryW(path);

    pDirectInput8Create = (tDirectInput8Create)GetProcAddress(realDInput8, "DirectInput8Create");
}


__declspec(dllexport)
HRESULT WINAPI DirectInput8Create(
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID* ppvOut,
    LPUNKNOWN punkOuter)
{
    LoadRealDInput8();
    return pDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

DWORD WINAPI ModThread(LPVOID param)
{
    Sleep(500);

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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, ModThread, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

