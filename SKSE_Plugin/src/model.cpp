#include "model.h"
#include <algorithm>

namespace {
    void RemoveRuntimeRecordForLocalId(const uint32_t localId, const bool deleteActualForm) {
        const auto formId = MakeDynamicFormID(localId);
        for (auto it = formData.begin(); it != formData.end();) {
            auto* item = *it;
            if (item && item->formId == formId) {
                if (deleteActualForm && item->actualForm) {
                    item->actualForm->SetDelete(true);
                }
                delete item;
                it = formData.erase(it);
            } else {
                ++it;
            }
        }
    }

    void RemoveOwnerKeyIndex(const DynamicSlot& slot) {
        if (!slot.owner.empty() || !slot.key.empty()) {
            dynamicOwnerKeyIndex.erase(MakeOwnerKey(slot.owner, slot.key));
        }
    }
}

void AddFormData(FormRecord* item) {
    if (!item) {
        return;
    }
    if (item->order == 0) {
        item->order = nextRecordOrder++;
    } else if (item->order >= nextRecordOrder) {
        nextRecordOrder = item->order + 1;
    }
    formData.push_back(item);
    ReserveDynamicFormID(item->formId);
    if (IsDynamicFormID(item->formId) && item->formType != RE::FormType::None) {
        RegisterDynamicSlot(ToDynamicLocalID(item->formId), item->formType);
    }
}

void AddFormRef(FormRecord* item) {
    if (!item) {
        return;
    }
    if (item->order == 0) {
        item->order = nextRecordOrder++;
    } else if (item->order >= nextRecordOrder) {
        nextRecordOrder = item->order + 1;
    }
    formRef.push_back(item);
}

void EachFormData(const std::function<bool(FormRecord*)>& iteration) {
    for (const auto item : formData) {
        if (!iteration(item)) {
            return;
        }
    }
}

void EachFormRef(const std::function<bool(FormRecord*)>& iteration) {
    for (const auto item : formRef) {
        if (!iteration(item)) {
            return;
        }
    }
}

bool IsDynamicFormID(const RE::FormID formId) {
    if (!espFound || formId == 0) {
        return false;
    }
    return ((formId >> 24) & 0xff) == dynamicModId;
}

uint32_t ToDynamicLocalID(const RE::FormID formId) {
    return formId & 0x00ffffff;
}

RE::FormID MakeDynamicFormID(const uint32_t localId) {
    return (dynamicModId << 24) | (localId & 0x00ffffff);
}

void ReserveDynamicLocalID(const uint32_t localId) {
    if (localId == 0) {
        return;
    }
    const auto normalized = localId & 0x00ffffff;
    reservedDynamicLocalIds.insert(normalized);
    if (normalized >= nextDynamicLocalId) {
        nextDynamicLocalId = normalized + 1;
    }
}

void ReserveDynamicFormID(const RE::FormID formId) {
    if (IsDynamicFormID(formId)) {
        ReserveDynamicLocalID(ToDynamicLocalID(formId));
    }
}

std::string NormalizeOwnerKeyPart(const char* value) {
    return value ? std::string(value) : std::string();
}

std::string MakeOwnerKey(std::string_view owner, std::string_view key) {
    std::string result;
    result.reserve(owner.size() + key.size() + 1);
    result.append(owner.data(), owner.size());
    result.push_back('\x1f');
    result.append(key.data(), key.size());
    return result;
}

bool RegisterDynamicSlot(const uint32_t localId, const RE::FormType formType, std::string owner, std::string key) {
    const auto normalized = localId & 0x00ffffff;
    if (normalized == 0 || formType == RE::FormType::None) {
        return false;
    }

    const auto ownerKey = MakeOwnerKey(owner, key);
    const bool hasOwnerKey = !owner.empty() || !key.empty();
    if (hasOwnerKey) {
        const auto existingByOwner = dynamicOwnerKeyIndex.find(ownerKey);
        if (existingByOwner != dynamicOwnerKeyIndex.end() && existingByOwner->second != normalized) {
            logger::error("Owner/key '{}'/'{}' is already registered to slot {:06X}", owner, key, existingByOwner->second);
            return false;
        }
    }

    const auto existing = dynamicSlots.find(normalized);
    if (existing != dynamicSlots.end()) {
        const auto& slot = existing->second;
        if (slot.formType != formType) {
            logger::error("Dynamic slot {:06X} is already registered as type {}, requested {}", normalized,
                static_cast<uint32_t>(slot.formType), static_cast<uint32_t>(formType));
            return false;
        }
        if ((!owner.empty() || !key.empty()) && (slot.owner != owner || slot.key != key)) {
            logger::error("Dynamic slot {:06X} is owned by '{}'/'{}', requested '{}'/'{}'", normalized,
                slot.owner, slot.key, owner, key);
            return false;
        }
        ReserveDynamicLocalID(normalized);
        return true;
    }

    DynamicSlot slot;
    slot.localId = normalized;
    slot.formType = formType;
    slot.owner = std::move(owner);
    slot.key = std::move(key);
    dynamicSlots[normalized] = slot;
    if (!slot.owner.empty() || !slot.key.empty()) {
        dynamicOwnerKeyIndex[MakeOwnerKey(slot.owner, slot.key)] = normalized;
    }
    ReserveDynamicLocalID(normalized);
    return true;
}

