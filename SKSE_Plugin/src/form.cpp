#include "form.h"
#include "model.h"
#include "persistence.h"

void copyFormArmorModel(RE::TESForm* source, RE::TESForm* target) {
    const auto* sourceModelBipedForm = source->As<RE::TESObjectARMO>();
    auto* targeteModelBipedForm = target->As<RE::TESObjectARMO>();
    if (sourceModelBipedForm && targeteModelBipedForm) {
        logger::trace("armor");
        targeteModelBipedForm->armorAddons = sourceModelBipedForm->armorAddons;
    }
}

void copyFormObjectWeaponModel(RE::TESForm* source, RE::TESForm* target) {
    const auto* sourceModelWeapon = source->As<RE::TESObjectWEAP>();
    auto* targeteModelWeapon = target->As<RE::TESObjectWEAP>();
    if (sourceModelWeapon && targeteModelWeapon) {
        logger::trace("weapon");
        targeteModelWeapon->firstPersonModelObject = sourceModelWeapon->firstPersonModelObject;
        targeteModelWeapon->attackSound = sourceModelWeapon->attackSound;
        targeteModelWeapon->attackSound2D = sourceModelWeapon->attackSound2D;
        targeteModelWeapon->attackSound = sourceModelWeapon->attackSound;
        targeteModelWeapon->attackFailSound = sourceModelWeapon->attackFailSound;
        targeteModelWeapon->idleSound = sourceModelWeapon->idleSound;
        targeteModelWeapon->equipSound = sourceModelWeapon->equipSound;
        targeteModelWeapon->unequipSound = sourceModelWeapon->unequipSound;
        targeteModelWeapon->soundLevel = sourceModelWeapon->soundLevel;
    }
}

void copyMagicEffect(RE::TESForm* source, RE::TESForm* target) {
    const auto* sourceEffect = source->As<RE::EffectSetting>();
    auto* targetEffect = target->As<RE::EffectSetting>();
    if (sourceEffect && targetEffect) {
        targetEffect->effectSounds = sourceEffect->effectSounds;
        targetEffect->data.castingArt = sourceEffect->data.castingArt;
        targetEffect->data.light = sourceEffect->data.light;
        targetEffect->data.hitEffectArt = sourceEffect->data.hitEffectArt;
        targetEffect->data.effectShader = sourceEffect->data.effectShader;
        targetEffect->data.hitVisuals = sourceEffect->data.hitVisuals;
        targetEffect->data.enchantShader = sourceEffect->data.enchantShader;
        targetEffect->data.enchantEffectArt = sourceEffect->data.enchantEffectArt;
        targetEffect->data.enchantVisuals = sourceEffect->data.enchantVisuals;
        targetEffect->data.projectileBase = sourceEffect->data.projectileBase;
        targetEffect->data.explosion = sourceEffect->data.explosion;
        targetEffect->data.impactDataSet = sourceEffect->data.impactDataSet;
        targetEffect->data.imageSpaceMod = sourceEffect->data.imageSpaceMod;
    }
}

void copyBookAppearence(RE::TESForm* source, RE::TESForm* target) {
    const auto* sourceBook = source->As<RE::TESObjectBOOK>();
    auto* targetBook = target->As<RE::TESObjectBOOK>();
    if (sourceBook && targetBook) {
        targetBook->inventoryModel = sourceBook->inventoryModel;
    }
}

