#pragma once

#include "Book.h"
#include "DataFile.h"
#include "HashIndex.h"

#include <string>
#include <vector>

// База данных книг. Через этот класс GUI выполняет все операции.
//
// База — это два файла с общим именем:
//   books.dat   — сами книги (DataFile)
//   books.pidx  — первичный индекс: id -> номер слота (HashIndex)
//
// В памяти книги не хранятся: каждая операция читает и пишет файлы напрямую.
class Database {
public:
    // basePath — путь без расширения, например "/Users/me/books".
    bool create(const std::string& basePath);
    bool open(const std::string& basePath);
    void close();
    bool isOpen() const;

    // Добавляет книгу. false, если книга с таким id уже есть. O(1).
    bool add(const Book& book);

    // Ищет книгу по id. O(1).
    bool findById(int id, Book& result);

    // Удаляет книгу по id. false, если такой нет. O(1).
    bool removeById(int id);

    // Заменяет книгу с id = oldId на newBook. Если id меняется,
    // новый тоже проверяется на уникальность. O(1).
    bool update(int oldId, const Book& newBook);

    int64_t count() const;

private:
    // Удаляет книгу из слота: освобождает слот и убирает id из индекса.
    void removeSlot(int64_t slot);

    static std::string idKey(int id);

    DataFile data;
    HashIndex idIndex;
};
