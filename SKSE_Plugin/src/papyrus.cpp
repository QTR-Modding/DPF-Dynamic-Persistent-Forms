#include "papyrus.h"
#include "model.h"
#include "Services.h"
#include "form.h"

std::mutex papyrusMutex;

RE::TESForm* Create(RE::StaticFunctionTag*, RE::TESForm* baseItem) {
    return Services::Create(baseItem);
}

void Track(RE::StaticFunctionTag*, RE::TESForm* baseItem) {
    Services::Track(baseItem);
}

void UnTrack(RE::StaticFunctionTag*, RE::TESForm* form) {
    Services::UnTrack(form);
}

void AddMagicEffect(RE::StaticFunctionTag*, RE::TESForm* item, RE::EffectSetting* effect, const float magnitude,
                    const uint32_t area, const uint32_t duration, const float cost) {
    std::lock_guard lock(papyrusMutex);
    try {
        logger::trace("added");

        if (!item || !effect) {
            return;
        }

        const auto magicItem = item->As<RE::MagicItem>();

        if (magicItem) {
            const auto newEffect = new RE::Effect();
            newEffect->cost = cost;
            newEffect->baseEffect = effect;
            newEffect->effectItem.magnitude = magnitude;
            newEffect->effectItem.area = area;
            newEffect->effectItem.duration = duration;
            magicItem->effects.push_back(newEffect);
        }
    } catch (const std::exception&) {
    }
}

void CopyMagicEffects(RE::StaticFunctionTag*, RE::TESForm* from, RE::TESForm* to) {
    std::lock_guard lock(papyrusMutex);
    try {
        logger::trace("added");

        if (!from || !to) {
            return;
        }

        const auto magicItemFrom = from->As<RE::MagicItem>();
        const auto magicItemTo = to->As<RE::MagicItem>();

        if (magicItemFrom && magicItemTo) {
            for (const auto item : magicItemFrom->effects) {
                auto newEffect = new RE::Effect();
                newEffect->cost = item->cost;
                newEffect->baseEffect = item->baseEffect;
                newEffect->effectItem.magnitude = item->effectItem.magnitude;
                newEffect->effectItem.area = item->effectItem.area;
                newEffect->effectItem.duration = item->effectItem.duration;
                magicItemTo->effects.push_back(newEffect);
            }
        }
    } catch (const std::exception&) {
    }
}

void Dispose(RE::StaticFunctionTag*, RE::TESForm* form) {
    Services::Dispose(form);
}

void ClearMagicEffects(RE::StaticFunctionTag*, RE::TESForm* item) {
    std::lock_guard lock(papyrusMutex);
    try {
        if (!item) {
            return;
        }

        logger::trace("added");

        const auto magicItem = item->As<RE::MagicItem>();

        if (magicItem) {
            magicItem->effects.clear();
        }
    } catch (const std::exception&) {
    }
}

void CopyAppearance(RE::StaticFunctionTag*, RE::TESForm* source, RE::TESForm* target) {
    std::lock_guard lock(papyrusMutex);
    try {
        if (!source || !target) {
            return;
        }

        EachFormData([&](FormRecord* item) {
            if (!item->deleted && item->Match(target)) {
                item->modelForm = source;
                return false;
            }
            return true;
        });

        EachFormRef([&](FormRecord* item) {
            if (!item->deleted && item->Match(target)) {
                item->modelForm = source;
                return false;
            }
            return true;
        });

        copyAppearence(source, target);
    } catch (const std::exception&) {
        logger::error("error copying model");
    }
}

void SetSpellTomeSpell(RE::StaticFunctionTag*, RE::TESObjectBOOK* spellTome, RE::SpellItem* spell) {
    std::lock_guard lock(papyrusMutex);
    try {
        if (!spellTome || !spell) {
            return;
        }
        spellTome->data.flags = RE::OBJ_BOOK::Flag::kTeachesSpell;
        spellTome->data.teaches.spell = spell;
    } catch (const std::exception&) {
        logger::error("add spell");
    }
}

