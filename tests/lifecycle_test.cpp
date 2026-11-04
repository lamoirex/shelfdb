// Проверяем жизненный цикл базы: сохранение, очистку и удаление файлов.

#include "Database.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cout << "ОШИБКА: " << what << std::endl;
        failures++;
    }
}

bool allFilesExist(const std::string& base) {
    for (const std::string& f : Database::filesOf(base)) {
        if (!fs::exists(f)) return false;
    }
    return true;
}

bool noFilesExist(const std::string& base) {
    for (const std::string& f : Database::filesOf(base)) {
        if (fs::exists(f)) return false;
    }
    return true;
}

int main() {
    const std::string base = "lifecycle_test";
    Database db;
    Book found;

    // 1. Создание: появились три файла, путь запомнен.
    check(!db.isOpen() && db.basePath().empty(), "база изначально закрыта");
    check(db.create(base), "создание");
    check(db.isOpen() && db.basePath() == base, "база открыта, путь запомнен");
    check(allFilesExist(base), "созданы .dat, .pidx и .sidx");

    // 2. Сохранение: данные видны другому экземпляру, открытому параллельно.
    db.add(makeBook(1, "Война и мир", "Толстой", 1869, 500.0, true));
    db.add(makeBook(2, "Идиот", "Достоевский", 1869, 350.0, false));
    db.save();
    {
        Database other;
        check(other.open(base), "второй экземпляр открывает базу");
        check(other.count() == 2 && other.findById(2, found), "после save данные уже на диске");
    }

    // 3. Очистка: файлы на месте, книг нет, индексы пустые.
    check(db.clear(), "очистка");
    check(db.isOpen() && db.count() == 0, "база пуста и открыта");
    check(!db.findById(1, found) && db.findByAuthor("Толстой").empty(), "индексы очищены");
    check(db.add(makeBook(1, "Снова", "Толстой", 1869, 1.0, true)), "после очистки можно добавлять");
    check(allFilesExist(base), "файлы на месте");

    // 4. Удаление: база закрыта, файлов нет.
    check(db.destroy(), "удаление");
    check(!db.isOpen(), "база закрыта");
    check(noFilesExist(base), "файлы удалены");
    check(!db.open(base), "удалённую базу нельзя открыть");

    // 5. Операции над закрытой базой ничего не ломают.
    check(!db.clear() && !db.destroy(), "clear и destroy на закрытой базе возвращают false");

    if (failures == 0) {
        std::cout << "lifecycle_test: все проверки пройдены" << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
