#pragma once

#include "Database.h"

#include <string>

namespace Exporter {

bool toCsv(Database& db, const std::string& path);

bool toXlsx(Database& db, const std::string& path);

}
