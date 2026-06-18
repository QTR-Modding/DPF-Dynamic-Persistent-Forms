#pragma once

#include "../public/DPFAPI.h"
#include <mutex>

namespace Services {
    inline std::mutex serviceMutex;

    RE::TESForm* Create(RE::TESForm* baseItem);

    RE::TESForm* CreateByType(uint32_t formType);

    RE::TESForm* GetOrCreateByLocalId(uint32_t localId, uint32_t formType);

    RE::TESForm* GetOrCreateByFormId(RE::FormID formId, uint32_t formType);

    RE::TESForm* GetOrCreateByOwnerKey(const char* owner, const char* key, uint32_t formType, uint32_t* localId, bool* existed);

    bool ReleaseByOwnerKey(const char* owner, const char* key);

    bool ReleaseByLocalId(uint32_t localId, const char* owner);

    uint32_t ReleaseOwner(const char* owner);

    void Track(RE::TESForm* baseItem);

    void UnTrack(RE::TESForm* form);

    void Dispose(RE::TESForm* form);
}
