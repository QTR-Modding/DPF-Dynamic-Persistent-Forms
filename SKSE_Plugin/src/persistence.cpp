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
        } else {
            const auto serializer = new SaveDataSerializer(a_intfc);
            StoreAllFormRecords(serializer);
        }
        SaveCache();
    } catch (const std::exception&) {
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
    } catch (const std::exception&) {
        logger::error("error loading");
    }
}

void LoadCache() {
    logger::info("LOAD CACHE");
    const auto fileReader = new FileReader("DynamicPersistentFormsCache.bin", std::ios::in | std::ios::binary);

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

    const auto fileWriter = new FileWriter("DynamicPersistentFormsCache.bin",
                                           std::ios::out | std::ios::binary | std::ios::trunc);

    if (!fileWriter->IsOpen()) {
        logger::error("File not found");
        return;
    }

    StoreAllFormRecords(fileWriter);

    delete fileWriter;

    logger::info("Property data has been written to file successfully.");
}