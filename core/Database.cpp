#include "Database.h"

#include <cstdio>

namespace {
const int ID_KEY_LEN = 12;  // "-2147483648" — самый длинный int, 11 символов
}

std::string Database::idKey(int id) {
    return std::to_string(id);
}

std::string Database::authorKey(const Book& book) {
    return getText(book.author, AUTHOR_LEN);
}

// ---------- открытие и закрытие ----------

bool Database::create(const std::string& basePath) {
    close();
    if (!data.create(basePath + ".dat") ||
        !idIndex.create(basePath + ".pidx", ID_KEY_LEN) ||
        !authorIndex.create(basePath + ".sidx", AUTHOR_LEN)) {
        close();
        return false;
    }
    path = basePath;
    return true;
}

bool Database::open(const std::string& basePath) {
    close();
    if (!data.open(basePath + ".dat") ||
        !idIndex.open(basePath + ".pidx") ||
        !authorIndex.open(basePath + ".sidx")) {
        close();
        return false;
    }
    path = basePath;
    return true;
}

void Database::close() {
    data.close();
    idIndex.close();
    authorIndex.close();
    path.clear();
}

void Database::save() {
    data.flush();
    idIndex.flush();
    authorIndex.flush();
}

bool Database::clear() {
    if (!isOpen()) return false;
    std::string current = path;
    return create(current);
}

bool Database::destroy() {
    if (!isOpen()) return false;
    std::string current = path;
    close();

    bool ok = true;
    for (const std::string& file : filesOf(current)) {
        if (std::remove(file.c_str()) != 0) ok = false;
    }
    return ok;
}

std::vector<std::string> Database::filesOf(const std::string& basePath) {
    return {basePath + ".dat", basePath + ".pidx", basePath + ".sidx"};
}

bool Database::isOpen() const {
    return data.isOpen();
}

int64_t Database::count() const {
    return data.liveCount();
}

// ---------- цепочки книг одного автора ----------

void Database::linkToAuthor(int64_t slot, Slot& s) {
    // Текущая первая книга автора (или NO_SLOT, если книг автора ещё нет).
    int64_t head = authorIndex.find(authorKey(s.book));

    // Новая книга встаёт перед ней.
    s.prevSameAuthor = NO_SLOT;
    s.nextSameAuthor = head;

    if (head != NO_SLOT) {
        Slot headSlot = data.readSlot(head);
        headSlot.prevSameAuthor = slot;
        data.writeSlot(head, headSlot);
    }

    // Теперь цепочка автора начинается с новой книги.
    authorIndex.put(authorKey(s.book), slot);
}

void Database::unlinkFromAuthor(const Slot& s) {
    // Было:  prev <-> slot <-> next
    // Стало: prev <-> next

    if (s.prevSameAuthor != NO_SLOT) {
        Slot prev = data.readSlot(s.prevSameAuthor);
        prev.nextSameAuthor = s.nextSameAuthor;
        data.writeSlot(s.prevSameAuthor, prev);
    } else if (s.nextSameAuthor != NO_SLOT) {
        // Удаляем первую книгу цепочки — началом становится следующая.
        authorIndex.put(authorKey(s.book), s.nextSameAuthor);
    } else {
        // Это была единственная книга автора.
        authorIndex.remove(authorKey(s.book));
    }

    if (s.nextSameAuthor != NO_SLOT) {
        Slot next = data.readSlot(s.nextSameAuthor);
        next.prevSameAuthor = s.prevSameAuthor;
        data.writeSlot(s.nextSameAuthor, next);
    }
}

// ---------- добавление ----------

bool Database::add(const Book& book) {
    // 1. Проверка уникальности ключа через индекс — O(1).
    if (idIndex.find(idKey(book.id)) != NO_SLOT) return false;

    // 2. Берём слот: свободный или новый в конце файла — O(1).
    int64_t slot = data.allocateSlot();

    // 3. Ставим книгу в начало цепочки её автора и записываем в слот — O(1).
    Slot s{};
    s.used = true;
    s.book = book;
    s.nextFree = NO_SLOT;
    linkToAuthor(slot, s);
    data.writeSlot(slot, s);

    // 4. Запоминаем в индексе, где лежит книга — O(1).
    idIndex.put(idKey(book.id), slot);
    return true;
}

