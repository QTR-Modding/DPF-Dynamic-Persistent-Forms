#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>
#include "form_record.h"

inline std::vector<FormRecord*> formData;
inline std::vector<FormRecord*> formRef;

inline bool espFound = false;
inline uint32_t firstDynamicLocalId = 0x801;
inline uint32_t nextDynamicLocalId = 0x801;
inline uint32_t nextRecordOrder = 1;
inline uint32_t dynamicModId = 0;
inline std::string dynamicPluginName = "Dynamic Persistent Forms.esp";
inline std::unordered_set<uint32_t> reservedDynamicLocalIds;


void AddFormData(FormRecord* item);

void AddFormRef(FormRecord* item);

void EachFormData(const std::function<bool(FormRecord*)>& iteration);

void EachFormRef(const std::function<bool(FormRecord*)>& iteration);


bool IsDynamicFormID(RE::FormID formId);

uint32_t ToDynamicLocalID(RE::FormID formId);

RE::FormID MakeDynamicFormID(uint32_t localId);

void ReserveDynamicLocalID(uint32_t localId);

void ReserveDynamicFormID(RE::FormID formId);

RE::FormID AllocateDynamicFormID();

void ReadFirstFormIdFromESP();

void ResetDynamicState();

void ClearRecords(bool deleteActualForms);
