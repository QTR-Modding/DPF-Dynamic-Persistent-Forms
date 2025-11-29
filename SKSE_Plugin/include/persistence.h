#pragma once

void SaveCallback(SKSE::SerializationInterface* a_intfc);
void LoadCallback(SKSE::SerializationInterface* a_intfc);


namespace Persistence {
    void Load(std::string fileName);
    void Save(std::string fileName);
}