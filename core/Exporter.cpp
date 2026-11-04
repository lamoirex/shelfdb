#include "Exporter.h"

#include "BookField.h"
#include "XlsxWriter.h"

#include <cstdio>
#include <fstream>

namespace {

std::vector<std::string> headerNames() {
    std::vector<std::string> names;
    for (Field f : ALL_FIELDS) names.push_back(fieldName(f));
    return names;
}

// Значение в CSV берётся в кавычки, кавычки внутри удваиваются.
std::string csvQuote(const std::string& text) {
    std::string out = "\"";
    for (char c : text) {
        if (c == '"') out += '"';
        out += c;
    }
    return out + "\"";
}

std::string formatPrice(double price) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.2f", price);
    return buf;
}

}  // namespace

namespace Exporter {

bool toCsv(Database& db, const std::string& path) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    out << "\xEF\xBB\xBF";  // метка UTF-8, чтобы Excel не испортил русские буквы

    std::vector<std::string> names = headerNames();
    for (size_t i = 0; i < names.size(); i++) {
        out << (i ? ";" : "") << csvQuote(names[i]);
    }
    out << "\r\n";

    db.forEach([&](const Book& b) {
        out << b.id << ';'
            << csvQuote(getText(b.title, TITLE_LEN)) << ';'
            << csvQuote(getText(b.author, AUTHOR_LEN)) << ';'
            << b.year << ';'
            << formatPrice(b.price) << ';'
            << (b.available ? "да" : "нет") << "\r\n";
    });
    return (bool)out;
}

bool toXlsx(Database& db, const std::string& path) {
    XlsxWriter xlsx;
    xlsx.addHeader(headerNames());

    db.forEach([&](const Book& b) {
        xlsx.addNumberCell(b.id);
        xlsx.addTextCell(getText(b.title, TITLE_LEN));
        xlsx.addTextCell(getText(b.author, AUTHOR_LEN));
        xlsx.addNumberCell(b.year);
        xlsx.addNumberCell(b.price);
        xlsx.addTextCell(b.available ? "да" : "нет");
        xlsx.endRow();
    });
    return xlsx.save(path);
}

}  // namespace Exporter
