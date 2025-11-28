#pragma once
#include <mutex>

namespace DPF {
    // Defina um ID único para sua interface
    constexpr const char* InterfaceName = "DynamicPersistentForms";
    constexpr uint32_t InterfaceVersion = 1;

    // Classe abstrata pura
    class IDynamicPersistentForms {
    public:
        virtual ~IDynamicPersistentForms() = default;

        // Versão da API para compatibilidade
        virtual uint32_t GetVersion() const = 0;

        // Métodos do seu mod (espelhando o que você fez no Papyrus)
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