// ---------- поиск ----------

bool Database::findById(int id, Book& result) {
    int64_t slot = idIndex.find(idKey(id));
    if (slot == NO_SLOT) return false;

    result = data.readSlot(slot).book;
    return true;
}

std::vector<Book> Database::findByAuthor(const std::string& author) {
    std::vector<Book> result;

    // Индекс даёт начало цепочки, дальше идём по ссылкам nextSameAuthor.
    int64_t slot = authorIndex.find(author);
    while (slot != NO_SLOT) {
        Slot s = data.readSlot(slot);
        result.push_back(s.book);
        slot = s.nextSameAuthor;
    }
    return result;
}

std::vector<Book> Database::find(Field field, const std::string& value) {
    if (field == Field::Id) {
        std::vector<Book> result;
        int id;
        Book book;
        if (parseInt(value, id) && findById(id, book)) result.push_back(book);
        return result;
    }

    if (field == Field::Author) {
        return findByAuthor(value);
    }

    // По остальным полям индекса нет — проверяем каждый слот файла.
    std::vector<Book> result;
    for (int64_t i = 0; i < data.slotCount(); i++) {
        Slot s = data.readSlot(i);
        if (s.used && fieldEquals(s.book, field, value)) result.push_back(s.book);
    }
    return result;
}

void Database::forEach(const std::function<void(const Book&)>& visit) {
    for (int64_t i = 0; i < data.slotCount(); i++) {
        Slot s = data.readSlot(i);
        if (s.used) visit(s.book);
    }
}

// ---------- удаление ----------

void Database::removeSlot(int64_t slot) {
    Slot s = data.readSlot(slot);
    unlinkFromAuthor(s);
    idIndex.remove(idKey(s.book.id));
    data.freeSlot(slot);
}

bool Database::removeById(int id) {
    int64_t slot = idIndex.find(idKey(id));
    if (slot == NO_SLOT) return false;

    removeSlot(slot);
    return true;
}

int Database::removeByAuthor(const std::string& author) {
    int removed = 0;

    // Удаляется вся цепочка, поэтому соседей перевязывать не нужно:
    // идём по цепочке, освобождаем слоты и в конце убираем автора из индекса.
    int64_t slot = authorIndex.find(author);
    while (slot != NO_SLOT) {
        Slot s = data.readSlot(slot);
        int64_t next = s.nextSameAuthor;

        idIndex.remove(idKey(s.book.id));
        data.freeSlot(slot);
        removed++;

        slot = next;
    }

    authorIndex.remove(author);
    return removed;
}

int Database::remove(Field field, const std::string& value) {
    if (field == Field::Id) {
        int id;
        return parseInt(value, id) && removeById(id) ? 1 : 0;
    }

    if (field == Field::Author) {
        return removeByAuthor(value);
    }

    // Проход по файлу. Удаление не сдвигает другие слоты,
    // поэтому можно удалять прямо во время прохода.
    int removed = 0;
    for (int64_t i = 0; i < data.slotCount(); i++) {
        Slot s = data.readSlot(i);
        if (s.used && fieldEquals(s.book, field, value)) {
            removeSlot(i);
            removed++;
        }
    }
    return removed;
}

// ---------- редактирование ----------

bool Database::update(int oldId, const Book& newBook) {
    int64_t slot = idIndex.find(idKey(oldId));
    if (slot == NO_SLOT) return false;

    bool idChanged = (newBook.id != oldId);
    if (idChanged && idIndex.find(idKey(newBook.id)) != NO_SLOT) {
        return false;  // новый id уже занят другой книгой
    }

    // Книга остаётся в том же слоте, меняется только содержимое.
    Slot s = data.readSlot(slot);
    bool authorChanged = authorKey(s.book) != authorKey(newBook);

    if (authorChanged) {
        // Переносим книгу из цепочки старого автора в цепочку нового.
        unlinkFromAuthor(s);
        s.book = newBook;
        linkToAuthor(slot, s);
    } else {
        s.book = newBook;
    }
    data.writeSlot(slot, s);

    if (idChanged) {
        idIndex.remove(idKey(oldId));
        idIndex.put(idKey(newBook.id), slot);
    }
    return true;
}
