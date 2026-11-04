#pragma once

#include "Database.h"

#include <string>

namespace Backup {

std::string create(Database& db);

bool restore(Database& db, const std::string& backupFolder);

bool restoreTo(Database& db, const std::string& backupFolder, const std::string& basePath);

}
