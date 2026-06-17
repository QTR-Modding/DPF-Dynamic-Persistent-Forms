#pragma once

#include "../public/DPFAPI.h"
#include <mutex>

namespace Services {
    inline std::mutex serviceMutex;

    RE::TESForm* Create(RE::TESForm* baseItem);

    RE::TESForm* CreateByType(uint32_t formType);

    void Track(RE::TESForm* baseItem);

    void UnTrack(RE::TESForm* form);

    void Dispose(RE::TESForm* form);
}

