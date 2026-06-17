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

RE::TESForm* Services::CreateByType(const uint32_t formType) {
    std::lock_guard lock(serviceMutex);
    try {
        auto* newForm = AddFormByType(static_cast<RE::FormType>(formType));
        if (newForm) {
            logger::info("new form id", newForm->GetFormID());
        }
        return newForm;
    } catch (const std::exception&) {
        return nullptr;
    }
}

RE::TESForm* Services::GetOrCreateByLocalId(const uint32_t localId, const uint32_t formType) {
    std::lock_guard lock(serviceMutex);
    try {
        return GetOrCreateFormByLocalId(localId, static_cast<RE::FormType>(formType));
    } catch (const std::exception&) {
        return nullptr;
    }
}

RE::TESForm* Services::GetOrCreateByFormId(const RE::FormID formId, const uint32_t formType) {
    std::lock_guard lock(serviceMutex);
    try {
        return GetOrCreateFormByFormId(formId, static_cast<RE::FormType>(formType));
    } catch (const std::exception&) {
        return nullptr;
    }
}

RE::TESForm* Services::CreateByTypeForOwner(const char* owner, const char* key, const uint32_t formType) {
    std::lock_guard lock(serviceMutex);
    try {
        return AddFormByTypeForOwner(owner, key, static_cast<RE::FormType>(formType));
    } catch (const std::exception&) {
        return nullptr;
    }
}

RE::TESForm* Services::GetOrCreateByOwnerKey(const char* owner, const char* key, const uint32_t formType) {
    std::lock_guard lock(serviceMutex);
    try {
        return GetOrCreateFormByOwnerKey(owner, key, static_cast<RE::FormType>(formType));
    } catch (const std::exception&) {
        return nullptr;
    }
}

bool Services::ReleaseByOwnerKey(const char* owner, const char* key) {
    std::lock_guard lock(serviceMutex);
    try {
        return ReleaseFormByOwnerKey(owner, key);
    } catch (const std::exception&) {
        return false;
    }
}

bool Services::ReleaseByLocalId(const uint32_t localId, const char* owner) {
    std::lock_guard lock(serviceMutex);
    try {
        return ReleaseFormByLocalId(localId, owner);
    } catch (const std::exception&) {
        return false;
    }
}

uint32_t Services::ReleaseOwner(const char* owner) {
    std::lock_guard lock(serviceMutex);
    try {
        return ReleaseFormsByOwner(owner);
    } catch (const std::exception&) {
        return 0;
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
            return true;
        });
        if (!found) {
            AddFormRef(FormRecord::CreateReference(baseItem));
        }
    } catch (...) {
    }
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
    } catch (...) {
    }
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
    } catch (const std::exception&) {
    }
}
