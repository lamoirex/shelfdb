#pragma once

#include "Book.h"

#include <string>

// Поля книги, по которым можно искать и удалять.
enum class Field { Id, Title, Author, Year, Price, Available };

const Field ALL_FIELDS[] = {Field::Id, Field::Title, Field::Author,
                            Field::Year, Field::Price, Field::Available};

std::string fieldName(Field field);

bool parseInt(const std::string& text, int& result);
bool parseDouble(const std::string& text, double& result);
bool parseBool(const std::string& text, bool& result);

// Равно ли поле книги значению, записанному строкой.
bool fieldEquals(const Book& book, Field field, const std::string& value);
