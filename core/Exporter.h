#pragma once

#include "Database.h"

#include <string>

// Выгрузка всей базы в сторонние форматы.
// Книги читаются из файла базы по одной (через Database::forEach).
namespace Exporter {

// CSV: текст, поля через ";". Excel открывает его двойным щелчком.
bool toCsv(Database& db, const std::string& path);

// Настоящий файл Excel.
bool toXlsx(Database& db, const std::string& path);

}  // namespace Exporter
