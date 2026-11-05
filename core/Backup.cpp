#include "Backup.h"

#include <ctime>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

// Текущие дата и время в виде "2026-11-04_12-30-05".
std::string timestamp() {
    std::time_t now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d_%H-%M-%S", std::localtime(&now));
    return buf;
}

}

namespace Backup {

std::string create(Database& db) {
    if (!db.isOpen()) return "";
    db.save();  // всё, что в буферах, должно попасть в файлы до копирования

    fs::path base = db.basePath();
    fs::path folder = base.parent_path() / "backups" /
                      (base.filename().string() + "_" + timestamp());

    fs::path unique = folder;
    for (int n = 2; fs::exists(unique); n++) {
        unique = folder.string() + "_" + std::to_string(n);
    }

    std::error_code error;
    fs::create_directories(unique, error);
    if (error) return "";

    for (const std::string& file : Database::filesOf(base.string())) {
        fs::copy_file(file, unique / fs::path(file).filename(), error);
        if (error) return "";
    }
    return unique.string();
}

bool restore(Database& db, const std::string& backupFolder) {
    if (!db.isOpen()) return false;
    return restoreTo(db, backupFolder, db.basePath());
}

bool restoreTo(Database& db, const std::string& backupFolder, const std::string& basePath) {
    std::error_code error;
    fs::path backupBase;
    for (const auto& entry : fs::directory_iterator(backupFolder, error)) {
        if (entry.path().extension() == ".dat") {
            backupBase = entry.path();
            backupBase.replace_extension();
            break;
        }
    }
    if (backupBase.empty()) return false;

    Database check;
    if (!check.open(backupBase.string())) return false;
    check.close();

    db.close();
    std::vector<std::string> from = Database::filesOf(backupBase.string());
    std::vector<std::string> to = Database::filesOf(basePath);
    bool ok = true;
    for (size_t i = 0; i < from.size(); i++) {
        fs::copy_file(from[i], to[i], fs::copy_options::overwrite_existing, error);
        if (error) ok = false;
    }
    return db.open(basePath) && ok;
}

}
