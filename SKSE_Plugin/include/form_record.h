#pragma once

class FormRecord {
     FormRecord() {}
     explicit FormRecord(RE::TESForm *_actualForm);

public:

    static FormRecord *CreateNew(RE::TESForm *_actualForm, RE::FormType formType, RE::FormID formId);
    static FormRecord *CreateReference(RE::TESForm *_actualForm);

    static FormRecord *CreateDeleted(RE::FormID formId);

    void UndeleteReference(RE::TESForm *_actualForm);

    void Undelete(RE::TESForm *_actualForm, RE::FormType _formType);

    bool Match(RE::TESForm *matchForm);

    bool Match(const RE::FormID matchForm) const { return matchForm == formId; }
    bool HasModel() const { return modelForm != nullptr; }

    RE::TESForm *baseForm = nullptr;
    RE::TESForm *actualForm = nullptr;
    RE::TESForm *modelForm = nullptr;
    bool deleted = false;
    RE::FormType formType{};
    RE::FormID formId{};
};