void SetSpellAutoCalculate(RE::StaticFunctionTag*, RE::SpellItem* spell, const bool autoCalculate) {
    std::lock_guard lock(papyrusMutex);

    if (!spell) {
        return;
    }

    if (!autoCalculate) {
        spell->data.flags |= RE::SpellItem::SpellFlag::kCostOverride;
    } else {
        spell->data.flags &= static_cast<RE::SpellItem::SpellFlag>(~static_cast<uint32_t>(
            RE::SpellItem::SpellFlag::kCostOverride));
    }
}

void SetSpellCostOverride(RE::StaticFunctionTag*, RE::SpellItem* spell, const uint32_t costOverride) {
    std::lock_guard lock(papyrusMutex);

    if (!spell) {
        return;
    }
    spell->data.flags |= RE::SpellItem::SpellFlag::kCostOverride;
    spell->data.costOverride = costOverride;
}

void SetSpellChargeTime(RE::StaticFunctionTag*, RE::SpellItem* spell, const float chargeTime) {
    std::lock_guard lock(papyrusMutex);

    if (!spell) {
        return;
    }
    spell->data.flags |= RE::SpellItem::SpellFlag::kCostOverride;
    spell->data.chargeTime = chargeTime;
}

void SetSpellCastDuration(RE::StaticFunctionTag*, RE::SpellItem* spell, const float castDuration) {
    std::lock_guard lock(papyrusMutex);

    if (!spell) {
        return;
    }
    spell->data.flags |= RE::SpellItem::SpellFlag::kCostOverride;
    spell->data.castDuration = castDuration;
}

void SetSpellRange(RE::StaticFunctionTag*, RE::SpellItem* spell, const float range) {
    std::lock_guard lock(papyrusMutex);

    if (!spell) {
        return;
    }

    spell->data.range = range;
}

void SetSpellCastingPerk(RE::StaticFunctionTag*, RE::SpellItem* spell, RE::BGSPerk* perk) {
    std::lock_guard lock(papyrusMutex);

    if (!spell || !perk) {
        return;
    }

    spell->data.castingPerk = perk;
}

void SetEnchantmentAutoCalculate(RE::StaticFunctionTag*, RE::EnchantmentItem* spell, const bool autoCalculate) {
    std::lock_guard lock(papyrusMutex);

    if (!spell) {
        return;
    }

    if (!autoCalculate) {
        spell->data.flags |= RE::EnchantmentItem::EnchantmentFlag::kCostOverride;
    } else {
        spell->data.flags &=
            static_cast<RE::EnchantmentItem::EnchantmentFlag>(~static_cast<uint32_t>(
                RE::EnchantmentItem::EnchantmentFlag::kCostOverride));
    }
}

void SetEnchantmentChargeOverride(RE::StaticFunctionTag*, RE::EnchantmentItem* spell, const uint32_t costOverride) {
    std::lock_guard lock(papyrusMutex);

    if (!spell) {
        return;
    }
    spell->data.flags |= RE::EnchantmentItem::EnchantmentFlag::kCostOverride;
    spell->data.chargeOverride = costOverride;
}

void SetEnchantmentCostOverride(RE::StaticFunctionTag*, RE::EnchantmentItem* spell, const uint32_t costOverride) {
    std::lock_guard lock(papyrusMutex);

    if (!spell) {
        return;
    }

    spell->data.flags |= RE::EnchantmentItem::EnchantmentFlag::kCostOverride;
    spell->data.costOverride = costOverride;
}

void SetEnchantmentChargeTime(RE::StaticFunctionTag*, RE::EnchantmentItem* spell, const float chargeTime) {
    std::lock_guard lock(papyrusMutex);

    if (!spell) {
        return;
    }
    spell->data.chargeTime = chargeTime;
}

void SetAmmoDamage(RE::StaticFunctionTag*, RE::TESAmmo* ammo, const float damage) {
    std::lock_guard lock(papyrusMutex);

    if (!ammo) {
        return;
    }
    ammo->GetRuntimeData().data.damage = damage;
}

void SetAmmoProjectile(RE::StaticFunctionTag*, RE::TESAmmo* ammo, RE::BGSProjectile* projectile) {
    std::lock_guard lock(papyrusMutex);

    if (!ammo || !projectile) {
        return;
    }
    ammo->GetRuntimeData().data.projectile = projectile;
}

