#include "DataFile.h"

#include <cstring>

namespace {
const char MAGIC[4] = {'S', 'H', 'L', 'F'};
}

bool DataFile::create(const std::string& path) {
    close();

    // trunc — создать файл заново, даже если он уже был.
    file.open(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return false;

    std::memcpy(header.magic, MAGIC, 4);
    header.slotCount = 0;
    header.liveCount = 0;
    header.freeHead = NO_SLOT;
    writeHeader();
    return true;
}

bool DataFile::open(const std::string& path) {
    close();

    file.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0);
    file.read((char*)&header, sizeof(DataHeader));
    if (!file || std::memcmp(header.magic, MAGIC, 4) != 0) {
        close();
        return false;
    }
    return true;
}

void DataFile::close() {
    if (file.is_open()) {
        file.flush();
        file.close();
    }
    file.clear();  // сбрасываем флаги ошибок, чтобы поток можно было открыть снова
}

bool DataFile::isOpen() const {
    return file.is_open();
}

std::streamoff DataFile::offsetOf(int64_t index) const {
    return sizeof(DataHeader) + index * (std::streamoff)sizeof(Slot);
}

Slot DataFile::readSlot(int64_t index) {
    Slot slot{};
    file.seekg(offsetOf(index));
    file.read((char*)&slot, sizeof(Slot));
    file.clear();
    return slot;
}

void DataFile::writeSlot(int64_t index, const Slot& slot) {
    file.seekp(offsetOf(index));
    file.write((const char*)&slot, sizeof(Slot));
}

void DataFile::writeHeader() {
    file.seekp(0);
    file.write((const char*)&header, sizeof(DataHeader));
}

int64_t DataFile::allocateSlot() {
    int64_t index;

    if (header.freeHead != NO_SLOT) {
        index = header.freeHead;
        Slot freeSlot = readSlot(index);
        header.freeHead = freeSlot.nextFree;
    } else {
        index = header.slotCount;
        header.slotCount++;
    }

    header.liveCount++;
    writeHeader();
    return index;
}

void DataFile::freeSlot(int64_t index) {
    Slot empty{};
    empty.used = false;
    empty.prevSameAuthor = NO_SLOT;
    empty.nextSameAuthor = NO_SLOT;
    empty.nextFree = header.freeHead;
    writeSlot(index, empty);

    header.freeHead = index;
    header.liveCount--;
    writeHeader();
}

void DataFile::flush() {
    file.flush();
}
