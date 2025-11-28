#pragma once
#include <vector>
#include <functional>
#include <cstdint>
#include "form_record.h"

inline std::vector<FormRecord*> formData;
inline std::vector<FormRecord*> formRef;

inline bool espFound = false;
inline uint32_t lastFormId = 0;
inline uint32_t firstFormId = 0;
inline uint32_t dynamicModId = 0;


void AddFormData(FormRecord* item);

void AddFormRef(FormRecord* item);

void EachFormData(std::function<bool(FormRecord*)> const& iteration);

void EachFormRef(std::function<bool(FormRecord*)> const& iteration);


void incrementLastFormID();


void UpdateId();

void ResetId();

void ReadFirstFormIdFromESP();