#include "Database.h"

namespace {
const int ID_KEY_LEN = 12;  // "-2147483648" — самый длинный int, 11 символов
}

std::string Database::idKey(int id) {
    return std::to_string(id);
}

// ---------- открытие и закрытие ----------

bool Database::create(const std::string& basePath) {
    close();
    if (!data.create(basePath + ".dat") ||
        !idIndex.create(basePath + ".pidx", ID_KEY_LEN)) {
        close();
        return false;
    }
    return true;
}

bool Database::open(const std::string& basePath) {
    close();
    if (!data.open(basePath + ".dat") ||
        !idIndex.open(basePath + ".pidx")) {
        close();
        return false;
    }
    return true;
}

void Database::close() {
    data.close();
    idIndex.close();
}

bool Database::isOpen() const {
    return data.isOpen();
}

int64_t Database::count() const {
    return data.liveCount();
}

// ---------- операции ----------

bool Database::add(const Book& book) {
    // 1. Проверка уникальности ключа через индекс — O(1).
    if (idIndex.find(idKey(book.id)) != NO_SLOT) return false;

    // 2. Берём слот: свободный или новый в конце файла — O(1).
    int64_t slot = data.allocateSlot();

    // 3. Записываем книгу в слот — O(1).
    Slot s{};
    s.used = true;
    s.book = book;
    s.prevSameAuthor = NO_SLOT;
    s.nextSameAuthor = NO_SLOT;
    s.nextFree = NO_SLOT;
    data.writeSlot(slot, s);

    // 4. Запоминаем в индексе, где лежит книга — O(1).
    idIndex.put(idKey(book.id), slot);
    return true;
}

bool Database::findById(int id, Book& result) {
    int64_t slot = idIndex.find(idKey(id));
    if (slot == NO_SLOT) return false;

    result = data.readSlot(slot).book;
    return true;
}

bool Database::removeById(int id) {
    int64_t slot = idIndex.find(idKey(id));
    if (slot == NO_SLOT) return false;

    removeSlot(slot);
    return true;
}

void Database::removeSlot(int64_t slot) {
    Slot s = data.readSlot(slot);
    idIndex.remove(idKey(s.book.id));
    data.freeSlot(slot);
}

bool Database::update(int oldId, const Book& newBook) {
    int64_t slot = idIndex.find(idKey(oldId));
    if (slot == NO_SLOT) return false;

    bool idChanged = (newBook.id != oldId);
    if (idChanged && idIndex.find(idKey(newBook.id)) != NO_SLOT) {
        return false;  // новый id уже занят другой книгой
    }

    // Книга остаётся в том же слоте, меняется только содержимое.
    Slot s = data.readSlot(slot);
    s.book = newBook;
    data.writeSlot(slot, s);

    if (idChanged) {
        idIndex.remove(idKey(oldId));
        idIndex.put(idKey(newBook.id), slot);
    }
    return true;
}
