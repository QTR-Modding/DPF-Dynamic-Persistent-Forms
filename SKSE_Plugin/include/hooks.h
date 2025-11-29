#pragma once
namespace Hooks {
    void Install();

    class SaveGameHook {
    public:
        static void Install();

    private:
        static char* thunk(RE::BGSSaveLoadManager* a1, void* a2, char* a3, void* a4, int32_t a5);
        static inline REL::Relocation<decltype(thunk)> originalFunction;
    };
    class LoadGameHook {
    public:
        static void Install();

    private:
        static int32_t thunk(RE::BSWin32SaveDataSystemUtility* a1, char* fileName, void* a3);
        static inline REL::Relocation<decltype(thunk)> originalFunction;
    };
}