void applyPattern(FormRecord* instance) {
    if (!instance) {
        return;
    }

    const auto baseForm = instance->baseForm;
    const auto newForm = instance->actualForm;

    if (baseForm && newForm) {
        const auto weaponBaseForm = baseForm->As<RE::TESObjectWEAP>();
        const auto weaponNewForm = newForm->As<RE::TESObjectWEAP>();

        const auto bookBaseForm = baseForm->As<RE::TESObjectBOOK>();
        const auto bookNewForm = newForm->As<RE::TESObjectBOOK>();

        const auto ammoBaseForm = baseForm->As<RE::TESAmmo>();
        const auto ammoNewForm = newForm->As<RE::TESAmmo>();

        if (weaponNewForm && weaponBaseForm) {
            weaponNewForm->firstPersonModelObject = weaponBaseForm->firstPersonModelObject;

            weaponNewForm->weaponData = weaponBaseForm->weaponData;
            weaponNewForm->criticalData = weaponBaseForm->criticalData;

            weaponNewForm->attackSound = weaponBaseForm->attackSound;
            weaponNewForm->attackSound2D = weaponBaseForm->attackSound2D;
            weaponNewForm->attackSound = weaponBaseForm->attackSound;
            weaponNewForm->attackFailSound = weaponBaseForm->attackFailSound;
            weaponNewForm->idleSound = weaponBaseForm->idleSound;
            weaponNewForm->equipSound = weaponBaseForm->equipSound;
            weaponNewForm->unequipSound = weaponBaseForm->unequipSound;
            weaponNewForm->soundLevel = weaponBaseForm->soundLevel;

            weaponNewForm->impactDataSet = weaponBaseForm->impactDataSet;
            weaponNewForm->templateWeapon = weaponBaseForm->templateWeapon;
            weaponNewForm->embeddedNode = weaponBaseForm->embeddedNode;
        } else if (bookBaseForm && bookNewForm) {
            bookNewForm->data.flags = bookBaseForm->data.flags;
            bookNewForm->data.teaches.spell = bookBaseForm->data.teaches.spell;
            bookNewForm->data.teaches.actorValueToAdvance = bookBaseForm->data.teaches.actorValueToAdvance;
            bookNewForm->data.type = bookBaseForm->data.type;
            bookNewForm->inventoryModel = bookBaseForm->inventoryModel;
            bookNewForm->itemCardDescription = bookBaseForm->itemCardDescription;
        } else if (ammoBaseForm && ammoNewForm) {
            ammoNewForm->GetRuntimeData().data.damage = ammoBaseForm->GetRuntimeData().data.damage;
            ammoNewForm->GetRuntimeData().data.flags = ammoBaseForm->GetRuntimeData().data.flags;
            ammoNewForm->GetRuntimeData().data.projectile = ammoBaseForm->GetRuntimeData().data.projectile;
        } else {
            newForm->Copy(baseForm);
        }

        copyComponent<RE::TESDescription>(baseForm, newForm);
        copyComponent<RE::BGSKeywordForm>(baseForm, newForm);
        copyComponent<RE::BGSPickupPutdownSounds>(baseForm, newForm);
        copyComponent<RE::TESModelTextureSwap>(baseForm, newForm);
        copyComponent<RE::TESModel>(baseForm, newForm);
        copyComponent<RE::BGSMessageIcon>(baseForm, newForm);
        copyComponent<RE::TESIcon>(baseForm, newForm);
        copyComponent<RE::TESFullName>(baseForm, newForm);
        copyComponent<RE::TESValueForm>(baseForm, newForm);
        copyComponent<RE::TESWeightForm>(baseForm, newForm);
        copyComponent<RE::BGSDestructibleObjectForm>(baseForm, newForm);
        copyComponent<RE::TESEnchantableForm>(baseForm, newForm);
        copyComponent<RE::BGSBlockBashData>(baseForm, newForm);
        copyComponent<RE::BGSEquipType>(baseForm, newForm);
        copyComponent<RE::TESAttackDamageForm>(baseForm, newForm);
        copyComponent<RE::TESBipedModelForm>(baseForm, newForm);
    }
    if (newForm && instance->modelForm) {
        copyAppearence(instance->modelForm, newForm);
    }
}

void copyAppearence(RE::TESForm* source, RE::TESForm* target) {
    copyFormArmorModel(source, target);
    copyFormObjectWeaponModel(source, target);
    copyMagicEffect(source, target);
    copyBookAppearence(source, target);
    copyComponent<RE::BGSPickupPutdownSounds>(source, target);
    copyComponent<RE::BGSMenuDisplayObject>(source, target);
    copyComponent<RE::TESModel>(source, target);
    copyComponent<RE::TESBipedModelForm>(source, target);
}


namespace {
    RE::TESForm* CreateFormInstance(const RE::FormType formType, const RE::FormID formId) {
        const auto factory = RE::IFormFactory::GetFormFactoryByType(formType);
        if (!factory) {
            logger::error("No form factory for form type {}", static_cast<uint32_t>(formType));
            return nullptr;
        }

        auto* result = factory->Create();
        if (!result) {
            logger::error("Factory returned null for form type {}", static_cast<uint32_t>(formType));
            return nullptr;
        }

        result->SetFormID(formId, false);
        return result;
    }

    FormRecord* FindCreatedRecord(const RE::FormID formId) {
        FormRecord* found = nullptr;
        EachFormData([&](FormRecord* item) {
            if (item && item->formId == formId) {
                found = item;
                return false;
            }
            return true;
        });
        return found;
    }

