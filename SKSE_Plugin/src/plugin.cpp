#include "log.h"
#include "form_record.h"
#include "model.h"
#include "persistence.h"
#include "papyrus.h"

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);

    //EnableLog("DynamicPersistentFormsLog.txt", "DPF 2");

    SKSE::GetPapyrusInterface()->Register(PapyrusFunctions);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            ReadFirstFormIdFromESP();
            LoadCache();
            print("loaded");
        } else if (message->type == SKSE::MessagingInterface::kNewGame) {
            std::filesystem::remove("DynamicPersistentFormsCache.bin");
            while (formRef.size() > 0) {
                delete formRef.back();
                formRef.pop_back();
            }
            while (formData.size() > 0) {
                if (formData.back()) {
                    if (formData.back()->actualForm) {
                        formData.back()->actualForm->SetDelete(true);
                    }
                }
                delete formData.back();
                formData.pop_back();
            }
            ResetId();
            print("new game");
        }
    });

    auto serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID('DPF1');
    serialization->SetSaveCallback(SaveCallback);
    serialization->SetLoadCallback(LoadCallback);

    return true;
}

