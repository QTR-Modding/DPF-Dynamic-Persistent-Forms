#include "Services.h"
#include "logger.h"
#include "model.h"
#include "papyrus.h"
#include "persistence.h"

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

    RE::TESForm* CreateByType(const uint32_t formType) override {
        return Services::CreateByType(formType);
    }

    RE::TESForm* GetOrCreateByLocalId(const uint32_t localId, const uint32_t formType) override {
        return Services::GetOrCreateByLocalId(localId, formType);
    }

    RE::TESForm* GetOrCreateByFormId(const RE::FormID formId, const uint32_t formType) override {
        return Services::GetOrCreateByFormId(formId, formType);
    }

    RE::TESForm* GetOrCreateByOwnerKey(const char* owner, const char* key, const uint32_t formType,
        uint32_t* localId, bool* existed) override {
        return Services::GetOrCreateByOwnerKey(owner, key, formType, localId, existed);
    }

    bool ReleaseByOwnerKey(const char* owner, const char* key) override {
        return Services::ReleaseByOwnerKey(owner, key);
    }

    bool ReleaseByLocalId(const uint32_t localId, const char* owner) override {
        return Services::ReleaseByLocalId(localId, owner);
    }

    uint32_t ReleaseOwner(const char* owner) override {
        return Services::ReleaseOwner(owner);
    }
};

extern "C" __declspec(dllexport) void* GetDPFAPI() {
    return DPFInterfaceImpl::GetSingleton();
}

namespace {
    void OnMessage(SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            ReadFirstFormIdFromESP();
            LoadGlobalRegistry();
            logger::info("DynamicPersistentForms data loaded");
        } else if (message->type == SKSE::MessagingInterface::kNewGame) {
            ClearRecords(true);
            logger::info("DPF new game state cleared");
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SetupLog();
    logger::info("Plugin loaded");

    SKSE::Init(skse);
    SKSE::GetPapyrusInterface()->Register(PapyrusFunctions);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    const auto serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID('DPF1');
    serialization->SetSaveCallback(SaveCallback);
    serialization->SetLoadCallback(LoadCallback);

    return true;
}


