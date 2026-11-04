#include "BookField.h"

#include <cstdlib>
#include <cerrno>
#include <climits>

std::string fieldName(Field field) {
    switch (field) {
        case Field::Id:        return "ID";
        case Field::Title:     return "Название";
        case Field::Author:    return "Автор";
        case Field::Year:      return "Год";
        case Field::Price:     return "Цена";
        case Field::Available: return "В наличии";
    }
    return "";
}

bool parseInt(const std::string& text, int& result) {
    if (text.empty()) return false;
    char* end = nullptr;
    errno = 0;
    long value = std::strtol(text.c_str(), &end, 10);
    // Строка должна быть числом целиком и влезать в int.
    if (*end != '\0' || errno == ERANGE || value < INT_MIN || value > INT_MAX) return false;
    result = (int)value;
    return true;
}

bool parseDouble(const std::string& text, double& result) {
    if (text.empty()) return false;
    std::string fixed = text;
    for (char& c : fixed) {
        if (c == ',') c = '.';  // разрешаем писать "19,99"
    }
    char* end = nullptr;
    errno = 0;
    double value = std::strtod(fixed.c_str(), &end);
    if (*end != '\0' || errno == ERANGE) return false;
    result = value;
    return true;
}

bool parseBool(const std::string& text, bool& result) {
    if (text == "true" || text == "1" || text == "да" || text == "Да") {
        result = true;
        return true;
    }
    if (text == "false" || text == "0" || text == "нет" || text == "Нет") {
        result = false;
        return true;
    }
    return false;
}

bool fieldEquals(const Book& book, Field field, const std::string& value) {
    int i;
    double d;
    bool b;
    switch (field) {
        case Field::Id:        return parseInt(value, i) && book.id == i;
        case Field::Title:     return getText(book.title, TITLE_LEN) == value;
        case Field::Author:    return getText(book.author, AUTHOR_LEN) == value;
        case Field::Year:      return parseInt(value, i) && book.year == i;
        case Field::Price:     return parseDouble(value, d) && book.price == d;
        case Field::Available: return parseBool(value, b) && book.available == b;
    }
    return false;
}
