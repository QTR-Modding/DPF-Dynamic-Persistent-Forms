#include "persistence.h"
#include "form_record_serializer.h"
#include "serializer.h"

std::mutex callbackMutext;

void SaveCallback(SKSE::SerializationInterface* a_intfc) {
    std::lock_guard lock(callbackMutext);
    try {
        logger::info("SAVE CAllBACK");
        if (!a_intfc->OpenRecord('ARR_', 1)) {
            logger::error("Failed to open record for arr!");
        }
        else {
            const auto serializer = new SaveDataSerializer(a_intfc);
            StoreAllFormRecords(serializer);
        }
        SaveCache();
    }
    catch (const std::exception&) {
        logger::error("error saving");
    }
}

void LoadCallback(SKSE::SerializationInterface* a_intfc) {
    std::lock_guard lock(callbackMutext);
    try {
        logger::info("LOAD CAllBACK");

        uint32_t type;
        uint32_t version;
        uint32_t length;
        bool refreshGame = false;

        while (a_intfc->GetNextRecordInfo(type, version, length)) {
            switch (type) {
            case 'ARR_': {
                const auto serializer = new SaveDataSerializer(a_intfc);
                refreshGame = RestoreAllFormRecords(serializer);
                delete serializer;
            }
                       break;
            default:
                logger::error("Unrecognized signature type!");
                break;
            }
        }
        if (refreshGame) {
            SaveCache();
            UpdateId();
            RE::PlayerCharacter::GetSingleton()->KillImmediate();
        }

        logger::info("CAllBACK LOADED");
    }
    catch (const std::exception&) {
        logger::error("error loading");
    }
}

std::string GetCacheFilePath()
{
    const std::string settingsFile = "Data/SKSE/Plugins/DPF_Settings.json";
    const std::string defaultPath = "Data/SKSE/Plugins/[NoDelete] DPF/DynamicPersistentFormsCache.bin";

    if (!fs::exists(settingsFile)) {
        return defaultPath;
    }

    FILE* fp = fopen(settingsFile.c_str(), "rb");
    if (!fp) return defaultPath;

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    rapidjson::Document d;
    d.ParseStream(is);
    fclose(fp);

    if (!d.HasParseError() && d.HasMember("SavePath") && d["SavePath"].IsString()) {
        std::string customDir = d["SavePath"].GetString();
        return customDir + "/DynamicPersistentFormsCache.bin";
    }

    return defaultPath;
}

void LoadCache() {
    logger::info("LOAD CACHE");

    std::string path = GetCacheFilePath();

    if (!fs::exists(path)) {
        logger::info("Cache file not found. Creating initial file at: {}", path);
        SaveCache(); 
        return;
    }

    const auto fileReader = new FileReader(path, std::ios::in | std::ios::binary);
    if (!fileReader->IsOpen()) {
        logger::error("File not found");
        return;
    }
    RestoreAllFormRecords(fileReader);

    UpdateId();

    delete fileReader;

    logger::info("Property data has been loaded from file successfully.");
}

void SaveCache() {
    logger::info("save cache");

    std::string fullPath = GetCacheFilePath();
    fs::path p(fullPath);

    try {
        if (p.has_parent_path() && !fs::exists(p.parent_path())) {
            fs::create_directories(p.parent_path());
        }

        const auto fileWriter = new FileWriter(fullPath,
            std::ios::out | std::ios::binary | std::ios::trunc);

        if (!fileWriter->IsOpen()) {
            logger::error("Failed to open file for writing: {}", fullPath);
            delete fileWriter;
            return;
        }

        StoreAllFormRecords(fileWriter);
        delete fileWriter;

        logger::info("Property data has been written successfully to: {}", fullPath);
    }
    catch (const std::exception& e) {
        logger::error("Error during save: {}", e.what());
    }
}