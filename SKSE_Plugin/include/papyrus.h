#pragma once


RE::TESForm* Create(RE::StaticFunctionTag*, RE::TESForm* baseItem);

void Track(RE::StaticFunctionTag*, RE::TESForm* baseItem);

void UnTrack(RE::StaticFunctionTag*, RE::TESForm* form);


void AddMagicEffect(RE::StaticFunctionTag*, RE::TESForm* item, RE::EffectSetting* effect, float magnitude,
                    uint32_t area, uint32_t duration, float cost);


void CopyMagicEffects(RE::StaticFunctionTag*, RE::TESForm* from, RE::TESForm* to);


void Dispose(RE::StaticFunctionTag*, RE::TESForm* form);

void ClearMagicEffects(RE::StaticFunctionTag*, RE::TESForm* item);


void CopyAppearance(RE::StaticFunctionTag*, RE::TESForm* source, RE::TESForm* target);

void SetSpellTomeSpell(RE::StaticFunctionTag*, RE::TESObjectBOOK* spellTome, RE::SpellItem* spell);

void SetSpellAutoCalculate(RE::StaticFunctionTag*, RE::SpellItem* spell, bool autoCalculate);

void SetSpellCostOverride(RE::StaticFunctionTag*, RE::SpellItem* spell, uint32_t costOverride);

void SetSpellChargeTime(RE::StaticFunctionTag*, RE::SpellItem* spell, float chargeTime);

void SetSpellCastDuration(RE::StaticFunctionTag*, RE::SpellItem* spell, float castDuration);

void SetSpellRange(RE::StaticFunctionTag*, RE::SpellItem* spell, float range);

void SetSpellCastingPerk(RE::StaticFunctionTag*, RE::SpellItem* spell, RE::BGSPerk* perk);


void SetEnchantmentAutoCalculate(RE::StaticFunctionTag*, RE::EnchantmentItem* spell, bool autoCalculate);

void SetEnchantmentChargeOverride(RE::StaticFunctionTag*, RE::EnchantmentItem* spell, uint32_t costOverride);

void SetEnchantmentCostOverride(RE::StaticFunctionTag*, RE::EnchantmentItem* spell, uint32_t costOverride);

void SetEnchantmentChargeTime(RE::StaticFunctionTag*, RE::EnchantmentItem* spell, float chargeTime);


void SetAmmoDamage(RE::StaticFunctionTag*, RE::TESAmmo* ammo, float damage);

void SetAmmoProjectile(RE::StaticFunctionTag*, RE::TESAmmo* ammo, RE::BGSProjectile* projectile);


void SetSoulGemCapacity(RE::StaticFunctionTag*, RE::TESSoulGem* soulGem, uint32_t capacity);

void SetSoulGemCurrentSoul(RE::StaticFunctionTag*, RE::TESSoulGem* soulGem, uint32_t capacity);

void LinkSoulGems(RE::StaticFunctionTag*, RE::TESSoulGem* empty, RE::TESSoulGem* filled);

bool PapyrusFunctions(RE::BSScript::IVirtualMachine* vm);