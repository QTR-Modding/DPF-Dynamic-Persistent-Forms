#include "persistence.h"
#include "form.h"
#include "form_record_serializer.h"
#include "model.h"
#include "serializer.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace {
    constexpr uint32_t kJsonRecord = 'JSN1';
    constexpr uint32_t kJsonVersion = 1;

    std::mutex callbackMutex;

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

        const std::string text = value.GetString();
        try {
            return static_cast<uint32_t>(std::stoul(text, nullptr, 0));
        } catch (...) {
            return fallback;
        }
    }

    std::string BytesToHex(const std::vector<uint8_t>& bytes) {
        static constexpr char digits[] = "0123456789ABCDEF";
        std::string output;
        output.reserve(bytes.size() * 2);
        for (const auto byte : bytes) {
            output.push_back(digits[(byte >> 4) & 0x0f]);
            output.push_back(digits[byte & 0x0f]);
        }
        return output;
    }

    std::vector<uint8_t> HexToBytes(const std::string& text) {
        std::vector<uint8_t> bytes;
        if (text.size() % 2 != 0) {
            return bytes;
        }

        bytes.reserve(text.size() / 2);
        for (size_t i = 0; i < text.size(); i += 2) {
            uint32_t value = 0;
            const auto result = std::from_chars(text.data() + i, text.data() + i + 2, value, 16);
            if (result.ec != std::errc()) {
                bytes.clear();
                return bytes;
            }
            bytes.push_back(static_cast<uint8_t>(value));
        }
        return bytes;
    }

    template <class Allocator>
    rapidjson::Value StringValue(Allocator& allocator, const std::string& text) {
        rapidjson::Value value;
        value.SetString(text.c_str(), static_cast<rapidjson::SizeType>(text.size()), allocator);
        return value;
    }

    template <class Allocator>
    rapidjson::Value FormRefToJson(RE::TESForm* form, Allocator& allocator) {
        if (!form) {
            return rapidjson::Value(rapidjson::kNullType);
        }

        rapidjson::Value result(rapidjson::kObjectType);
        if (IsDynamicFormID(form->GetFormID())) {
            result.AddMember("dynamic", true, allocator);
            result.AddMember("localId", StringValue(allocator, ToHex(ToDynamicLocalID(form->GetFormID()), 6)), allocator);
            return result;
        }

        if (const auto file = form->GetFile(0)) {
            result.AddMember("file", StringValue(allocator, file->fileName), allocator);
            result.AddMember("localId", StringValue(allocator, ToHex(form->GetLocalFormID(), 6)), allocator);
            return result;
        }

        result.AddMember("formId", StringValue(allocator, ToHex(form->GetFormID(), 8)), allocator);
        return result;
    }

    RE::TESForm* JsonToFormRef(const rapidjson::Value& value) {
        if (!value.IsObject()) {
            return nullptr;
        }

        if (value.HasMember("dynamic") && value["dynamic"].IsBool() && value["dynamic"].GetBool()) {
            const auto localId = value.HasMember("localId") ? ParseUInt(value["localId"]) : 0;
            return localId ? RE::TESForm::LookupByID(MakeDynamicFormID(localId)) : nullptr;
        }

        if (value.HasMember("file") && value["file"].IsString() && value.HasMember("localId")) {
            const auto localId = ParseUInt(value["localId"]);
            const auto fullId = RE::TESDataHandler::GetSingleton()->LookupFormID(localId, value["file"].GetString());
            return fullId ? RE::TESForm::LookupByID(fullId) : nullptr;
        }

        if (value.HasMember("formId")) {
            const auto formId = ParseUInt(value["formId"]);
            return formId ? RE::TESForm::LookupByID(formId) : nullptr;
        }

        return nullptr;
    }

    std::string SerializeRecordData(FormRecord* record) {
        if (!record) {
            return {};
        }

        MemoryWriter writer;
        StoreFormRecordData(&writer, record);
        return BytesToHex(writer.Data());
    }

    bool DeserializeRecordData(FormRecord* record, const std::string& hex) {
        if (!record || hex.empty()) {
            return false;
        }

        auto bytes = HexToBytes(hex);
        if (bytes.empty()) {
            return false;
        }

        MemoryReader reader(std::move(bytes));
        RestoreFormRecordData(&reader, record);
        return true;
    }

    std::vector<FormRecord*> SortedRecords(const std::vector<FormRecord*>& records) {
        auto sorted = records;
        std::ranges::sort(sorted, [](const FormRecord* lhs, const FormRecord* rhs) {
            return lhs && rhs ? lhs->order < rhs->order : lhs != nullptr;
        });
        return sorted;
    }

    template <class Allocator>
    rapidjson::Value CreatedRecordToJson(FormRecord* record, Allocator& allocator) {
        rapidjson::Value item(rapidjson::kObjectType);
        const auto localId = ToDynamicLocalID(record->formId);

        item.AddMember("order", record->order, allocator);
        item.AddMember("localId", StringValue(allocator, ToHex(localId, 6)), allocator);
        item.AddMember("formId", StringValue(allocator, ToHex(record->formId, 8)), allocator);
        item.AddMember("formType", static_cast<uint32_t>(record->formType), allocator);
        item.AddMember("deleted", record->deleted, allocator);
        item.AddMember("baseForm", FormRefToJson(record->baseForm, allocator), allocator);
        item.AddMember("modelForm", FormRefToJson(record->modelForm, allocator), allocator);
        item.AddMember("data", StringValue(allocator, SerializeRecordData(record)), allocator);
        return item;
    }

    template <class Allocator>
    rapidjson::Value TrackedRecordToJson(FormRecord* record, Allocator& allocator) {
        rapidjson::Value item(rapidjson::kObjectType);
        item.AddMember("order", record->order, allocator);
        item.AddMember("form", FormRefToJson(record->actualForm, allocator), allocator);
        item.AddMember("formId", StringValue(allocator, ToHex(record->formId, 8)), allocator);
        item.AddMember("deleted", record->deleted, allocator);
        item.AddMember("modelForm", FormRefToJson(record->modelForm, allocator), allocator);
        item.AddMember("data", StringValue(allocator, SerializeRecordData(record)), allocator);
        return item;
    }

    FormRecord* CreateRecordFromJson(const rapidjson::Value& item) {
        const auto order = item.HasMember("order") ? ParseUInt(item["order"]) : 0;
        const auto localId = item.HasMember("localId") ? ParseUInt(item["localId"]) : 0;
        const auto formType = static_cast<RE::FormType>(item.HasMember("formType") ? ParseUInt(item["formType"]) : 0);
        const bool deleted = item.HasMember("deleted") && item["deleted"].IsBool() && item["deleted"].GetBool();
        const auto formId = MakeDynamicFormID(localId);

        FormRecord* record = nullptr;
        if (deleted) {
            record = FormRecord::CreateDeleted(formId);
            record->formType = formType;
        } else {
            const auto factory = RE::IFormFactory::GetFormFactoryByType(formType);
            if (!factory) {
                logger::error("No factory for saved form type {}", static_cast<uint32_t>(formType));
                return nullptr;
            }

            auto* form = factory->Create();
            if (!form) {
                logger::error("Factory returned null for saved form type {}", static_cast<uint32_t>(formType));
                return nullptr;
            }

            form->SetFormID(formId, false);
            record = FormRecord::CreateNew(form, formType, formId);
            record->baseForm = item.HasMember("baseForm") ? JsonToFormRef(item["baseForm"]) : nullptr;
            record->modelForm = item.HasMember("modelForm") ? JsonToFormRef(item["modelForm"]) : nullptr;
            if (record->baseForm || record->modelForm) {
                applyPattern(record);
            }
        }

        record->order = order;
        AddFormData(record);
        return record;
    }

    FormRecord* CreateTrackedRecordFromJson(const rapidjson::Value& item) {
        const auto order = item.HasMember("order") ? ParseUInt(item["order"]) : 0;
        const bool deleted = item.HasMember("deleted") && item["deleted"].IsBool() && item["deleted"].GetBool();
        auto* actualForm = item.HasMember("form") ? JsonToFormRef(item["form"]) : nullptr;

        FormRecord* record = nullptr;
        if (!actualForm || deleted) {
            const auto formId = item.HasMember("formId") ? ParseUInt(item["formId"]) : 0;
            record = FormRecord::CreateDeleted(formId);
        } else {
            record = FormRecord::CreateReference(actualForm);
            record->modelForm = item.HasMember("modelForm") ? JsonToFormRef(item["modelForm"]) : nullptr;
            if (record->modelForm) {
                applyPattern(record);
            }
        }

        record->order = order;
        AddFormRef(record);
        return record;
    }

}

