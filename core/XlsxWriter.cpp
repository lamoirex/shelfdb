#include "XlsxWriter.h"

#include <cstdint>
#include <cstdio>
#include <fstream>

namespace {

// ---------- CRC-32: контрольная сумма, которую требует формат zip ----------

uint32_t crc32(const std::string& data) {
    uint32_t crc = 0xFFFFFFFF;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1);
        }
    }
    return ~crc;
}

// Числа в zip записываются младшим байтом вперёд.
void put16(std::string& out, uint16_t v) {
    out += (char)(v & 0xFF);
    out += (char)(v >> 8);
}

void put32(std::string& out, uint32_t v) {
    for (int i = 0; i < 4; i++) out += (char)((v >> (8 * i)) & 0xFF);
}

struct ZipEntry {
    std::string name;
    std::string data;
};

// Собирает zip-архив без сжатия.
// Устройство: [заголовок файла + данные] ... [оглавление] [конец оглавления].
std::string buildZip(const std::vector<ZipEntry>& entries) {
    std::string zip;
    std::string directory;

    for (const ZipEntry& e : entries) {
        uint32_t offset = (uint32_t)zip.size();
        uint32_t crc = crc32(e.data);
        uint32_t size = (uint32_t)e.data.size();

        // Локальный заголовок файла.
        put32(zip, 0x04034b50);
        put16(zip, 20);          // нужна версия zip 2.0
        put16(zip, 0x0800);      // имена в UTF-8
        put16(zip, 0);           // без сжатия
        put16(zip, 0);           // время
        put16(zip, 0x21);        // дата: 1980-01-01
        put32(zip, crc);
        put32(zip, size);        // размер в архиве
        put32(zip, size);        // исходный размер
        put16(zip, (uint16_t)e.name.size());
        put16(zip, 0);
        zip += e.name;
        zip += e.data;

        // Запись в оглавлении о том же файле.
        put32(directory, 0x02014b50);
        put16(directory, 20);
        put16(directory, 20);
        put16(directory, 0x0800);
        put16(directory, 0);
        put16(directory, 0);
        put16(directory, 0x21);
        put32(directory, crc);
        put32(directory, size);
        put32(directory, size);
        put16(directory, (uint16_t)e.name.size());
        put16(directory, 0);
        put16(directory, 0);
        put16(directory, 0);
        put16(directory, 0);
        put32(directory, 0);
        put32(directory, offset);  // где в архиве начинается файл
        directory += e.name;
    }

    uint32_t directoryOffset = (uint32_t)zip.size();
    zip += directory;

    // Конец оглавления.
    put32(zip, 0x06054b50);
    put16(zip, 0);
    put16(zip, 0);
    put16(zip, (uint16_t)entries.size());
    put16(zip, (uint16_t)entries.size());
    put32(zip, (uint32_t)directory.size());
    put32(zip, directoryOffset);
    put16(zip, 0);
    return zip;
}

const char* CONTENT_TYPES =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
    "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
    "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
    "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
    "</Types>";

const char* ROOT_RELS =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
    "</Relationships>";

const char* WORKBOOK =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
    "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
    "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
    "<sheets><sheet name=\"Books\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
    "</workbook>";

const char* WORKBOOK_RELS =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
    "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>"
    "</Relationships>";

// Два стиля ячеек: 0 — обычный, 1 — жирный (для заголовка).
const char* STYLES =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
    "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
    "<fonts count=\"2\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font>"
    "<font><b/><sz val=\"11\"/><name val=\"Calibri\"/></font></fonts>"
    "<fills count=\"2\"><fill><patternFill patternType=\"none\"/></fill>"
    "<fill><patternFill patternType=\"gray125\"/></fill></fills>"
    "<borders count=\"1\"><border><left/><right/><top/><bottom/><diagonal/></border></borders>"
    "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
    "<cellXfs count=\"2\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/>"
    "<xf numFmtId=\"0\" fontId=\"1\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyFont=\"1\"/></cellXfs>"
    "<cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/></cellStyles>"
    "</styleSheet>";

}  // namespace

// ---------- ячейки ----------

std::string XlsxWriter::escapeXml(const std::string& text) {
    std::string out;
    for (char c : text) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default:  out += c;
        }
    }
    return out;
}

std::string XlsxWriter::columnName(int index) {
    std::string name;
    index++;
    while (index > 0) {
        int rest = (index - 1) % 26;
        name = (char)('A' + rest) + name;
        index = (index - 1) / 26;
    }
    return name;
}

void XlsxWriter::startRowIfNeeded() {
    if (rowOpen) return;
    rowNumber++;
    column = 0;
    rows += "<row r=\"" + std::to_string(rowNumber) + "\">";
    rowOpen = true;
}

void XlsxWriter::addHeader(const std::vector<std::string>& cells) {
    startRowIfNeeded();
    for (const std::string& text : cells) {
        std::string ref = columnName(column++) + std::to_string(rowNumber);
        rows += "<c r=\"" + ref + "\" t=\"inlineStr\" s=\"1\"><is><t>" + escapeXml(text) + "</t></is></c>";
    }
    endRow();
}

void XlsxWriter::addTextCell(const std::string& text) {
    startRowIfNeeded();
    std::string ref = columnName(column++) + std::to_string(rowNumber);
    rows += "<c r=\"" + ref + "\" t=\"inlineStr\"><is><t>" + escapeXml(text) + "</t></is></c>";
}

void XlsxWriter::addNumberCell(double number) {
    startRowIfNeeded();
    std::string ref = columnName(column++) + std::to_string(rowNumber);
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.15g", number);
    rows += "<c r=\"" + ref + "\"><v>" + buf + "</v></c>";
}

void XlsxWriter::endRow() {
    if (!rowOpen) return;
    rows += "</row>";
    rowOpen = false;
}

// ---------- сохранение ----------

bool XlsxWriter::save(const std::string& path) {
    endRow();

    std::string sheet =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<sheetData>" + rows + "</sheetData></worksheet>";

    std::string zip = buildZip({
        {"[Content_Types].xml", CONTENT_TYPES},
        {"_rels/.rels", ROOT_RELS},
        {"xl/workbook.xml", WORKBOOK},
        {"xl/_rels/workbook.xml.rels", WORKBOOK_RELS},
        {"xl/styles.xml", STYLES},
        {"xl/worksheets/sheet1.xml", sheet},
    });

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(zip.data(), (std::streamsize)zip.size());
    return (bool)out;
}
