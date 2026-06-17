#include "persistence.h"
#include "model.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <vector>

namespace {
    constexpr uint32_t kRegistrySchemaVersion = 2;
    std::mutex registryMutex;

    std::string ToHex(const uint32_t value, const int width = 0) {
        std::ostringstream stream;
        stream << "0x" << std::uppercase << std::hex << std::setfill('0');
        if (width > 0) {
            stream << std::setw(width);
        }
        stream << value;
        return stream.str();
    }

    uint32_t ParseUInt(const rapidjson::Value& value, const uint32_t fallback = 0) {
        if (value.IsUint()) {
            return value.GetUint();
        }
        if (!value.IsString()) {
            return fallback;
        }

        try {
            return static_cast<uint32_t>(std::stoul(value.GetString(), nullptr, 0));
        } catch (...) {
            return fallback;
        }
    }

    template <class Allocator>
    rapidjson::Value StringValue(Allocator& allocator, const std::string& text) {
        rapidjson::Value value;
        value.SetString(text.c_str(), static_cast<rapidjson::SizeType>(text.size()), allocator);
        return value;
    }
}

std::string GetGlobalRegistryPath() {
    return "Data/SKSE/Plugins/DPF_Cache.json";
}

std::string BuildRegistryJson() {
    rapidjson::Document document(rapidjson::kObjectType);
    auto& allocator = document.GetAllocator();

    document.AddMember("schemaVersion", kRegistrySchemaVersion, allocator);
    document.AddMember("dynamicPluginFile", StringValue(allocator, dynamicPluginName), allocator);
    document.AddMember("nextLocalId", StringValue(allocator, ToHex(nextDynamicLocalId, 6)), allocator);

    rapidjson::Value slots(rapidjson::kArrayType);
    std::vector<DynamicSlot> sorted;
    sorted.reserve(dynamicSlots.size());
    for (const auto& [localId, slot] : dynamicSlots) {
        sorted.push_back(slot);
    }
    std::ranges::sort(sorted, [](const auto& lhs, const auto& rhs) {
        return lhs.localId < rhs.localId;
    });

    for (const auto& slotData : sorted) {
        rapidjson::Value slot(rapidjson::kObjectType);
        slot.AddMember("localId", StringValue(allocator, ToHex(slotData.localId, 6)), allocator);
        slot.AddMember("formType", static_cast<uint32_t>(slotData.formType), allocator);
        slot.AddMember("state", StringValue(allocator, "used"), allocator);
        slot.AddMember("owner", StringValue(allocator, slotData.owner), allocator);
        slot.AddMember("key", StringValue(allocator, slotData.key), allocator);
        slots.PushBack(slot, allocator);
    }
    document.AddMember("slots", slots, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
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

    if (!document.HasMember("schemaVersion") || !document["schemaVersion"].IsUint() ||
        document["schemaVersion"].GetUint() != kRegistrySchemaVersion) {
        logger::warn("DPF global registry schema is missing or unsupported; starting with empty schema {} registry", kRegistrySchemaVersion);
        ResetDynamicState();
        return SaveGlobalRegistry();
    }

    ResetDynamicState();

    if (document.HasMember("nextLocalId")) {
        nextDynamicLocalId = std::max(ParseUInt(document["nextLocalId"], firstDynamicLocalId), firstDynamicLocalId);
    }

    if (document.HasMember("slots") && document["slots"].IsArray()) {
        for (const auto& slot : document["slots"].GetArray()) {
            if (!slot.IsObject() || !slot.HasMember("localId") || !slot.HasMember("formType")) {
                continue;
            }

            const auto localId = ParseUInt(slot["localId"]);
            const auto formType = static_cast<RE::FormType>(ParseUInt(slot["formType"]));
            const std::string owner = slot.HasMember("owner") && slot["owner"].IsString() ? slot["owner"].GetString() : "";
            const std::string key = slot.HasMember("key") && slot["key"].IsString() ? slot["key"].GetString() : "";
            if (!RegisterDynamicSlot(localId, formType, owner, key)) {
                logger::warn("Ignoring invalid DPF registry slot {:06X}", localId);
            }
        }
    }

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
        logger::info("Saved DPF global registry to {}", path);
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
