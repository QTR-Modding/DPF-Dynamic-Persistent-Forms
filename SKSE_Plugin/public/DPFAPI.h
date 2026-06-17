#pragma once

#include "RE/Skyrim.h"
#include <Windows.h>
#include <cstdint>

namespace DPF {
    constexpr auto InterfaceName = "DynamicPersistentForms";
    constexpr uint32_t InterfaceVersion = 2;

    class IDynamicPersistentForms {
    public:
        virtual ~IDynamicPersistentForms() = default;

        virtual uint32_t GetVersion() const = 0;

        // v1
        virtual RE::TESForm* Create(RE::TESForm* baseItem) = 0;
        virtual void Dispose(RE::TESForm* form) = 0;
        virtual void Track(RE::TESForm* item) = 0;
        virtual void UnTrack(RE::TESForm* item) = 0;

        // v2
        virtual RE::TESForm* CreateByType(uint32_t formType) = 0;
    };

    using GetDPFAPI = void* (*)();

    inline IDynamicPersistentForms* GetAPI() {
        const auto module = GetModuleHandleA("DynamicPersistentForms.dll");
        if (!module) {
            return nullptr;
        }

        const auto getter = reinterpret_cast<GetDPFAPI>(GetProcAddress(module, "GetDPFAPI"));
        if (!getter) {
            return nullptr;
        }

        auto* api = static_cast<IDynamicPersistentForms*>(getter());
        if (!api || api->GetVersion() < InterfaceVersion) {
            return nullptr;
        }

        return api;
    }
}

