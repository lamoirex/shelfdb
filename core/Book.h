#pragma once

#include <cstdint>
#include <string>

const int TITLE_LEN = 64;
const int AUTHOR_LEN = 48;

#pragma pack(push, 1)
struct Book {
    int32_t id;                 // первичный ключ
    char title[TITLE_LEN];      // название
    char author[AUTHOR_LEN];    // автор (по нему будет вторичный индекс)
    int32_t year;               // год издания
    double price;               // цена
    bool available;             // есть ли в наличии
};
#pragma pack(pop)

static_assert(sizeof(Book) == 4 + TITLE_LEN + AUTHOR_LEN + 4 + 8 + 1,
              "В структуре Book не должно быть пустых байтов");

void setText(char* field, int size, const std::string& text);

std::string getText(const char* field, int size);

Book makeBook(int id, const std::string& title, const std::string& author,
              int year, double price, bool available);
