#pragma once
#include "form.h"
#include "form_record.h"
#include "form_serializer.h"
#include "serializer.h"
#include "model.h"

template <typename T>
void StoreAllFormRecords(Serializer<T>* serializer) {

    const size_t sizeData = formData.size();
    const size_t sizeRef = formRef.size();

    serializer->Write<uint32_t>(static_cast<uint32_t>(sizeData));
    serializer->Write<uint32_t>(static_cast<uint32_t>(sizeRef));
    logger::info("write forms");

    EachFormData([&](FormRecord* instance) {
        StoreFormRecord(serializer, instance, false);
        return true;
    });

    logger::info("write data");
    EachFormData([&](FormRecord* elem) {
        StoreFormRecordData(serializer, elem);
        return true;
    });
    logger::info("write ref");
    EachFormRef([&](FormRecord* instance) {
        StoreFormRecord(serializer, instance, true);
        return true;
    });
    logger::info("write ref data");

    EachFormRef([&](FormRecord* elem) {
        StoreFormRecordData(serializer, elem);
        return true;
    });
}

template <typename T>
bool RestoreAllFormRecords(Serializer<T>* serializer) {

    bool formRecordCreated = false;

    const uint32_t sizeData = serializer->Read<uint32_t>();
    const uint32_t sizeRef = serializer->Read<uint32_t>();
    logger::info("number of items {}", sizeData);

    for (uint32_t i = 0; i < sizeData; i++) {
        logger::trace("form data {}", i);
        if (RestoreFormRecord(serializer, i, false)) {
            formRecordCreated = true;
        }
    }
    logger::info("reading form data");

    for (uint32_t i = 0; i < sizeData; ++i) {
        logger::trace("form data {}", i);
        auto instance = formData[i];
        RestoreFormRecordData(serializer, instance);
    }

    logger::info("number of items {}", sizeRef);
    for (uint32_t i = 0; i < sizeRef; i++) {
        logger::trace("form data {}", i);
        if (RestoreFormRecord(serializer, i, true)) {
            formRecordCreated = true;
        }
    }
    logger::info("reading form data");

    for (uint32_t i = 0; i < sizeRef; ++i) {
        logger::trace("form data {}", i);
        auto instance = formRef[i];
        RestoreFormRecordData(serializer, instance);
    }



    return formRecordCreated;
}



template <typename T>
static void StoreFormRecord(Serializer<T>* serializer, FormRecord* instance, const bool reference) {

    serializer->StartWritingSection();

    serializer->Write<char>(instance->deleted ? 1 : 0);

    if (!instance->deleted) {
        if (reference) {
            logger::info("changed");
            serializer->WriteFormRef(instance->actualForm);
            serializer->WriteFormRef(instance->modelForm);
        } else {
            logger::info("created");
            serializer->WriteFormRef(instance->baseForm);
            serializer->WriteFormRef(instance->modelForm);
            serializer->WriteFormId(instance->formId);
        }
    } else {
        logger::info("deleted");
        serializer->WriteFormId(instance->formId);
    }

    serializer->finishWritingSection();
}

template <typename T>
static void StoreFormRecordData(Serializer<T>* serializer, FormRecord* instance) {
    serializer->StartWritingSection();
    serializer->Write<char>(instance->deleted ? 1 : 0);
    if (!instance->deleted) {
        StoreEachFormData(serializer, instance);
    }
    serializer->finishWritingSection();
}



template <typename T>
static bool RestoreFormRecord(Serializer<T>* serializer, const uint32_t i, const bool reference) {
    FormRecord* instance = nullptr;
    bool createdRecord = false;
    serializer->startReadingSection();
    auto deleted = serializer->Read<char>();

    if (deleted == 1) {
        auto formId = serializer->ReadFormId();
        serializer->finishReadingSection();
        if (reference) {
            if (i < formRef.size()) {
                instance = formRef[i];
                instance->deleted = true;
            } else {
                auto deletedInstance = FormRecord::CreateDeleted(formId);
                AddFormRef(deletedInstance);
            }
        } 
        else
        {
            if (i < formData.size()) {
                instance = formData[i];
                instance->deleted = true;
            } else {
                auto deletedInstance = FormRecord::CreateDeleted(formId);
                AddFormData(deletedInstance);
            }
        }
        return false;
    }

    if (reference) {
        if (i < formRef.size()) {
            instance = formRef[i];
        }
        createdRecord = RestoreModifiedItem(serializer, instance);
    } else {
        if (i < formData.size()) {
            instance = formData[i];
        }
        createdRecord = RestoreCreatedItem(serializer, instance);
    }

    return createdRecord;

}
template <typename T>
static bool RestoreModifiedItem(Serializer<T>* serializer, FormRecord* instance) {

    bool createdRecord = false;
    logger::info("changed form");
    auto actualForm = serializer->ReadFormRef();
    auto modelForm = serializer->ReadFormRef();
    serializer->finishReadingSection();

    if (!actualForm) {
        if (!instance) {
            instance = FormRecord::CreateDeleted(0);
            AddFormRef(instance);
        } else {
            instance->deleted = true;
        }
        logger::info("missing actual form");
        return false;
    }


    if (!instance) {
        logger::info("ref instance not found creating it");
        instance = FormRecord::CreateReference(actualForm);
        instance->modelForm = modelForm;
        AddFormRef(instance);
        createdRecord = true;
    }


    instance->modelForm = modelForm;
    instance->deleted = false;
    instance->actualForm = actualForm;

    applyPattern(instance);


    return createdRecord;
}

template <typename T>
static bool RestoreCreatedItem(Serializer<T>* serializer, FormRecord* instance) {
    logger::info("new form");
    bool createdRecord = false;
    auto baseForm = serializer->ReadFormRef();
    auto modelForm = serializer->ReadFormRef();
    auto id = serializer->ReadFormId();
    serializer->finishReadingSection();

    if (!baseForm) {
        logger::info("Missing base form");
        if (!instance) {
            instance = FormRecord::CreateDeleted(id);
            AddFormData(instance);
        } else {
            instance->deleted = true;
        }
        return false;
    }

    if (instance && (instance->formType != baseForm->GetFormType() || instance->deleted)) {
        if (instance->actualForm) {
            instance->actualForm->SetDelete(true);
        }
        logger::info("instance is of incompatible type");
        auto factory = RE::IFormFactory::GetFormFactoryByType(baseForm->GetFormType());
        RE::TESForm* current = factory->Create();
        current->SetFormID(id, false);
        instance->Undelete(current, baseForm->GetFormType());
        createdRecord = true;
    }
    else if (!instance) {
        logger::warn("instance not found creating it");
        auto factory = RE::IFormFactory::GetFormFactoryByType(baseForm->GetFormType());
        RE::TESForm* current = factory->Create();
        current->SetFormID(id, false);
        instance = FormRecord::CreateNew(current, baseForm->GetFormType(), id);
        createdRecord = true;
        AddFormData(instance);
    }

    instance->baseForm = baseForm;
    instance->modelForm = modelForm;
    instance->formId = id;


    applyPattern(instance);


    return createdRecord;
}

template <typename T>
static void RestoreFormRecordData(Serializer<T>* serializer, FormRecord* instance) {

    serializer->startReadingSection();

    auto deleted = serializer->Read<char>();

    if (deleted == 1) {
        logger::info("deleted");
        serializer->finishReadingSection();
        return;
    }

    if (!instance->actualForm) {
        logger::info("missing actual form");
        serializer->finishReadingSection();
        return; 
    }

    RestoreEachFormData(serializer, instance);

    serializer->finishReadingSection();
}