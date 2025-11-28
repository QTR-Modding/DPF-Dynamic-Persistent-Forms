#pragma once
#include "form.h"
#include <mutex>
#include <functional>

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

    inline RE::TESForm* Create(RE::TESForm* baseItem) {
        std::lock_guard<std::mutex> lock(serviceMutex);
        try {
            if (!baseItem) return nullptr;
            
            auto* newForm = AddForm(baseItem);
            if (newForm) {
                logger::info("new form id", newForm->GetFormID());
            }
            return newForm;
        } catch (const std::exception&) {
            return nullptr;
        }
    }

    inline void Track(RE::TESForm* baseItem) {
        std::lock_guard<std::mutex> lock(serviceMutex);
        try {
            if (!baseItem) return;
            bool found = false;
            EachFormRef([&](FormRecord* item) {
                if (item->Match(baseItem)) {
                    logger::info("reference reused");
                    if (item->deleted) {
                        item->UndeleteReference(baseItem);
                    }
                    found = true;
                    return false;
                }
                // Correção da lógica original que parecia ter um if duplicado
                return true; 
            });
            if (!found) {
                AddFormRef(FormRecord::CreateReference(baseItem));
            }
        } catch (...) {}
    }

    inline void UnTrack(RE::TESForm* form) {
        std::lock_guard<std::mutex> lock(serviceMutex);
        try {
            if (!form) return;
            EachFormRef([&](FormRecord* item) {
                if (item->Match(form)) {
                    item->deleted = true;
                    return false;
                }
                return true;
            });
        } catch (...) {}
    }

    inline void Dispose(RE::TESForm* form) {
        std::lock_guard<std::mutex> lock(serviceMutex);
        try {
            if (!form) {
                return;
            }

            EachFormData([&](FormRecord* item) {
                if (!item->deleted && item->Match(form)) {
                    item->deleted = true;
                    if (item->actualForm) {
                        item->actualForm->SetDelete(true);
                    }
                    return false;
                }
                return true;
                });


        }
        catch (const std::exception&) {
        }
    }

}
