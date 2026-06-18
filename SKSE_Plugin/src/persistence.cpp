#include "persistence.h"
#include "model.h"
#include "serializer.h"
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace {
    constexpr uint32_t kRegistryMagic = 0x44504643;  // DPFC
    constexpr uint8_t kRegistrySchemaVersion = 2;
    std::mutex registryMutex;

    template <typename T>
    void WriteVarUInt(Serializer<T>* serializer, uint32_t value) {
        while (value >= 0x80) {
            serializer->template Write<uint8_t>(static_cast<uint8_t>(value | 0x80));
            value >>= 7;
        }
        serializer->template Write<uint8_t>(static_cast<uint8_t>(value));
    }

    template <typename T>
    uint32_t ReadVarUInt(Serializer<T>* serializer) {
        uint32_t result = 0;
        uint32_t shift = 0;
        while (shift < 32) {
            const auto byte = serializer->template Read<uint8_t>();
            result |= static_cast<uint32_t>(byte & 0x7f) << shift;
            if ((byte & 0x80) == 0) {
                return result;
            }
            shift += 7;
        }
        logger::error("DPF registry cache has an invalid varint");
        return 0;
    }

    template <typename T>
    void WriteCompactString(Serializer<T>* serializer, const std::string& value) {
        WriteVarUInt(serializer, static_cast<uint32_t>(value.size()));
        for (const auto ch : value) {
            serializer->template Write<char>(ch);
        }
    }

    template <typename T>
    std::string ReadCompactString(Serializer<T>* serializer) {
        const auto size = ReadVarUInt(serializer);
        std::string result;
        result.resize(size);
        for (uint32_t i = 0; i < size; ++i) {
            result[i] = serializer->template Read<char>();
        }
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
        serializer->template Write<uint8_t>(kRegistrySchemaVersion);
        WriteCompactString(serializer, dynamicPluginName);

        WriteVarUInt(serializer, static_cast<uint32_t>(owners.size()));
        for (const auto& owner : owners) {
            WriteCompactString(serializer, owner);
        }

        WriteVarUInt(serializer, static_cast<uint32_t>(sorted.size()));
        for (const auto& slot : sorted) {
            WriteVarUInt(serializer, slot.localId);
            WriteVarUInt(serializer, static_cast<uint32_t>(slot.formType));
            WriteVarUInt(serializer, slot.owner.empty() ? 0 : ownerIndexes[slot.owner] + 1);
            WriteCompactString(serializer, slot.key);
        }
    }

    template <typename T>
    bool RestoreRegistry(Serializer<T>* serializer) {
        const auto magic = serializer->template Read<uint32_t>();
        const auto version = serializer->template Read<uint8_t>();
        if (magic != kRegistryMagic || version != kRegistrySchemaVersion) {
            logger::warn("DPF registry binary cache is missing or unsupported; starting with empty schema {} registry", kRegistrySchemaVersion);
            ResetDynamicState();
            return SaveGlobalRegistry();
        }

        ResetDynamicState();
        dynamicPluginName = ReadCompactString(serializer);

        std::vector<std::string> owners;
        const auto ownerCount = ReadVarUInt(serializer);
        owners.reserve(ownerCount);
        for (uint32_t i = 0; i < ownerCount; ++i) {
            owners.push_back(ReadCompactString(serializer));
        }

        const auto slotCount = ReadVarUInt(serializer);
        for (uint32_t i = 0; i < slotCount; ++i) {
            const auto localId = ReadVarUInt(serializer);
            const auto formType = static_cast<RE::FormType>(ReadVarUInt(serializer));
            const auto ownerIndex = ReadVarUInt(serializer);
            std::string owner;
            if (ownerIndex > 0 && ownerIndex - 1 < owners.size()) {
                owner = owners[ownerIndex - 1];
            }
            const auto key = ReadCompactString(serializer);

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