void SetSoulGemCapacity(RE::StaticFunctionTag*, RE::TESSoulGem* soulGem, uint32_t capacity) {
    std::lock_guard lock(papyrusMutex);

    const auto value = static_cast<RE::SOUL_LEVEL>(capacity);

    if (!soulGem || value > RE::SOUL_LEVEL::kGrand || value < RE::SOUL_LEVEL::kNone) {
        return;
    }

    soulGem->soulCapacity = value;
}

void SetSoulGemCurrentSoul(RE::StaticFunctionTag*, RE::TESSoulGem* soulGem, uint32_t capacity) {
    std::lock_guard lock(papyrusMutex);

    const auto value = static_cast<RE::SOUL_LEVEL>(capacity);

    if (!soulGem || value > RE::SOUL_LEVEL::kGrand || value < RE::SOUL_LEVEL::kNone) {
        return;
    }

    soulGem->currentSoul = value;
}

void LinkSoulGems(RE::StaticFunctionTag*, RE::TESSoulGem* empty, RE::TESSoulGem* filled) {
    std::lock_guard lock(papyrusMutex);

    if (!empty || !filled) {
        return;
    }

    empty->linkedSoulGem = filled;
}

bool PapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("Create", "DynamicPersistentForms", Create);
    vm->RegisterFunction("Dispose", "DynamicPersistentForms", Dispose);
    vm->RegisterFunction("Track", "DynamicPersistentForms", Track);
    vm->RegisterFunction("UnTrack", "DynamicPersistentForms", UnTrack);

    vm->RegisterFunction("AddMagicEffect", "DynamicPersistentForms", AddMagicEffect);
    vm->RegisterFunction("ClearMagicEffects", "DynamicPersistentForms", ClearMagicEffects);
    vm->RegisterFunction("CopyAppearance", "DynamicPersistentForms", CopyAppearance);
    vm->RegisterFunction("CopyMagicEffects", "DynamicPersistentForms", CopyMagicEffects);
    vm->RegisterFunction("SetSpellTomeSpell", "DynamicPersistentForms", SetSpellTomeSpell);

    vm->RegisterFunction("SetSpellCostOverride", "DynamicPersistentForms", SetSpellCostOverride);
    vm->RegisterFunction("SetSpellChargeTime", "DynamicPersistentForms", SetSpellChargeTime);
    vm->RegisterFunction("SetSpellCastDuration", "DynamicPersistentForms", SetSpellCastDuration);
    vm->RegisterFunction("SetSpellRange", "DynamicPersistentForms", SetSpellRange);
    vm->RegisterFunction("SetSpellCastingPerk", "DynamicPersistentForms", SetSpellCastingPerk);
    vm->RegisterFunction("SetSpellAutoCalculate", "DynamicPersistentForms", SetSpellAutoCalculate);

    vm->RegisterFunction("SetEnchantmentAutoCalculate", "DynamicPersistentForms", SetEnchantmentAutoCalculate);
    vm->RegisterFunction("SetEnchantmentChargeOverride", "DynamicPersistentForms", SetEnchantmentChargeOverride);
    vm->RegisterFunction("SetEnchantmentCostOverride", "DynamicPersistentForms", SetEnchantmentCostOverride);
    vm->RegisterFunction("SetEnchantmentChargeTime", "DynamicPersistentForms", SetEnchantmentChargeTime);

    vm->RegisterFunction("SetAmmoDamage", "DynamicPersistentForms", SetAmmoDamage);
    vm->RegisterFunction("SetAmmoProjectile", "DynamicPersistentForms", SetAmmoProjectile);

    vm->RegisterFunction("SetSoulGemCapacity", "DynamicPersistentForms", SetSoulGemCapacity);
    vm->RegisterFunction("SetSoulGemCurrentSoul", "DynamicPersistentForms", SetSoulGemCurrentSoul);
    vm->RegisterFunction("LinkSoulGems", "DynamicPersistentForms", LinkSoulGems);

    return true;
}