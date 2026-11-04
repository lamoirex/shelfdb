#pragma once

#include "Book.h"

#include <cstdint>
#include <fstream>
#include <string>

// Номер слота, которого нет. Используется как "нулевой указатель" в файле.
const int64_t NO_SLOT = -1;

#pragma pack(push, 1)

// Заголовок в начале файла данных.
struct DataHeader {
    char magic[4];       // подпись "SHLF", чтобы отличить наш файл от чужого
    int64_t slotCount;   // сколько всего слотов в файле (и занятых, и свободных)
    int64_t liveCount;   // сколько слотов занято книгами
    int64_t freeHead;    // первый свободный слот или NO_SLOT
};

// Слот — ячейка файла фиксированного размера. В ней лежит одна книга
// и служебные ссылки (номера других слотов).
struct Slot {
    bool used;               // true — в слоте книга, false — слот свободен
    Book book;
    int64_t prevSameAuthor;  // предыдущая книга того же автора (для вторичного индекса)
    int64_t nextSameAuthor;  // следующая книга того же автора
    int64_t nextFree;        // следующий свободный слот (если этот слот свободен)
};

#pragma pack(pop)

// Файл с книгами.
//
// Устройство файла:
//   [DataHeader][Slot 0][Slot 1][Slot 2]...
//
// Все слоты одного размера, поэтому слот номер i лежит по смещению
//   sizeof(DataHeader) + i * sizeof(Slot)
// и читается или записывается за O(1) через seek.
//
// Удалённые слоты не вырезаются из файла (это было бы O(n)),
// а помечаются свободными и связываются в список. Следующая добавленная
// книга займёт свободный слот, поэтому файл не растёт без нужды.
class DataFile {
public:
    // Создаёт новый пустой файл (старый с тем же именем стирается).
    bool create(const std::string& path);

    // Открывает существующий файл. false, если файла нет или он не наш.
    bool open(const std::string& path);

    void close();
    bool isOpen() const;

    Slot readSlot(int64_t index);
    void writeSlot(int64_t index, const Slot& slot);

    // Выдаёт слот под новую книгу: свободный из списка или новый в конце файла. O(1).
    int64_t allocateSlot();

    // Помечает слот свободным и кладёт его в начало списка свободных. O(1).
    void freeSlot(int64_t index);

    int64_t slotCount() const { return header.slotCount; }
    int64_t liveCount() const { return header.liveCount; }

    // Сбрасывает буферы на диск.
    void flush();

private:
    void writeHeader();
    std::streamoff offsetOf(int64_t index) const;

    std::fstream file;
    DataHeader header{};
};
