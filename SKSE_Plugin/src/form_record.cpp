#include "form_record.h"

FormRecord::FormRecord(RE::TESForm* _actualForm) {
    actualForm = _actualForm;
}

FormRecord* FormRecord::CreateNew(RE::TESForm* _actualForm, RE::FormType formType, RE::FormID formId) {
    if (!_actualForm) {
        return nullptr;
    }
    const auto result = new FormRecord(_actualForm);
    result->formType = formType;
    result->formId = formId;
    return result;
}

FormRecord* FormRecord::CreateReference(RE::TESForm* _actualForm) {
    if (!_actualForm) {
        return nullptr;
    }
    const auto result = new FormRecord(_actualForm);
    result->formId = _actualForm->GetFormID();
    return result;
}

FormRecord* FormRecord::CreateDeleted(RE::FormID formId) {
    const auto result = new FormRecord();
    result->formId = formId;
    result->deleted = true;
    return result;
}

void FormRecord::UndeleteReference(RE::TESForm* _actualForm) {
    if (!_actualForm) {
        return;
    }
    deleted = false;
    actualForm = _actualForm;
    formId = _actualForm->GetFormID();
}

void FormRecord::Undelete(RE::TESForm* _actualForm, RE::FormType _formType) {
    if (!_actualForm) {
        return;
    }
    actualForm = _actualForm;
    formType = _formType;
    deleted = false;
}

bool FormRecord::Match(RE::TESForm* matchForm) {
    if (!matchForm) {
        return false;
    }
    return matchForm->GetFormID() == formId;
}