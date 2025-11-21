#pragma once
#include <vector>
#include <functional>
#include <cstdint>

inline std::vector<FormRecord*> formData;
inline std::vector<FormRecord*> formRef;

inline bool espFound = false;
inline uint32_t lastFormId = 0;
inline uint32_t firstFormId = 0;
inline uint32_t dynamicModId = 0;


inline void AddFormData(FormRecord* item) {
    if (!item) {
        return;
    }
    formData.push_back(item);
}

inline void AddFormRef(FormRecord* item) {
    if (!item) {
        return;
    }
    formRef.push_back(item);
}

inline void EachFormData(std::function<bool(FormRecord*)> const& iteration) {
    for (const auto item : formData) {
        if (!iteration(item)) {
            return;
        }
    }
}

inline void EachFormRef(std::function<bool(FormRecord*)> const& iteration) {
    for (const auto item : formRef) {
        if (!iteration(item)) {
            return;
        }
    }
}


inline void incrementLastFormID() {
    ++lastFormId;
}


inline void UpdateId() {
    EachFormData([&](FormRecord* item) {
        if (item->formId > lastFormId) {
            lastFormId = item->formId;
        }
        return true;
    });
    incrementLastFormID();
}

inline void ResetId() {
    lastFormId = firstFormId;
}

inline void ReadFirstFormIdFromESP() {
    const auto dataHandler = RE::TESDataHandler::GetSingleton();

    const auto id = dataHandler->LookupFormID(0x800, "Dynamic Persistent Forms.esp");

    if (id != 0) {
        espFound = true;
    }

    firstFormId = id + 1;
    lastFormId = firstFormId;
    dynamicModId = (firstFormId >> 24) & 0xff;
}