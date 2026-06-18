#include "persistence.h"
#include "model.h"
#include "serializer.h"
#include <algorithm>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace {
    constexpr uint32_t kRegistryMagic = 0x44504643;  // DPFC
    constexpr uint32_t kRegistrySchemaVersion = 1;
    constexpr uint32_t kNoOwnerIndex = std::numeric_limits<uint32_t>::max();
    std::mutex registryMutex;

    template <typename T>
    void WriteString(Serializer<T>* serializer, const std::string& value) {
        serializer->WriteString(value.c_str());
    }

    template <typename T>
    std::string ReadStoredString(Serializer<T>* serializer) {
        const auto value = serializer->ReadString();
        std::string result = value ? value : "";
        delete[] value;
        return result;
    }

    std::vector<DynamicSlot> GetSortedSlots() {
        std::vector<DynamicSlot> sorted;
        sorted.reserve(dynamicSlots.size());
        for (const auto& [localId, slot] : dynamicSlots) {
            sorted.push_back(slot);
        }
        std::ranges::sort(sorted, [](const auto& lhs, const auto& rhs) {
            return lhs.localId < rhs.localId;
        });
        return sorted;
    }

    void RecalculateNextDynamicLocalId() {
        uint32_t localId = firstDynamicLocalId;
        while (localId < 0x00ffffff) {
            if (!reservedDynamicLocalIds.contains(localId) && !dynamicSlots.contains(localId)) {
                nextDynamicLocalId = localId;
                return;
            }
            ++localId;
        }

        nextDynamicLocalId = 0x00ffffff;
    }

    template <typename T>
    void StoreRegistry(Serializer<T>* serializer) {
        const auto sorted = GetSortedSlots();

        std::unordered_map<std::string, uint32_t> ownerIndexes;
        std::vector<std::string> owners;
        for (const auto& slot : sorted) {
            if (slot.owner.empty() || ownerIndexes.contains(slot.owner)) {
                continue;
            }

            const auto index = static_cast<uint32_t>(owners.size());
            ownerIndexes[slot.owner] = index;
            owners.push_back(slot.owner);
        }

        serializer->template Write<uint32_t>(kRegistryMagic);
        serializer->template Write<uint32_t>(kRegistrySchemaVersion);
        WriteString(serializer, dynamicPluginName);

        serializer->template Write<uint32_t>(static_cast<uint32_t>(owners.size()));
        for (const auto& owner : owners) {
            WriteString(serializer, owner);
        }

        serializer->template Write<uint32_t>(static_cast<uint32_t>(sorted.size()));
        for (const auto& slot : sorted) {
            serializer->template Write<uint32_t>(slot.localId);
            serializer->template Write<uint32_t>(static_cast<uint32_t>(slot.formType));
            serializer->template Write<uint32_t>(slot.owner.empty() ? kNoOwnerIndex : ownerIndexes[slot.owner]);
            WriteString(serializer, slot.key);
        }
    }

    template <typename T>
    bool RestoreRegistry(Serializer<T>* serializer) {
        const auto magic = serializer->template Read<uint32_t>();
        const auto version = serializer->template Read<uint32_t>();
        if (magic != kRegistryMagic || version != kRegistrySchemaVersion) {
            logger::warn("DPF registry binary cache is missing or unsupported; starting with empty schema {} registry", kRegistrySchemaVersion);
            ResetDynamicState();
            return SaveGlobalRegistry();
        }

        ResetDynamicState();
        dynamicPluginName = ReadStoredString(serializer);

        std::vector<std::string> owners;
        const auto ownerCount = serializer->template Read<uint32_t>();
        owners.reserve(ownerCount);
        for (uint32_t i = 0; i < ownerCount; ++i) {
            owners.push_back(ReadStoredString(serializer));
        }

        const auto slotCount = serializer->template Read<uint32_t>();
        for (uint32_t i = 0; i < slotCount; ++i) {
            const auto localId = serializer->template Read<uint32_t>();
            const auto formType = static_cast<RE::FormType>(serializer->template Read<uint32_t>());
            const auto ownerIndex = serializer->template Read<uint32_t>();
            std::string owner;
            if (ownerIndex != kNoOwnerIndex && ownerIndex < owners.size()) {
                owner = owners[ownerIndex];
            }
            const auto key = ReadStoredString(serializer);

            if (!RegisterDynamicSlot(localId, formType, owner, key)) {
                logger::warn("Ignoring invalid DPF registry slot {:06X}", localId);
            }
        }

        RecalculateNextDynamicLocalId();
        logger::info("Loaded DPF global registry with {} slots", dynamicSlots.size());
        return true;
    }
}

std::string GetGlobalRegistryPath() {
    return "Data/SKSE/Plugins/DPF_Cache.bin";
}

bool LoadGlobalRegistry() {
    std::lock_guard lock(registryMutex);
    const auto path = GetGlobalRegistryPath();
    if (!fs::exists(path)) {
        logger::info("DPF global registry not found at {}; starting empty", path);
        ResetDynamicState();
        return SaveGlobalRegistry();
    }

    FileReader fileReader(path, std::ios::in | std::ios::binary);
    if (!fileReader.IsOpen()) {
        logger::error("Could not open DPF global registry at {}", path);
        return false;
    }

    return RestoreRegistry(&fileReader);
}

bool SaveGlobalRegistry() {
    const auto path = GetGlobalRegistryPath();
    try {
        const fs::path registryPath(path);
        if (registryPath.has_parent_path()) {
            fs::create_directories(registryPath.parent_path());
        }

        FileWriter fileWriter(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!fileWriter.IsOpen()) {
            logger::error("Could not write DPF global registry at {}", path);
            return false;
        }

        StoreRegistry(&fileWriter);
        logger::debug("Saved DPF global registry to {}", path);
        return true;
    } catch (const std::exception& e) {
        logger::error("Error saving DPF global registry: {}", e.what());
        return false;
    }
}

void SaveCallback(SKSE::SerializationInterface*) {
    std::lock_guard lock(registryMutex);
    SaveGlobalRegistry();
}

void LoadCallback(SKSE::SerializationInterface*) {
    LoadGlobalRegistry();
}
