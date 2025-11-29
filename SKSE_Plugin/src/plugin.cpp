#include "form_record.h"
#include "model.h"
#include "persistence.h"
#include "papyrus.h"
#include "Services.h"
#include "hooks.h"


class DPFInterfaceImpl : public DPF::IDynamicPersistentForms {
public:
    static DPFInterfaceImpl* GetSingleton() {
        static DPFInterfaceImpl instance;
        return &instance;
    }

    uint32_t GetVersion() const override {
        return DPF::InterfaceVersion;
    }

    RE::TESForm* Create(RE::TESForm* baseItem) override {
        return Services::Create(baseItem);
    }

    void Dispose(RE::TESForm* form) override {
        Services::Dispose(form);
    }

    void Track(RE::TESForm* item) override {
        Services::Track(item);
    }

    void UnTrack(RE::TESForm* item) override {
        Services::UnTrack(item);
    }
};

extern "C" __declspec(dllexport) void* GetDPFAPI() {
    return DPFInterfaceImpl::GetSingleton();
}

namespace {
    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void OnMessage(SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            ReadFirstFormIdFromESP();
            logger::info("loaded");
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
            logger::info("new game");
        }
    }
}


SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);

    //EnableLog("DynamicPersistentFormsLog.txt", "DPF 2");

    SKSE::GetPapyrusInterface()->Register(PapyrusFunctions);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    auto serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID('DPF1');
    serialization->SetSaveCallback(SaveCallback);
    serialization->SetLoadCallback(LoadCallback);
    Hooks::Install();

    return true;
}