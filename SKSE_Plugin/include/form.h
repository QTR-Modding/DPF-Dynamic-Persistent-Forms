#pragma once
#include "form_record.h"

static void copyFormArmorModel(RE::TESForm* source, RE::TESForm* target);
static void copyFormObjectWeaponModel(RE::TESForm* source, RE::TESForm* target);
static void copyMagicEffect(RE::TESForm* source, RE::TESForm* target);
static void copyBookAppearence(RE::TESForm* source, RE::TESForm* target);
void applyPattern(FormRecord* instance);

template <class T>
void copyComponent(RE::TESForm* from, RE::TESForm* to) {
    auto fromT = from->As<T>();
    auto toT = to->As<T>();
    if (fromT && toT) {
        toT->CopyComponent(fromT);
    }
}

void copyAppearence(RE::TESForm* source, RE::TESForm* target);

RE::TESForm* AddForm(RE::TESForm* baseItem);

RE::TESForm* AddFormByType(RE::FormType formType);

RE::TESForm* AddFormByTypeForOwner(const char* owner, const char* key, RE::FormType formType);

RE::TESForm* GetOrCreateFormByLocalId(uint32_t localId, RE::FormType formType);

RE::TESForm* GetOrCreateFormByFormId(RE::FormID formId, RE::FormType formType);

RE::TESForm* GetOrCreateFormByOwnerKey(const char* owner, const char* key, RE::FormType formType);

RE::TESForm* GetOrCreateFormByOwnerKeyEx(const char* owner, const char* key, RE::FormType formType, uint32_t* localId, bool* existed);

bool ReleaseFormByOwnerKey(const char* owner, const char* key);

bool ReleaseFormByLocalId(uint32_t localId, const char* owner);

uint32_t ReleaseFormsByOwner(const char* owner);
