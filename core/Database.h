#pragma once

#include "Book.h"
#include "BookField.h"
#include "DataFile.h"
#include "HashIndex.h"

#include <functional>
#include <string>
#include <vector>

class Database {
public:
    bool create(const std::string& basePath);
    bool open(const std::string& basePath);
    void close();
    bool isOpen() const;

    void save();

    bool clear();

    bool destroy();

    std::string basePath() const { return path; }

    // Все файлы базы с таким путём: .dat, .pidx, .sidx.
    static std::vector<std::string> filesOf(const std::string& basePath);

    // Добавляет книгу. false, если книга с таким id уже есть. O(1).
    bool add(const Book& book);

    // Ищет книгу по id. O(1).
    bool findById(int id, Book& result);

    // Все книги автора. O(k), где k — сколько книг нашлось.
    std::vector<Book> findByAuthor(const std::string& author);

    // Поиск по любому полю:
    //   id и автор — через индексы, O(1) и O(k);
    //   остальные поля — проход по всему файлу, O(n).
    std::vector<Book> find(Field field, const std::string& value);

    // Удаляет книгу по id. false, если такой нет. O(1).
    bool removeById(int id);

    // Удаляет все книги автора. Возвращает, сколько удалено. O(k).
    int removeByAuthor(const std::string& author);

    // Удаление по любому полю, сложность как у find. Возвращает, сколько удалено.
    int remove(Field field, const std::string& value);

    // Заменяет книгу с id = oldId на newBook. Если id меняется,
    // новый тоже проверяется на уникальность. O(1).
    bool update(int oldId, const Book& newBook);

    // Проходит по всем книгам в файле (для показа таблицы в GUI). O(n).
    void forEach(const std::function<void(const Book&)>& visit);

    int64_t count() const;

private:
    // Удаляет книгу из слота: из цепочки автора, из индекса id и освобождает слот. O(1).
    void removeSlot(int64_t slot);

    // Ставит слот в начало цепочки своего автора. O(1).
    void linkToAuthor(int64_t slot, Slot& s);

    // Вынимает слот из цепочки автора, соединяя соседей друг с другом. O(1).
    void unlinkFromAuthor(const Slot& s);

    static std::string idKey(int id);
    static std::string authorKey(const Book& book);

    std::string path;
    DataFile data;
    HashIndex idIndex;
    HashIndex authorIndex;
};
