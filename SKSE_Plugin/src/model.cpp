#include "model.h"

void AddFormData(FormRecord* item) {
    if (!item) {
        return;
    }
    formData.push_back(item);
}

void AddFormRef(FormRecord* item) {
    if (!item) {
        return;
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

void incrementLastFormID() {
    ++lastFormId;
}

void UpdateId() {
    EachFormData([&](FormRecord* item) {
        if (item->formId > lastFormId) {
            lastFormId = item->formId;
        }
        return true;
    });
    incrementLastFormID();
}

void ResetId() {
    lastFormId = firstFormId;
}

void ReadFirstFormIdFromESP() {
    const auto dataHandler = RE::TESDataHandler::GetSingleton();

    const auto id = dataHandler->LookupFormID(0x800, "Dynamic Persistent Forms.esp");

    if (id != 0) {
        espFound = true;
    }

    firstFormId = id + 1;
    lastFormId = firstFormId;
    dynamicModId = (firstFormId >> 24) & 0xff;
}