std::string BuildStateJson() {
    rapidjson::Document document(rapidjson::kObjectType);
    auto& allocator = document.GetAllocator();

    document.AddMember("schemaVersion", 1, allocator);
    document.AddMember("dynamicPluginFile", StringValue(allocator, dynamicPluginName), allocator);
    document.AddMember("firstDynamicLocalId", StringValue(allocator, ToHex(firstDynamicLocalId, 6)), allocator);

    rapidjson::Value createdForms(rapidjson::kArrayType);
    for (auto* record : SortedRecords(formData)) {
        if (record && IsDynamicFormID(record->formId)) {
            createdForms.PushBack(CreatedRecordToJson(record, allocator), allocator);
        }
    }
    document.AddMember("createdForms", createdForms, allocator);

    rapidjson::Value trackedForms(rapidjson::kArrayType);
    for (auto* record : SortedRecords(formRef)) {
        if (record) {
            trackedForms.PushBack(TrackedRecordToJson(record, allocator), allocator);
        }
    }
    document.AddMember("trackedForms", trackedForms, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return { buffer.GetString(), buffer.GetSize() };
}

bool RestoreStateJson(const std::string& json) {
    if (json.empty()) {
        return false;
    }

    rapidjson::Document document;
    document.Parse(json.c_str(), json.size());
    if (document.HasParseError() || !document.IsObject()) {
        logger::error("DynamicPersistentForms save record is not valid JSON");
        return false;
    }

    if (!espFound) {
        ReadFirstFormIdFromESP();
    }
    if (!espFound) {
        return false;
    }

    ClearRecords(true);

    if (document.HasMember("createdForms") && document["createdForms"].IsArray()) {
        for (const auto& item : document["createdForms"].GetArray()) {
            if (item.IsObject() && item.HasMember("localId")) {
                ReserveDynamicLocalID(ParseUInt(item["localId"]));
            }
        }

        std::vector<const rapidjson::Value*> items;
        for (const auto& item : document["createdForms"].GetArray()) {
            if (item.IsObject()) {
                items.push_back(&item);
            }
        }
        std::ranges::sort(items, [](const rapidjson::Value* lhs, const rapidjson::Value* rhs) {
            return ParseUInt((*lhs)["order"]) < ParseUInt((*rhs)["order"]);
        });

        std::vector<std::pair<FormRecord*, const rapidjson::Value*>> restored;
        for (const auto* item : items) {
            auto* record = CreateRecordFromJson(*item);
            if (record) {
                restored.emplace_back(record, item);
            }
        }

        for (const auto& [record, item] : restored) {
            if (item->HasMember("data") && (*item)["data"].IsString()) {
                DeserializeRecordData(record, (*item)["data"].GetString());
            }
        }
    }

    if (document.HasMember("trackedForms") && document["trackedForms"].IsArray()) {
        std::vector<const rapidjson::Value*> items;
        for (const auto& item : document["trackedForms"].GetArray()) {
            if (item.IsObject()) {
                items.push_back(&item);
            }
        }
        std::ranges::sort(items, [](const rapidjson::Value* lhs, const rapidjson::Value* rhs) {
            return ParseUInt((*lhs)["order"]) < ParseUInt((*rhs)["order"]);
        });

        std::vector<std::pair<FormRecord*, const rapidjson::Value*>> restored;
        for (const auto* item : items) {
            auto* record = CreateTrackedRecordFromJson(*item);
            if (record) {
                restored.emplace_back(record, item);
            }
        }

        for (const auto& [record, item] : restored) {
            if (item->HasMember("data") && (*item)["data"].IsString()) {
                DeserializeRecordData(record, (*item)["data"].GetString());
            }
        }
    }

    logger::info("DynamicPersistentForms restored {} created forms and {} tracked forms", formData.size(), formRef.size());
    return true;
}

void SaveCallback(SKSE::SerializationInterface* a_intfc) {
    std::lock_guard lock(callbackMutex);
    try {
        const auto json = BuildStateJson();
        if (!a_intfc->OpenRecord(kJsonRecord, kJsonVersion)) {
            logger::error("Failed to open DynamicPersistentForms JSON save record");
            return;
        }
        if (!json.empty() && !a_intfc->WriteRecordData(json.data(), static_cast<uint32_t>(json.size()))) {
            logger::error("Failed to write DynamicPersistentForms JSON save record");
            return;
        }
    } catch (const std::exception& e) {
        logger::error("Error saving DynamicPersistentForms JSON state: {}", e.what());
    }
}

void LoadCallback(SKSE::SerializationInterface* a_intfc) {
    std::lock_guard lock(callbackMutex);
    try {
        uint32_t type = 0;
        uint32_t version = 0;
        uint32_t length = 0;

        while (a_intfc->GetNextRecordInfo(type, version, length)) {
            if (type != kJsonRecord) {
                logger::warn("Ignoring unrecognized DPF record {:08X}", type);
                continue;
            }
            if (version != kJsonVersion) {
                logger::warn("Ignoring unsupported DynamicPersistentForms JSON version {}", version);
                continue;
            }

            std::string json(length, '\0');
            const auto read = a_intfc->ReadRecordData(json.data(), length);
            if (read != length) {
                logger::error("Could not read full DynamicPersistentForms JSON record");
                return;
            }

            RestoreStateJson(json);
        }
    } catch (const std::exception& e) {
        logger::error("Error loading DynamicPersistentForms JSON state: {}", e.what());
    }
}

