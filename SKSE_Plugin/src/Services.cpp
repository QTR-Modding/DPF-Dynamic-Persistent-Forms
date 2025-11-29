#include "Services.h"
#include "form.h"
#include "model.h"

RE::TESForm* Services::Create(RE::TESForm* baseItem) {
    std::lock_guard lock(serviceMutex);
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

void Services::Track(RE::TESForm* baseItem) {
    std::lock_guard lock(serviceMutex);
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

void Services::UnTrack(RE::TESForm* form) {
    std::lock_guard lock(serviceMutex);
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

void Services::Dispose(RE::TESForm* form) {
    std::lock_guard lock(serviceMutex);
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