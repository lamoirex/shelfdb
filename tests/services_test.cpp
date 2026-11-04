// Проверяем резервные копии и экспорт.

#include "Backup.h"
#include "Database.h"
#include "Exporter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cout << "ОШИБКА: " << what << std::endl;
        failures++;
    }
}

std::string readAll(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main() {
    const fs::path dir = "services_test_dir";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string base = (dir / "books").string();

    Database db;
    Book found;
    db.create(base);
    db.add(makeBook(1, "Война и мир", "Толстой", 1869, 500.0, true));
    db.add(makeBook(2, "Анна \"Каренина\"", "Толстой", 1877, 450.5, false));

    // ---------- резервная копия ----------

    // 1. Копия создаётся в папке backups рядом с базой.
    std::string backup = Backup::create(db);
    check(!backup.empty(), "копия создана");
    check(fs::path(backup).parent_path() == dir / "backups", "копия лежит в backups");
    for (const std::string& f : Database::filesOf(base)) {
        check(fs::exists(fs::path(backup) / fs::path(f).filename()), "в копии есть " + fs::path(f).filename().string());
    }

    // 2. Вторая копия за ту же секунду не перезаписывает первую.
    std::string backup2 = Backup::create(db);
    check(!backup2.empty() && backup2 != backup, "вторая копия в отдельной папке");

    // 3. Меняем базу, потом восстанавливаем — изменения откатываются.
    db.removeById(1);
    db.add(makeBook(3, "Новая", "Кто-то", 2024, 1.0, true));
    check(Backup::restore(db, backup), "восстановление");
    check(db.isOpen() && db.count() == 2, "в базе снова две книги");
    check(db.findById(1, found) && !db.findById(3, found), "книги как в момент копии");
    check(db.findByAuthor("Толстой").size() == 2, "индекс по автору тоже восстановлен");

    // 4. Из повреждённой копии не восстанавливаем, база остаётся прежней.
    fs::path broken = dir / "broken";
    fs::create_directories(broken);
    std::ofstream(broken / "books.dat") << "это не база";
    check(!Backup::restore(db, broken.string()), "повреждённая копия отклонена");
    check(db.isOpen() && db.count() == 2, "база не пострадала");
    check(!Backup::restore(db, (dir / "nothing").string()), "несуществующая папка отклонена");

    // ---------- экспорт ----------

    // 5. CSV.
    std::string csvPath = (dir / "books.csv").string();
    check(Exporter::toCsv(db, csvPath), "экспорт в CSV");
    std::string csv = readAll(csvPath);
    check(csv.rfind("\xEF\xBB\xBF", 0) == 0, "CSV начинается с метки UTF-8");
    check(csv.find("\"ID\";\"Название\";\"Автор\"") != std::string::npos, "заголовок CSV");
    check(csv.find("1;\"Война и мир\";\"Толстой\";1869;500.00;да") != std::string::npos, "строка CSV");
    check(csv.find("\"Анна \"\"Каренина\"\"\"") != std::string::npos, "кавычки в CSV удвоены");

    // 6. XLSX: это zip-архив с листом, в котором есть наши данные.
    std::string xlsxPath = (dir / "books.xlsx").string();
    check(Exporter::toXlsx(db, xlsxPath), "экспорт в xlsx");
    std::string xlsx = readAll(xlsxPath);
    check(xlsx.rfind("PK\x03\x04", 0) == 0, "xlsx — это zip-архив");
    check(xlsx.find("xl/worksheets/sheet1.xml") != std::string::npos, "в архиве есть лист");
    check(xlsx.find("Война и мир") != std::string::npos, "в листе есть данные");
    check(xlsx.find("&quot;Каренина&quot;") != std::string::npos, "кавычки экранированы для XML");

    db.close();
    fs::remove_all(dir);

    if (failures == 0) {
        std::cout << "services_test: все проверки пройдены" << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
