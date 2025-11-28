#pragma once
#include "model.h"
#include <cstdint>
#include <stack>
#include <sstream>
#include <functional>
#include <string>
#include <cstring>

class StreamWrapper {
    std::stringstream stream;
public:
        void Clear();
    void SeekBeginning();

    template <class T>
    void Write(T value) { stream.write(reinterpret_cast<const char*>(&value), sizeof(T)); }
        
    template <class T>
    T Read() {
        T result;
        if (stream.read(reinterpret_cast<char*>(&result), sizeof(T))) {
            return result;
        }
        return T();
    }
    
    uint32_t Size();

    void WriteDown(std::function<void(uint32_t)> const& start, std::function<void(char)> const& step);

    void ReadOut(std::function<uint32_t()> start, std::function<char()> const& step);
};

template <typename Derived>
class Serializer {
    
    std::stack<StreamWrapper*> ctx;

    template <class T>
    void WriteTarget(T value) {
        static_cast<Derived*>(this)->template WriteImplementation<T>(value);
    }
    template <class T>
    T ReadSource() {
        return static_cast<Derived*>(this)->template ReadImplementation<T>();
    }

protected:
    bool error = false;
public:

    ~Serializer();

    template <class T>
    void Write(T value) {
        if (!ctx.empty()) {
            ctx.top()->Write<T>(value);
        } else {
            WriteTarget<T>(value);
        }
    }
    template <class T>
    T Read() {
        if (!ctx.empty()) {
            return ctx.top()->Read<T>();
        } else {
            return ReadSource<T>();
        }
    }

    void StartWritingSection() {
        ctx.push(new StreamWrapper());
    }

    void finishWritingSection() {
        if (ctx.size() == 1) {
            auto body = ctx.top();
            ctx.pop();
            body->WriteDown(
                [&](uint32_t size) { WriteTarget<uint32_t>(size); }, 
                [&](char item) { WriteTarget<char>(item); }
            );
            delete body;
        } else if (ctx.size() > 1) {
            auto body = ctx.top();
            ctx.pop();
            body->WriteDown(
                [&](uint32_t size) { ctx.top()->Write<uint32_t>(size); }, 
                [&](char item) { ctx.top()->Write<char>(item); }
            );
            delete body;
        }
    }

    void startReadingSection() {

        if (ctx.size() == 0) {
            ctx.push(new StreamWrapper());
            ctx.top()->ReadOut(
                [&]() { return ReadSource<uint32_t>(); }, 
                [&]() { return ReadSource<char>(); }
            );
        } else {
            const auto parent = ctx.top();
            ctx.push(new StreamWrapper());
            ctx.top()->ReadOut(
                [&]() { return parent->Read<uint32_t>(); }, 
                [&]() { return parent->Read<char>(); }
            );
        }
    }

    void finishReadingSection() {
        if (ctx.size() > 0) {
            const auto top = ctx.top();
            ctx.pop();
            delete top;
        }
    }
    template<class T>
    T* ReadFormRef() {
        const auto item = ReadFormId();
        if (item == 0) {
            return nullptr;
        }
        const auto form = RE::TESForm::LookupByID(item);
        if (!form) {
            return nullptr;
        }
        return form->As<T>();
    }

    RE::TESForm* ReadFormRef() {
        const auto item = ReadFormId();
        if (item == 0) {
            return nullptr;
        }
        return RE::TESForm::LookupByID(item);
    }

    void WriteFormRef(RE::TESForm* item) {
        if (item) {
            WriteFormId(item->GetFormID());
        } else {
            WriteFormId(0);
        }
    }

    RE::FormID ReadFormId() {
        const auto dataHandler = RE::TESDataHandler::GetSingleton();
        const char fileRef = Read<char>();
        logger::trace("fileref", fileRef);

        if (fileRef == 0) {
            return 0;
        }

        if (fileRef == 1) {
            const uint32_t dynamicId = Read<uint32_t>();
            return dynamicId + (dynamicModId << 24);
        }
        else if(fileRef == 2){
            const std::string fileName = ReadString();
            const uint32_t localId = Read<uint32_t>();
            const auto formId = dataHandler->LookupFormID(localId, fileName);
            logger::trace("localid", formId);
            logger::trace("modname", fileName);
            logger::trace("id", formId);
            return formId;
        }

        return 0;


    }

