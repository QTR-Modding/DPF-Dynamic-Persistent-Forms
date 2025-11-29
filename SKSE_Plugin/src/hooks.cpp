#include "hooks.h"
#include "persistence.h"

char* Hooks::SaveGameHook::thunk(RE::BGSSaveLoadManager* manager, void* a2, char* fileName, void* a4, int32_t a5) {
    auto util = RE::BSWin32SaveDataSystemUtility::GetSingleton();
    char fullPath[242];
    util->PrepareFileSavePath(fileName, fullPath, 0, 0);
    Persistence::Save(fullPath);
    return originalFunction(manager, a2, fileName, a4, a5);
}

void Hooks::SaveGameHook::Install() {
    SKSE::AllocTrampoline(14);
    auto& trampoline = SKSE::GetTrampoline();
    originalFunction = trampoline.write_call<5>(REL::RelocationID(34818, 35727).address() + REL::Relocate(0x112, 0x1ce), thunk);
}

int32_t Hooks::LoadGameHook::thunk(RE::BSWin32SaveDataSystemUtility* util, char* fileName, void* unknown) {
    char fullPath[242];
    util->PrepareFileSavePath(fileName, fullPath, 0, 0);
    Persistence::Load(fullPath);
    return originalFunction(util, fileName, unknown);
}

void Hooks::LoadGameHook::Install() {
    SKSE::AllocTrampoline(14);
    auto& trampoline = SKSE::GetTrampoline();
    originalFunction = trampoline.write_call<5>(REL::RelocationID(34677, 35600).address() + REL::Relocate(0xab, 0xab), thunk);
}

void Hooks::Install() {
    LoadGameHook::Install();
    SaveGameHook::Install();
}