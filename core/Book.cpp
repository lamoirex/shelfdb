#include "Book.h"

#include <cstring>

void setText(char* field, int size, const std::string& text) {
    std::memset(field, 0, size);

    int length = (int)text.size();
    if (length > size) {
        length = size;
        while (length > 0 && ((unsigned char)text[length] & 0xC0) == 0x80) {
            length--;
        }
    }

    std::memcpy(field, text.data(), length);
}

std::string getText(const char* field, int size) {
    int length = 0;
    while (length < size && field[length] != '\0') {
        length++;
    }
    return std::string(field, length);
}

Book makeBook(int id, const std::string& title, const std::string& author,
              int year, double price, bool available) {
    Book book;
    book.id = id;
    setText(book.title, TITLE_LEN, title);
    setText(book.author, AUTHOR_LEN, author);
    book.year = year;
    book.price = price;
    book.available = available;
    return book;
}
