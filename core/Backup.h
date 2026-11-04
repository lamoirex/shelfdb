#pragma once

#include "Database.h"

#include <string>

// Резервные копии базы.
//
// Копия — это папка рядом с базой, в которую скопированы все три файла:
//   /Users/me/backups/books_2026-11-04_12-30-05/books.dat
//                                               books.pidx
//                                               books.sidx
namespace Backup {

// Сохраняет базу и копирует её файлы в новую папку backups/<имя>_<дата-время>.
// Возвращает путь к папке копии или "" при ошибке.
std::string create(Database& db);

// Заменяет файлы открытой базы файлами из папки копии и открывает базу заново.
// Если копия повреждена, база остаётся в прежнем состоянии.
bool restore(Database& db, const std::string& backupFolder);

// Восстанавливает копию в базу по пути basePath (без расширения) и открывает её в db.
// Подходит и для случая, когда исходная база уже удалена.
bool restoreTo(Database& db, const std::string& backupFolder, const std::string& basePath);

}  // namespace Backup
