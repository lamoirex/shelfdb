#pragma once

#include <string>
#include <vector>

// Простейший писатель файлов Excel (.xlsx) без сторонних библиотек.
//
// Файл .xlsx — это zip-архив, в котором лежат несколько XML-файлов:
//   [Content_Types].xml        — какие типы файлов внутри
//   _rels/.rels                — где главный файл книги
//   xl/workbook.xml            — список листов
//   xl/_rels/workbook.xml.rels — где лежит каждый лист
//   xl/worksheets/sheet1.xml   — сами данные: строки и ячейки
//
// Архив пишется без сжатия (метод "stored"): Excel такие открывает,
// а код получается коротким — нужна только контрольная сумма CRC-32.
class XlsxWriter {
public:
    // Ячейки первой строки будут жирными (заголовок).
    void addHeader(const std::vector<std::string>& cells);
    void addTextCell(const std::string& text);
    void addNumberCell(double number);
    void endRow();

    bool save(const std::string& path);

private:
    void startRowIfNeeded();
    static std::string escapeXml(const std::string& text);
    static std::string columnName(int index);  // 0 -> A, 1 -> B, ..., 26 -> AA

    std::string rows;   // XML строк листа
    int rowNumber = 0;
    int column = 0;
    bool rowOpen = false;
};
