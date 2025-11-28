#pragma once
#include "model.h"
#include "form.h"
#include "serializer.h"
#include "form_serializer.h"
#include "form_record_serializer.h"
#include <mutex>
#include <filesystem> // Necessário para manipular caminhos e diretórios

// Alias para facilitar
namespace fs = std::filesystem;

std::mutex callbackMutext;

void LoadCache();
void SaveCache();

// Função robusta para pegar o caminho
inline std::string GetCachePath() {
    try {
        // Pega o diretório raiz do jogo (onde está o SkyrimSE.exe)
        fs::path path = fs::current_path();

        // Constrói o caminho: Data/SKSE/Plugins
        path /= "Data";
        path /= "SKSE";
        path /= "Plugins";

        // Verifica se o diretório existe, se não, cria
        if (!fs::exists(path)) {
            fs::create_directories(path);
            print("Diretório de cache criado em Data/SKSE/Plugins");
        }

        // Adiciona o nome do arquivo
        path /= "DynamicPersistentFormsCache.bin";

        // Retorna como string para o FileWriter
        return path.string();
    }
    catch (const std::exception& e) {
        print("ERRO CRITICO ao resolver caminho do cache: ");
        print(e.what());
        return "DynamicPersistentFormsCache.bin"; // Fallback de emergência
    }
}

static void SaveCallback(SKSE::SerializationInterface* a_intfc) {
    std::lock_guard<std::mutex> lock(callbackMutext);
    try {
        print("SAVE CALLBACK - SKSE Co-Save");
        if (!a_intfc->OpenRecord('ARR_', 1)) {
            print("Failed to open record for arr!");
        }
        else {
            const auto serializer = new SaveDataSerializer(a_intfc);
            StoreAllFormRecords(serializer);
            delete serializer; // Importante: deletar o serializer se foi alocado com new
        }

        // Salva o cache global (.bin) toda vez que o jogo salva
        SaveCache();
    }
    catch (const std::exception&) {
        print("error saving");
    }
}

static void LoadCallback(SKSE::SerializationInterface* a_intfc) {
    std::lock_guard<std::mutex> lock(callbackMutext);
    try {
        print("LOAD CALLBACK - SKSE Co-Save");

        uint32_t type;
        uint32_t version;
        uint32_t length;
        bool refreshGame = false;

        while (a_intfc->GetNextRecordInfo(type, version, length)) {
            switch (type) {
            case 'ARR_': {
                const auto serializer = new SaveDataSerializer(a_intfc);
                refreshGame = RestoreAllFormRecords(serializer);
                delete serializer;
            } break;
            default:
                print("Unrecognized signature type!");
                break;
            }
        }
        if (refreshGame) {
            // Se recuperamos dados do save, atualizamos o cache global também
            SaveCache();
            UpdateId();
            // KillImmediate pode ser perigoso dependendo do contexto, mas mantive sua lógica
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player) player->KillImmediate();
        }

        print("CALLBACK LOADED");
    }
    catch (const std::exception&) {
        print("error loading");
    }
}

void SaveCache() {
    print("Tentando salvar cache global (.bin)...");

    std::string path = GetCachePath();

    // Verifica se o caminho veio vazio ou inválido
    if (path.empty()) {
        print("Caminho do cache inválido.");
        return;
    }

    // Usa std::ios::binary e trunc para sobrescrever
    const auto fileWriter = new FileWriter(path, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!fileWriter->IsOpen()) {
        print("ERRO: Nao foi possivel criar o arquivo de cache em: ");
        print(path.c_str());
        // Tente verificar permissões da pasta Data/SKSE/Plugins
        delete fileWriter;
        return;
    }

    StoreAllFormRecords(fileWriter);

    delete fileWriter; // Fecha o arquivo
    print("Cache global salvo com sucesso.");
}

void LoadCache() {
    SKSE::log::info("Tentando carregar cache global (.bin)...");

    std::string path = GetCachePath();

    // Verifica se o arquivo existe antes de tentar abrir para evitar erros desnecessários
    if (!fs::exists(path)) {
        print("Arquivo de cache nao existe ainda (primeira execucao?): ");
        print(path.c_str());
        return;
    }

    const auto fileReader = new FileReader(path, std::ios::in | std::ios::binary);

    if (!fileReader->IsOpen()) {
        print("Arquivo existe mas nao pode ser aberto (permissao?): ");
        print(path.c_str());
        delete fileReader;
        return;
    }

    RestoreAllFormRecords(fileReader);
    UpdateId();

    delete fileReader;
    print("Cache global carregado com sucesso.");
}