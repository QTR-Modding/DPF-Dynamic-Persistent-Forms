#include "serializer.h"

void StreamWrapper::Clear() {
    stream.str("");
    stream.clear();
    stream.seekg(0);
}

void StreamWrapper::SeekBeginning() { stream.seekg(0); }

uint32_t StreamWrapper::Size() {
    const std::streampos currentPosition = stream.tellg();
    stream.seekg(0, std::ios::end);
    const size_t size = stream.tellg();
    stream.seekg(currentPosition);
    return static_cast<uint32_t>(size);
}

void StreamWrapper::WriteDown(const std::function<void(uint32_t)>& start, const std::function<void(char)>& step) {
    const auto size = Size();
    start(size);
    SeekBeginning();
    for (size_t i = 0; i < size; i++) {
        step(static_cast<char>(stream.get()));
    }
    Clear();
}

void StreamWrapper::ReadOut(std::function<uint32_t()> start, const std::function<char()>& step) {
    Clear();
    const uint32_t arrayLength = start();
    for (size_t i = 0; i < arrayLength; i++) {
        stream.put(step());
    }
    SeekBeginning();
}

SaveDataSerializer::SaveDataSerializer(SKSE::SerializationInterface* _a_intfc) { a_intfc = _a_intfc; }

FileWriter::FileWriter(const std::string& filename, const std::ios_base::openmode _Mode) {
    fileStream.open(filename, _Mode);
    if (!fileStream.is_open()) {
        logger::error("Error: Unable to open file ");
    }
}

FileWriter::~FileWriter() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

FileReader::FileReader(const std::string& filename, const std::ios_base::openmode _Mode) {
    fileStream.open(filename, _Mode);
    if (!fileStream.is_open()) {
        logger::error("Error: Unable to open file");
    }
}

FileReader::~FileReader() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}