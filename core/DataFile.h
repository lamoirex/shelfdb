#pragma once

#include "Book.h"

#include <cstdint>
#include <fstream>
#include <string>

const int64_t NO_SLOT = -1;

#pragma pack(push, 1)

struct DataHeader {
    char magic[4];       // подпись "SHLF", чтобы отличить наш файл от чужого
    int64_t slotCount;   // сколько всего слотов в файле (и занятых, и свободных)
    int64_t liveCount;   // сколько слотов занято книгами
    int64_t freeHead;    // первый свободный слот или NO_SLOT
};

struct Slot {
    bool used;               // true — в слоте книга, false — слот свободен
    Book book;
    int64_t prevSameAuthor;  // предыдущая книга того же автора (для вторичного индекса)
    int64_t nextSameAuthor;  // следующая книга того же автора
    int64_t nextFree;        // следующий свободный слот (если этот слот свободен)
};

#pragma pack(pop)

class DataFile {
public:
    bool create(const std::string& path);

    bool open(const std::string& path);

    void close();
    bool isOpen() const;

    Slot readSlot(int64_t index);
    void writeSlot(int64_t index, const Slot& slot);

    int64_t allocateSlot();

    void freeSlot(int64_t index);

    int64_t slotCount() const { return header.slotCount; }
    int64_t liveCount() const { return header.liveCount; }

    void flush();

private:
    void writeHeader();
    std::streamoff offsetOf(int64_t index) const;

    std::fstream file;
    DataHeader header{};
};
