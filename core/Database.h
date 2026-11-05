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

    static std::vector<std::string> filesOf(const std::string& basePath);

    bool add(const Book& book);

    bool findById(int id, Book& result);

    std::vector<Book> findByAuthor(const std::string& author);

    std::vector<Book> find(Field field, const std::string& value);

    bool removeById(int id);

    int removeByAuthor(const std::string& author);

    int remove(Field field, const std::string& value);

    bool update(int oldId, const Book& newBook);

    void forEach(const std::function<void(const Book&)>& visit);

    int64_t count() const;

private:
    void removeSlot(int64_t slot);

    void linkToAuthor(int64_t slot, Slot& s);

    void unlinkFromAuthor(const Slot& s);

    static std::string idKey(int id);
    static std::string authorKey(const Book& book);

    std::string path;
    DataFile data;
    HashIndex idIndex;
    HashIndex authorIndex;
};
