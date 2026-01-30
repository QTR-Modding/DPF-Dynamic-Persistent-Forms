#pragma once
#include <mutex>

namespace DPF {
    constexpr auto InterfaceName = "DynamicPersistentForms";
    constexpr uint32_t InterfaceVersion = 1;

    class IDynamicPersistentForms {
    public:
        virtual ~IDynamicPersistentForms() = default;
        virtual uint32_t GetVersion() const = 0;


        virtual RE::TESForm* Create(RE::TESForm* baseItem) = 0;
        virtual void Dispose(RE::TESForm* form) = 0;
        virtual void Track(RE::TESForm* item) = 0;
        virtual void UnTrack(RE::TESForm* item) = 0;
    };
}

namespace Services {
    // Mutex global para thread safety
    inline std::mutex serviceMutex;

    RE::TESForm* Create(RE::TESForm* baseItem);

    void Track(RE::TESForm* baseItem);

    void UnTrack(RE::TESForm* form);

    void Dispose(RE::TESForm* form);
}