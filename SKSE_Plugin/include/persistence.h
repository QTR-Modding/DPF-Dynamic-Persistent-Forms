#pragma once
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

bool LoadGlobalRegistry();
bool SaveGlobalRegistry();
std::string GetGlobalRegistryPath();

void SaveCallback(SKSE::SerializationInterface* a_intfc);

void LoadCallback(SKSE::SerializationInterface* a_intfc);
