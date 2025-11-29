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
            UpdateId();
            RE::PlayerCharacter::GetSingleton()->KillImmediate();
        }

        logger::info("CAllBACK LOADED");
    } catch (const std::exception&) {
        logger::error("error loading");
    }
}


std::string removeEssSuffix(const std::string& input) {
    if (input.size() >= 4 && input.compare(input.size() - 4, 4, ".ess") == 0) {
        return input.substr(0, input.size() - 4);
    }
    return input;
}

void Persistence::Load(std::string fileName) {
    fileName = removeEssSuffix(fileName) + "_DPF.bin";
    logger::trace("loading: {}", fileName);

    auto fileReader = new FileReader(fileName);

    if (!fileReader->IsOpen()) {
        logger::error("File not found");
        return;
    }

    RestoreAllFormRecords(fileReader);

    UpdateId();

    delete fileReader;
}
void Persistence::Save(std::string fileName) {
    fileName = removeEssSuffix(fileName) + "_DPF.bin";
    logger::trace("saving: {}", fileName);

    auto fileWriter = new FileWriter(fileName);

    if (!fileWriter->IsOpen()) {
        logger::error("File not found");
        return;
    }

    StoreAllFormRecords(fileWriter);

    delete fileWriter;
}