    RE::TESForm* CreateRegisteredForm(const uint32_t localId, const RE::FormType formType, RE::TESForm* baseItem,
        std::string owner = {}, std::string key = {}) {
        if (!espFound || localId == 0 || formType == RE::FormType::None) {
            return nullptr;
        }

        const auto existingSlot = GetRegisteredDynamicSlot(localId);
        if (existingSlot.has_value()) {
            if (existingSlot->formType != formType) {
                logger::error("Slot {:06X} is registered as type {}, requested {}", localId,
                    static_cast<uint32_t>(existingSlot->formType), static_cast<uint32_t>(formType));
                return nullptr;
            }
            if ((!owner.empty() || !key.empty()) && (existingSlot->owner != owner || existingSlot->key != key)) {
                logger::error("Slot {:06X} is owned by '{}'/'{}', requested '{}'/'{}'", localId,
                    existingSlot->owner, existingSlot->key, owner, key);
                return nullptr;
            }
        }

        const auto formId = MakeDynamicFormID(localId);
        if (auto* existingForm = RE::TESForm::LookupByID(formId)) {
            if (existingForm->GetFormType() != formType) {
                logger::error("FormID {:08X} exists as type {}, requested {}", formId,
                    static_cast<uint32_t>(existingForm->GetFormType()), static_cast<uint32_t>(formType));
                return nullptr;
            }
            if (!RegisterDynamicSlot(localId, formType, std::move(owner), std::move(key))) {
                return nullptr;
            }
            if (!FindCreatedRecord(formId)) {
                auto* record = FormRecord::CreateNew(existingForm, formType, formId);
                record->baseForm = baseItem;
                AddFormData(record);
            }
            SaveGlobalRegistry();
            return existingForm;
        }

        if (!RegisterDynamicSlot(localId, formType, std::move(owner), std::move(key))) {
            return nullptr;
        }

        auto* newForm = CreateFormInstance(formType, formId);
        if (!newForm) {
            return nullptr;
        }

        auto* record = FormRecord::CreateNew(newForm, formType, formId);
        record->baseForm = baseItem;
        if (baseItem) {
            applyPattern(record);
        }
        AddFormData(record);
        SaveGlobalRegistry();
        return newForm;
    }
}

RE::TESForm* AddForm(RE::TESForm* baseItem) {
    if (!baseItem) {
        logger::error("Create(baseItem) was called with a null baseItem. Use CreateByType for empty forms.");
        return nullptr;
    }

    const auto formId = AllocateDynamicFormID();
    if (formId == 0) {
        return nullptr;
    }

    return CreateRegisteredForm(ToDynamicLocalID(formId), baseItem->GetFormType(), baseItem);
}

RE::TESForm* AddFormByType(const RE::FormType formType) {
    if (formType == RE::FormType::None) {
        logger::error("CreateByType called with FormType::None");
        return nullptr;
    }

    const auto formId = AllocateDynamicFormID();
    if (formId == 0) {
        return nullptr;
    }

    return CreateRegisteredForm(ToDynamicLocalID(formId), formType, nullptr);
}

RE::TESForm* AddFormByTypeForOwner(const char* owner, const char* key, const RE::FormType formType) {
    return GetOrCreateFormByOwnerKey(owner, key, formType);
}

RE::TESForm* GetOrCreateFormByLocalId(const uint32_t localId, const RE::FormType formType) {
    return CreateRegisteredForm(localId & 0x00ffffff, formType, nullptr);
}

RE::TESForm* GetOrCreateFormByFormId(const RE::FormID formId, const RE::FormType formType) {
    if (!IsDynamicFormID(formId)) {
        logger::error("FormID {:08X} does not belong to Dynamic Persistent Forms.esp", formId);
        return nullptr;
    }
    return GetOrCreateFormByLocalId(ToDynamicLocalID(formId), formType);
}

RE::TESForm* GetOrCreateFormByOwnerKey(const char* ownerRaw, const char* keyRaw, const RE::FormType formType) {
    const auto owner = NormalizeOwnerKeyPart(ownerRaw);
    const auto key = NormalizeOwnerKeyPart(keyRaw);
    if (owner.empty() || key.empty()) {
        logger::error("Owner and key are required for owned DPF slots");
        return nullptr;
    }

    if (const auto existingLocalId = FindDynamicSlotByOwnerKey(owner, key)) {
        return CreateRegisteredForm(existingLocalId.value(), formType, nullptr, owner, key);
    }

    const auto formId = AllocateDynamicFormID();
    if (formId == 0) {
        return nullptr;
    }

    return CreateRegisteredForm(ToDynamicLocalID(formId), formType, nullptr, owner, key);
}

bool ReleaseFormByOwnerKey(const char* ownerRaw, const char* keyRaw) {
    const auto released = ReleaseDynamicSlotByOwnerKey(NormalizeOwnerKeyPart(ownerRaw), NormalizeOwnerKeyPart(keyRaw));
    if (released) {
        SaveGlobalRegistry();
    }
    return released;
}

bool ReleaseFormByLocalId(const uint32_t localId, const char* ownerRaw) {
    const auto released = ReleaseDynamicSlot(localId, NormalizeOwnerKeyPart(ownerRaw));
    if (released) {
        SaveGlobalRegistry();
    }
    return released;
}

uint32_t ReleaseFormsByOwner(const char* ownerRaw) {
    const auto released = ReleaseDynamicSlotsByOwner(NormalizeOwnerKeyPart(ownerRaw));
    if (released > 0) {
        SaveGlobalRegistry();
    }
    return released;
}
