#pragma once

#include "Book.h"

#include <string>

// Поля книги, по которым можно искать и удалять.
enum class Field { Id, Title, Author, Year, Price, Available };

// Все поля по порядку — удобно для выпадающих списков в GUI.
const Field ALL_FIELDS[] = {Field::Id, Field::Title, Field::Author,
                            Field::Year, Field::Price, Field::Available};

// Название поля для показа пользователю.
std::string fieldName(Field field);

// Перевод строки, которую ввёл пользователь, в значение.
// Возвращают false, если строка некорректна ("abc" вместо числа и т. п.).
bool parseInt(const std::string& text, int& result);
bool parseDouble(const std::string& text, double& result);
bool parseBool(const std::string& text, bool& result);  // true/false, 1/0, да/нет

// Равно ли поле книги значению, записанному строкой.
bool fieldEquals(const Book& book, Field field, const std::string& value);