    void WriteFormId(RE::FormID formId) {
        logger::trace("formid", formId);
        if (formId == 0) {
            logger::trace("zero");
            Write<char>(0);
            return;
        }

        const auto dataHandler = RE::TESDataHandler::GetSingleton();

        const auto modId = (formId >> 24) & 0xff;

        logger::trace("mid", modId);
        if (modId == dynamicModId) {
            logger::trace("dynamic");
            const auto localId = formId & 0xFFFFFF;
            Write<char>(1);
            Write<uint32_t>(localId);
        }
        else if (modId == 0xfe) {
            logger::trace("light");
            const auto lightId = (formId >> 12) & 0xFFF;
            const auto file = dataHandler->LookupLoadedLightModByIndex(lightId);
            if (file) {
                const auto localId = formId & 0xFFF;
                const std::string fileName = file->fileName;
                Write<char>(2);
                WriteString(fileName.c_str());
                Write<uint32_t>(localId);
            } else {
                Write<char>(0);
                logger::error("missing file");
            }
        } 
        else {
            logger::trace("regular");
            const auto file = dataHandler->LookupLoadedModByIndex(modId);
            if (file) {
                const auto localId = formId & 0xFFFFFF;
                const std::string fileName = file->fileName;
                Write<char>(2);
                WriteString(fileName.c_str());
                Write<uint32_t>(localId);
            } else {
                Write<char>(0);
                logger::error("missing file");
            }
        }


    }

    char* ReadString() {
        const size_t arrayLength = Read<uint32_t>();
        char* result = new char[arrayLength+1];
        for (size_t i = 0; i < arrayLength; i++) {
            result[i] = Read<char>();
        }
        result[arrayLength] = '\0';
        return result;
    }

    void WriteString(const char* string) {
        const size_t arrayLength = strlen(string);
        Write<uint32_t>(static_cast<uint32_t>(arrayLength));
        for (size_t i = 0; i < arrayLength; i++) {
            const char item = string[i];
            Write<char>(item);
        }
    }
};

template <typename Derived>
Serializer<Derived>::~Serializer() {
    while (!ctx.empty()) {
        delete ctx.top();
        ctx.pop();      
    }
}

class SaveDataSerializer : public Serializer<SaveDataSerializer> {
    SKSE::SerializationInterface* a_intfc;
public:
    SaveDataSerializer(SKSE::SerializationInterface* _a_intfc);

    template <class T>
    void WriteImplementation(T item) {
        a_intfc->WriteRecordData(item);
    }

    template <class T>
    T ReadImplementation() {
        T item;
        auto success = a_intfc->ReadRecordData(item);
        if (!success) {
            logger::error("error reading");
            error = true;
        }
        return item;
    }
};

class FileWriter : public Serializer<FileWriter> {
    std::ofstream fileStream;

public:
    FileWriter(const std::string& filename, std::ios_base::openmode _Mode = std::ios_base::out);

    ~FileWriter();

    bool IsOpen() { return fileStream.is_open(); }

    template <class T>
    T ReadImplementation() {
        return T();
    }

    template <class T>
    void WriteImplementation(T value) {
        if (fileStream.is_open()) {
            fileStream.write(reinterpret_cast<const char*>(&value), sizeof(T));
        } else {
            logger::error("Error: File not open for writing.");
        }
    }
};

class FileReader : public Serializer<FileReader> {
    std::ifstream fileStream;

public:
    FileReader(const std::string& filename, std::ios_base::openmode _Mode = std::ios_base::in);

    ~FileReader();

    bool IsOpen() { return fileStream.is_open(); }

    template <class T>
    void WriteImplementation(T) {}
    template <class T>
    T ReadImplementation() {
        T value;
        if (fileStream.is_open()) {
            fileStream.read(reinterpret_cast<char*>(&value), sizeof(T));
            return value;
        } else {
            logger::error("Error: File not open for reading.");
        }
        return T();
    }
};