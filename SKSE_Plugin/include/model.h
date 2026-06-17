#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "form_record.h"

struct DynamicSlot {
    uint32_t localId = 0;
    RE::FormType formType = RE::FormType::None;
    std::string owner;
    std::string key;
};

inline std::vector<FormRecord*> formData;
inline std::vector<FormRecord*> formRef;

inline bool espFound = false;
inline uint32_t firstDynamicLocalId = 0x801;
inline uint32_t nextDynamicLocalId = 0x801;
inline uint32_t nextRecordOrder = 1;
inline uint32_t dynamicModId = 0;
inline std::string dynamicPluginName = "Dynamic Persistent Forms.esp";
inline std::unordered_set<uint32_t> reservedDynamicLocalIds;
inline std::unordered_map<uint32_t, DynamicSlot> dynamicSlots;
inline std::unordered_map<std::string, uint32_t> dynamicOwnerKeyIndex;

void AddFormData(FormRecord* item);

void AddFormRef(FormRecord* item);

void EachFormData(const std::function<bool(FormRecord*)>& iteration);

void EachFormRef(const std::function<bool(FormRecord*)>& iteration);

bool IsDynamicFormID(RE::FormID formId);

uint32_t ToDynamicLocalID(RE::FormID formId);

RE::FormID MakeDynamicFormID(uint32_t localId);

void ReserveDynamicLocalID(uint32_t localId);

void ReserveDynamicFormID(RE::FormID formId);

std::string NormalizeOwnerKeyPart(const char* value);

std::string MakeOwnerKey(std::string_view owner, std::string_view key);

bool RegisterDynamicSlot(uint32_t localId, RE::FormType formType, std::string owner = {}, std::string key = {});

std::optional<DynamicSlot> GetRegisteredDynamicSlot(uint32_t localId);

std::optional<RE::FormType> GetRegisteredDynamicSlotType(uint32_t localId);

std::optional<uint32_t> FindDynamicSlotByOwnerKey(std::string_view owner, std::string_view key);

bool IsDynamicLocalIDRegistered(uint32_t localId);

bool ReleaseDynamicSlot(uint32_t localId, std::string_view owner);

bool ReleaseDynamicSlotByOwnerKey(std::string_view owner, std::string_view key);

uint32_t ReleaseDynamicSlotsByOwner(std::string_view owner);

RE::FormID AllocateDynamicFormID();

void ReadFirstFormIdFromESP();

void ResetDynamicState();

void ClearRecords(bool deleteActualForms);
