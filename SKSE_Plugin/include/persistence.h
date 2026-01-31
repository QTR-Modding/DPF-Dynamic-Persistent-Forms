#pragma once
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

std::string GetCacheFilePath();
void LoadCache();
void SaveCache();

void SaveCallback(SKSE::SerializationInterface* a_intfc);

void LoadCallback(SKSE::SerializationInterface* a_intfc);
