#pragma once

#include <string>
#include <vector>

class XlsxWriter {
public:
    void addHeader(const std::vector<std::string>& cells);
    void addTextCell(const std::string& text);
    void addNumberCell(double number);
    void endRow();

    bool save(const std::string& path);

private:
    void startRowIfNeeded();
    static std::string escapeXml(const std::string& text);
    static std::string columnName(int index);  // 0 -> A, 1 -> B, ..., 26 -> AA

    std::string rows;
    int rowNumber = 0;
    int column = 0;
    bool rowOpen = false;
};
