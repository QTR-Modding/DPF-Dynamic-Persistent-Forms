#pragma once
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

std::string BuildRegistryJson();
bool RestoreRegistryJson(const std::string& json);
bool LoadGlobalRegistry();
bool SaveGlobalRegistry();
std::string GetGlobalRegistryPath();

void SaveCallback(SKSE::SerializationInterface* a_intfc);

void LoadCallback(SKSE::SerializationInterface* a_intfc);
