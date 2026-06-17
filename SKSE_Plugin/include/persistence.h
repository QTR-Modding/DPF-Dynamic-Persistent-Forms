#pragma once
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

std::string BuildStateJson();
bool RestoreStateJson(const std::string& json);

void SaveCallback(SKSE::SerializationInterface* a_intfc);

void LoadCallback(SKSE::SerializationInterface* a_intfc);
