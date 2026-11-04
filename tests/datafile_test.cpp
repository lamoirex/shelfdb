// Проверяем файл данных: запись слотов, переиспользование свободных, повторное открытие.

#include "DataFile.h"

#include <cstdio>
#include <iostream>

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cout << "ОШИБКА: " << what << std::endl;
        failures++;
    }
}

Slot makeSlot(const Book& book) {
    Slot slot{};
    slot.used = true;
    slot.book = book;
    slot.prevSameAuthor = NO_SLOT;
    slot.nextSameAuthor = NO_SLOT;
    slot.nextFree = NO_SLOT;
    return slot;
}

int main() {
    const char* path = "datafile_test.dat";
    DataFile data;

    // 1. Новый файл пустой.
    check(data.create(path), "создание файла");
    check(data.slotCount() == 0 && data.liveCount() == 0, "новый файл пустой");

    // 2. Три книги попадают в слоты 0, 1, 2.
    for (int i = 0; i < 3; i++) {
        int64_t slot = data.allocateSlot();
        check(slot == i, "слоты выдаются по порядку");
        data.writeSlot(slot, makeSlot(makeBook(100 + i, "Книга", "Автор", 2000 + i, 10.0 * i, true)));
    }
    check(data.slotCount() == 3 && data.liveCount() == 3, "три слота заняты");
    check(data.readSlot(1).book.id == 101, "чтение слота по номеру");

    // 3. Освобождаем слот 1 — следующая книга должна занять именно его.
    data.freeSlot(1);
    check(!data.readSlot(1).used, "слот 1 помечен свободным");
    check(data.liveCount() == 2, "живых записей стало 2");

    int64_t reused = data.allocateSlot();
    check(reused == 1, "свободный слот используется повторно");
    check(data.slotCount() == 3, "файл не вырос");
    data.writeSlot(reused, makeSlot(makeBook(200, "Новая", "Другой", 2024, 1.5, false)));

    // 4. Освобождаем два слота: список свободных работает как стек.
    data.freeSlot(0);
    data.freeSlot(2);
    check(data.allocateSlot() == 2, "первым выдаётся последний освобождённый");
    check(data.allocateSlot() == 0, "потом предыдущий");
    check(data.allocateSlot() == 3, "потом новый в конце файла");
    data.close();

    // 5. После повторного открытия всё на месте.
    check(data.open(path), "повторное открытие");
    check(data.slotCount() == 4 && data.liveCount() == 4, "заголовок сохранился");
    check(data.readSlot(1).book.id == 200, "книга сохранилась");
    check(getText(data.readSlot(1).book.title, TITLE_LEN) == "Новая", "строка сохранилась");
    data.close();

    // 6. Чужой файл не открывается.
    std::FILE* f = std::fopen("not_ours.dat", "wb");
    std::fputs("hello, world! this is not a database", f);
    std::fclose(f);
    check(!data.open("not_ours.dat"), "чужой файл отклонён");

    std::remove(path);
    std::remove("not_ours.dat");

    if (failures == 0) {
        std::cout << "datafile_test: все проверки пройдены" << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
