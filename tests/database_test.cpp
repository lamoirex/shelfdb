// Проверяем базу данных: добавление, поиск, удаление и редактирование по id.

#include "Database.h"

#include <cstdio>
#include <iostream>
#include <string>

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cout << "ОШИБКА: " << what << std::endl;
        failures++;
    }
}

int main() {
    const std::string base = "database_test";
    Database db;
    Book found;

    // 1. Добавление и поиск.
    check(db.create(base), "создание базы");
    check(db.add(makeBook(1, "Война и мир", "Толстой", 1869, 500.0, true)), "добавление книги 1");
    check(db.add(makeBook(2, "Анна Каренина", "Толстой", 1877, 450.0, false)), "добавление книги 2");
    check(db.count() == 2, "в базе две книги");

    check(db.findById(1, found), "книга 1 находится");
    check(getText(found.title, TITLE_LEN) == "Война и мир", "у книги 1 верное название");
    check(!db.findById(3, found), "несуществующая книга не находится");

    // 2. Уникальность ключа.
    check(!db.add(makeBook(1, "Дубликат", "Кто-то", 2000, 1.0, true)), "дубликат id отклонён");
    db.findById(1, found);
    check(getText(found.title, TITLE_LEN) == "Война и мир", "дубликат не испортил книгу");

    // 3. Удаление по id.
    check(db.removeById(1), "удаление книги 1");
    check(!db.findById(1, found), "удалённая книга не находится");
    check(!db.removeById(1), "повторное удаление возвращает false");
    check(db.count() == 1, "в базе одна книга");
    check(db.add(makeBook(1, "Снова", "Толстой", 1869, 1.0, true)), "id после удаления снова свободен");

    // 4. Редактирование без смены id.
    check(db.update(2, makeBook(2, "Анна Каренина", "Толстой", 1878, 999.0, true)), "редактирование");
    db.findById(2, found);
    check(found.year == 1878 && found.price == 999.0 && found.available, "поля изменились");

    // 5. Редактирование со сменой id.
    check(!db.update(2, makeBook(1, "x", "x", 1, 1.0, true)), "смена id на занятый отклонена");
    check(db.update(2, makeBook(20, "Анна Каренина", "Толстой", 1878, 999.0, true)), "смена id на свободный");
    check(!db.findById(2, found), "старый id больше не находится");
    check(db.findById(20, found), "новый id находится");
    check(!db.update(777, makeBook(777, "x", "x", 1, 1.0, true)), "редактирование несуществующей книги");

    // 6. Много записей.
    db.create(base);
    for (int i = 0; i < 20000; i++) {
        db.add(makeBook(i, "Книга " + std::to_string(i), "Автор", 2000, i * 1.5, true));
    }
    for (int i = 0; i < 20000; i += 2) db.removeById(i);
    bool ok = db.count() == 10000;
    for (int i = 0; i < 20000; i++) {
        bool exists = db.findById(i, found);
        if (exists != (i % 2 == 1)) ok = false;
        if (exists && found.price != i * 1.5) ok = false;
    }
    check(ok, "20 000 добавлений и 10 000 удалений");
    db.close();

    // 7. После повторного открытия всё на месте.
    check(db.open(base), "повторное открытие");
    check(db.count() == 10000, "количество сохранилось");
    check(db.findById(19999, found) && getText(found.title, TITLE_LEN) == "Книга 19999", "книга сохранилась");
    db.close();

    std::remove((base + ".dat").c_str());
    std::remove((base + ".pidx").c_str());
    std::remove((base + ".sidx").c_str());

    if (failures == 0) {
        std::cout << "database_test: все проверки пройдены" << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
