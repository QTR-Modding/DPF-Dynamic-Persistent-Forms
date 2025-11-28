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