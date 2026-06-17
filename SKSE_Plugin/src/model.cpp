#include "model.h"
#include <algorithm>

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
    reservedDynamicLocalIds.insert(localId & 0x00ffffff);
    if (localId >= nextDynamicLocalId) {
        nextDynamicLocalId = localId + 1;
    }
}

void ReserveDynamicFormID(const RE::FormID formId) {
    if (IsDynamicFormID(formId)) {
        ReserveDynamicLocalID(ToDynamicLocalID(formId));
    }
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
        const bool existsInGame = RE::TESForm::LookupByID(fullId) != nullptr;
        if (!reserved && !existsInGame) {
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
    nextDynamicLocalId = firstDynamicLocalId;
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

    ResetDynamicState();
}
