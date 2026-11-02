// Проверяем, что книга корректно записывается в файл и читается обратно.

#include "Book.h"

#include <cstdio>
#include <fstream>
#include <iostream>

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cout << "ОШИБКА: " << what << std::endl;
        failures++;
    }
}

int main() {
    const char* path = "book_test.tmp";

    Book books[3] = {
        makeBook(1, "War and Peace", "Leo Tolstoy", 1869, 19.99, true),
        makeBook(2, "Мастер и Маргарита", "Булгаков", 1967, 500.0, false),
        makeBook(-7, "", "", -300, -0.5, false),
    };

    // 1. Записываем три книги подряд.
    std::ofstream out(path, std::ios::binary);
    for (const Book& b : books) {
        out.write((const char*)&b, sizeof(Book));
    }
    out.close();

    // 2. Читаем вторую книгу напрямую по смещению 1 * sizeof(Book).
    std::ifstream in(path, std::ios::binary);
    Book second;
    in.seekg(1 * sizeof(Book));
    in.read((char*)&second, sizeof(Book));
    check(second.id == 2, "id второй книги");
    check(getText(second.title, TITLE_LEN) == "Мастер и Маргарита", "русское название");
    check(getText(second.author, AUTHOR_LEN) == "Булгаков", "русский автор");
    check(second.year == 1967 && second.price == 500.0 && !second.available, "числа и bool");

    // 3. Третья книга: отрицательные числа и пустые строки.
    Book third;
    in.read((char*)&third, sizeof(Book));
    check(third.id == -7 && third.year == -300 && third.price == -0.5, "отрицательные числа");
    check(getText(third.title, TITLE_LEN).empty(), "пустая строка");
    in.close();
    std::remove(path);

    // 4. Длинная русская строка обрезается и буква не ломается.
    std::string longText;
    for (int i = 0; i < 40; i++) longText += "Ж";  // 80 байт
    Book longBook = makeBook(3, longText, longText, 2000, 1.0, true);
    check(getText(longBook.title, TITLE_LEN) == longText.substr(0, 64), "обрезка title");
    check(getText(longBook.author, AUTHOR_LEN) == longText.substr(0, 48), "обрезка author");

    // 5. Нечётный размер поля: обрезка отступает к началу буквы.
    char small[5];
    setText(small, 5, "ЖЖЖ");  // 6 байт, влезает только 2 буквы (4 байта)
    check(getText(small, 5) == "ЖЖ", "обрезка по границе буквы");

    if (failures == 0) {
        std::cout << "book_test: все проверки пройдены" << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
