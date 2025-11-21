#pragma once
#include <string>
#include <fstream>
#include <format>

static bool DoLog = false;


class FileLogger {
    private:
    static std::string fileName;
    static FileLogger* instance; 
    std::ofstream stream; 
    FileLogger() {
        if (!fileName.empty()) {
            stream.open(fileName, std::ios::app);
            if (!stream.is_open()) {
                stream.open(fileName);
                if (!stream.is_open()) {
                    return;
                }
            }
        }
    }
    public:
    static FileLogger* GetSingleton() {
        if (instance == nullptr) {
            instance = new FileLogger(); 
        }
        return instance;
    }
    static void SetFileName(std::string name) {
        fileName = std::move(name);
    }
    void Print(const std::string& message) {
        if (stream.is_open()) {
            stream << message << std::endl;
        }
    }

};

FileLogger* FileLogger::instance = nullptr;
std::string FileLogger::fileName = "";

void EnableLog(std::string filename, const std::string& message) {
    FileLogger::SetFileName(std::move(filename));
    FileLogger::GetSingleton()->Print(message);
    DoLog = true;
}

void print(const std::string message) {
    if (DoLog) {
        FileLogger::GetSingleton()->Print(message);
        //RE::ConsoleLog::GetSingleton()->Print(message);
    }
}
void print(const std::string message1, const std::string message2) {
    if (DoLog) {
        const auto msg = std::format("{0}: {1}", message1, message2);
        FileLogger::GetSingleton()->Print(msg);
         RE::ConsoleLog::GetSingleton()->Print(msg.c_str());
    }
}
void print(const std::string message, const float number) {
    if (DoLog) {
        const auto msg = std::format("{0}: {1}", message, number);
        FileLogger::GetSingleton()->Print(msg);
        RE::ConsoleLog::GetSingleton()->Print(msg.c_str());
    }
}
void printSize(const std::string message, const size_t number) {
    if (DoLog) {
        const auto msg = std::format("{0}: {1}", message, number);
        FileLogger::GetSingleton()->Print(msg);
        RE::ConsoleLog::GetSingleton()->Print(msg.c_str());
    }
}
void printInt(const std::string message, const uint32_t number) {
    if (DoLog) {
        const auto msg = std::format("{0}: {1}", message, number);
        FileLogger::GetSingleton()->Print(msg);
        RE::ConsoleLog::GetSingleton()->Print(msg.c_str());
    }
}