std::optional<DynamicSlot> GetRegisteredDynamicSlot(const uint32_t localId) {
    const auto existing = dynamicSlots.find(localId & 0x00ffffff);
    if (existing == dynamicSlots.end()) {
        return std::nullopt;
    }
    return existing->second;
}

std::optional<RE::FormType> GetRegisteredDynamicSlotType(const uint32_t localId) {
    const auto slot = GetRegisteredDynamicSlot(localId);
    if (!slot.has_value()) {
        return std::nullopt;
    }
    return slot->formType;
}

std::optional<uint32_t> FindDynamicSlotByOwnerKey(std::string_view owner, std::string_view key) {
    const auto existing = dynamicOwnerKeyIndex.find(MakeOwnerKey(owner, key));
    if (existing == dynamicOwnerKeyIndex.end()) {
        return std::nullopt;
    }
    return existing->second;
}

bool IsDynamicLocalIDRegistered(const uint32_t localId) {
    return dynamicSlots.contains(localId & 0x00ffffff);
}

bool ReleaseDynamicSlot(const uint32_t localId, std::string_view owner) {
    const auto normalized = localId & 0x00ffffff;
    const auto existing = dynamicSlots.find(normalized);
    if (existing == dynamicSlots.end()) {
        return false;
    }
    if (existing->second.owner != owner) {
        logger::error("Owner '{}' cannot release slot {:06X} owned by '{}'", owner, normalized, existing->second.owner);
        return false;
    }

    RemoveRuntimeRecordForLocalId(normalized, true);
    RemoveOwnerKeyIndex(existing->second);
    dynamicSlots.erase(existing);
    reservedDynamicLocalIds.erase(normalized);
    nextDynamicLocalId = std::min(nextDynamicLocalId, normalized);
    return true;
}

bool ReleaseDynamicSlotByOwnerKey(std::string_view owner, std::string_view key) {
    const auto localId = FindDynamicSlotByOwnerKey(owner, key);
    if (!localId.has_value()) {
        return false;
    }
    return ReleaseDynamicSlot(localId.value(), owner);
}

uint32_t ReleaseDynamicSlotsByOwner(std::string_view owner) {
    std::vector<uint32_t> toRelease;
    for (const auto& [localId, slot] : dynamicSlots) {
        if (slot.owner == owner) {
            toRelease.push_back(localId);
        }
    }

    uint32_t released = 0;
    for (const auto localId : toRelease) {
        if (ReleaseDynamicSlot(localId, owner)) {
            ++released;
        }
    }
    return released;
}

RE::FormID AllocateDynamicFormID() {
    if (!espFound) {
        logger::error("Cannot allocate DynamicPersistentForms FormID because Dynamic Persistent Forms.esp was not found");
        return 0;
    }

    uint32_t localId = std::max(nextDynamicLocalId, firstDynamicLocalId);
    while (localId < 0x00ffffff) {
        const auto fullId = MakeDynamicFormID(localId);
        const bool reserved = reservedDynamicLocalIds.contains(localId);
        const bool registered = dynamicSlots.contains(localId);
        const bool existsInGame = RE::TESForm::LookupByID(fullId) != nullptr;
        if (!reserved && !registered && !existsInGame) {
            ReserveDynamicLocalID(localId);
            logger::info("Allocated DynamicPersistentForms FormID {:08X}", fullId);
            return fullId;
        }
        ++localId;
    }

    logger::error("Dynamic Persistent Forms.esp has no available FormID slots");
    return 0;
}

void ReadFirstFormIdFromESP() {
    const auto dataHandler = RE::TESDataHandler::GetSingleton();
    espFound = false;
    dynamicModId = 0;
    firstDynamicLocalId = 0x801;
    nextDynamicLocalId = std::max(nextDynamicLocalId, firstDynamicLocalId);
    dynamicPluginName = "Dynamic Persistent Forms.esp";

    const auto modIndex = dataHandler->GetLoadedModIndex(dynamicPluginName);
    if (!modIndex.has_value()) {
        logger::error("Dynamic Persistent Forms.esp was not found");
        return;
    }

    espFound = true;
    dynamicModId = modIndex.value();
    logger::info("Dynamic Persistent Forms.esp found at mod index {:02X}", dynamicModId);
}

void ResetDynamicState() {
    reservedDynamicLocalIds.clear();
    dynamicSlots.clear();
    dynamicOwnerKeyIndex.clear();
    nextDynamicLocalId = firstDynamicLocalId;
    nextRecordOrder = 1;
}

void ClearRecords(const bool deleteActualForms) {
    while (!formRef.empty()) {
        delete formRef.back();
        formRef.pop_back();
    }

    while (!formData.empty()) {
        const auto item = formData.back();
        if (deleteActualForms && item && item->actualForm) {
            item->actualForm->SetDelete(true);
        }
        delete item;
        formData.pop_back();
    }

    reservedDynamicLocalIds.clear();
    nextRecordOrder = 1;
    for (const auto& [localId, slot] : dynamicSlots) {
        ReserveDynamicLocalID(localId);
    }
}
