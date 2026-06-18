#include "persistence.h"
#include "model.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <algorithm>
#include <fstream>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {
    constexpr uint32_t kRegistrySchemaVersion = 1;
    std::mutex registryMutex;

    uint32_t ParseUInt(const rapidjson::Value& value, const uint32_t fallback = 0) {
        return value.IsUint() ? value.GetUint() : fallback;
    }

    template <class Allocator>
    rapidjson::Value StringValue(Allocator& allocator, const std::string& text) {
        rapidjson::Value value;
        value.SetString(text.c_str(), static_cast<rapidjson::SizeType>(text.size()), allocator);
        return value;
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
}

std::string GetGlobalRegistryPath() {
    return "Data/SKSE/Plugins/DPF_Cache.json";
}

std::string BuildRegistryJson() {
    rapidjson::Document document(rapidjson::kObjectType);
    auto& allocator = document.GetAllocator();

    std::vector<DynamicSlot> sorted;
    sorted.reserve(dynamicSlots.size());
    for (const auto& [localId, slot] : dynamicSlots) {
        sorted.push_back(slot);
    }
    std::ranges::sort(sorted, [](const auto& lhs, const auto& rhs) {
        return lhs.localId < rhs.localId;
    });

    std::unordered_map<std::string, uint32_t> ownerIndexes;
    rapidjson::Value owners(rapidjson::kArrayType);
    for (const auto& slotData : sorted) {
        if (slotData.owner.empty() || ownerIndexes.contains(slotData.owner)) {
            continue;
        }

        const auto index = static_cast<uint32_t>(ownerIndexes.size());
        ownerIndexes[slotData.owner] = index;
        auto ownerValue = StringValue(allocator, slotData.owner);
        owners.PushBack(ownerValue, allocator);
    }

    rapidjson::Value slots(rapidjson::kArrayType);
    for (const auto& slotData : sorted) {
        rapidjson::Value slot(rapidjson::kArrayType);
        slot.PushBack(slotData.localId, allocator);
        slot.PushBack(static_cast<uint32_t>(slotData.formType), allocator);
        if (slotData.owner.empty()) {
            rapidjson::Value ownerIndex;
            ownerIndex.SetNull();
            slot.PushBack(ownerIndex, allocator);
        } else {
            slot.PushBack(ownerIndexes[slotData.owner], allocator);
        }
        auto keyValue = StringValue(allocator, slotData.key);
        slot.PushBack(keyValue, allocator);
        slots.PushBack(slot, allocator);
    }

    document.AddMember("v", kRegistrySchemaVersion, allocator);
    document.AddMember("p", StringValue(allocator, dynamicPluginName), allocator);
    document.AddMember("o", owners, allocator);
    document.AddMember("s", slots, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return { buffer.GetString(), buffer.GetSize() };
}

bool RestoreRegistryJson(const std::string& json) {
    if (json.empty()) {
        return false;
    }

    rapidjson::Document document;
    document.Parse(json.c_str(), json.size());
    if (document.HasParseError() || !document.IsObject()) {
        logger::error("DPF global registry is not valid JSON");
        return false;
    }

    if (!document.HasMember("v") || !document["v"].IsUint() || document["v"].GetUint() != kRegistrySchemaVersion) {
        logger::warn("DPF global registry schema is missing or unsupported; starting with empty schema {} registry", kRegistrySchemaVersion);
        ResetDynamicState();
        return SaveGlobalRegistry();
    }

    ResetDynamicState();

    if (document.HasMember("p") && document["p"].IsString()) {
        dynamicPluginName = document["p"].GetString();
    }

    std::vector<std::string> owners;
    if (document.HasMember("o") && document["o"].IsArray()) {
        for (const auto& owner : document["o"].GetArray()) {
            owners.emplace_back(owner.IsString() ? owner.GetString() : "");
        }
    }

    if (document.HasMember("s") && document["s"].IsArray()) {
        for (const auto& slot : document["s"].GetArray()) {
            if (!slot.IsArray() || slot.Size() < 4) {
                continue;
            }

            const auto localId = ParseUInt(slot[0]);
            const auto formType = static_cast<RE::FormType>(ParseUInt(slot[1]));
            std::string owner;
            if (slot[2].IsUint() && slot[2].GetUint() < owners.size()) {
                owner = owners[slot[2].GetUint()];
            }
            const std::string key = slot[3].IsString() ? slot[3].GetString() : "";
            if (!RegisterDynamicSlot(localId, formType, owner, key)) {
                logger::warn("Ignoring invalid DPF registry slot {:06X}", localId);
            }
        }
    }

    RecalculateNextDynamicLocalId();

    logger::info("Loaded DPF global registry with {} slots", dynamicSlots.size());
    return true;
}

bool LoadGlobalRegistry() {
    std::lock_guard lock(registryMutex);
    const auto path = GetGlobalRegistryPath();
    if (!fs::exists(path)) {
        logger::info("DPF global registry not found at {}; starting empty", path);
        ResetDynamicState();
        return SaveGlobalRegistry();
    }

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        logger::error("Could not open DPF global registry at {}", path);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return RestoreRegistryJson(buffer.str());
}

bool SaveGlobalRegistry() {
    const auto path = GetGlobalRegistryPath();
    try {
        const fs::path registryPath(path);
        if (registryPath.has_parent_path()) {
            fs::create_directories(registryPath.parent_path());
        }

        std::ofstream file(registryPath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            logger::error("Could not write DPF global registry at {}", path);
            return false;
        }

        file << BuildRegistryJson();
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
