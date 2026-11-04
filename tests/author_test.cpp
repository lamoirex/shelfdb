// Проверяем индекс по автору, цепочки книг и поиск/удаление по любому полю.

#include "Database.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cout << "ОШИБКА: " << what << std::endl;
        failures++;
    }
}

// id всех книг из списка, по возрастанию — чтобы сравнивать без учёта порядка.
std::set<int> idsOf(const std::vector<Book>& books) {
    std::set<int> ids;
    for (const Book& b : books) ids.insert(b.id);
    return ids;
}

int main() {
    const std::string base = "author_test";
    Database db;
    db.create(base);

    db.add(makeBook(1, "Война и мир", "Толстой", 1869, 500.0, true));
    db.add(makeBook(2, "Анна Каренина", "Толстой", 1877, 450.0, false));
    db.add(makeBook(3, "Воскресение", "Толстой", 1899, 300.0, true));
    db.add(makeBook(4, "Мастер и Маргарита", "Булгаков", 1967, 500.0, true));
    db.add(makeBook(5, "Идиот", "Достоевский", 1869, 350.0, false));

    // 1. Поиск по автору.
    check(idsOf(db.findByAuthor("Толстой")) == std::set<int>{1, 2, 3}, "три книги Толстого");
    check(idsOf(db.findByAuthor("Булгаков")) == std::set<int>{4}, "одна книга Булгакова");
    check(db.findByAuthor("Пушкин").empty(), "нет книг Пушкина");

    // 2. Удаление из середины, начала и конца цепочки.
    // Цепочка Толстого: 3 <-> 2 <-> 1 (новые книги встают в начало).
    db.removeById(2);
    check(idsOf(db.findByAuthor("Толстой")) == std::set<int>{1, 3}, "удаление из середины цепочки");
    db.removeById(3);
    check(idsOf(db.findByAuthor("Толстой")) == std::set<int>{1}, "удаление начала цепочки");
    db.removeById(1);
    check(db.findByAuthor("Толстой").empty(), "удаление последней книги автора");

    // 3. Смена автора при редактировании.
    db.update(4, makeBook(4, "Мастер и Маргарита", "Достоевский", 1967, 500.0, true));
    check(db.findByAuthor("Булгаков").empty(), "книга ушла от старого автора");
    check(idsOf(db.findByAuthor("Достоевский")) == std::set<int>{4, 5}, "книга пришла к новому автору");

    // 4. Удаление всех книг автора.
    db.add(makeBook(6, "Бесы", "Достоевский", 1872, 400.0, true));
    check(db.removeByAuthor("Достоевский") == 3, "удалены три книги Достоевского");
    check(db.count() == 0, "база пуста");
    Book found;
    check(!db.findById(4, found) && !db.findById(5, found), "удалённые книги не находятся по id");
    check(db.add(makeBook(5, "x", "y", 1, 1.0, true)), "id удалённых книг снова свободны");

    // 5. Поиск и удаление по полям без индекса.
    db.create(base);
    db.add(makeBook(1, "A", "X", 1869, 10.5, true));
    db.add(makeBook(2, "B", "Y", 1869, 20.0, false));
    db.add(makeBook(3, "A", "Z", 1900, 10.5, true));
    check(idsOf(db.find(Field::Year, "1869")) == std::set<int>{1, 2}, "поиск по году");
    check(idsOf(db.find(Field::Price, "10,5")) == std::set<int>{1, 3}, "поиск по цене с запятой");
    check(idsOf(db.find(Field::Title, "A")) == std::set<int>{1, 3}, "поиск по названию");
    check(idsOf(db.find(Field::Available, "нет")) == std::set<int>{2}, "поиск по наличию");
    check(idsOf(db.find(Field::Id, "3")) == std::set<int>{3}, "поиск по id через find");
    check(db.find(Field::Year, "abc").empty(), "некорректное значение ничего не находит");
    check(db.remove(Field::Title, "A") == 2, "удаление по названию");
    check(db.count() == 1 && db.findByAuthor("X").empty(), "удаление по полю чистит индекс автора");

    // 6. forEach обходит все живые книги.
    int visited = 0;
    db.forEach([&](const Book&) { visited++; });
    check(visited == 1, "forEach");

    // 7. Стресс-тест: случайные операции, сверка с эталоном в памяти.
    // (Эталон std::map нужен только тесту, сама база в памяти ничего не держит.)
    db.create(base);
    std::map<int, std::string> expected;  // id -> автор
    std::mt19937 rng(42);
    const std::string authors[] = {"Толстой", "Булгаков", "Чехов", "Гоголь", "Пушкин"};
    bool stressOk = true;

    for (int step = 0; step < 20000 && stressOk; step++) {
        int op = rng() % 10;
        int id = rng() % 2000;
        std::string author = authors[rng() % 5];

        if (op < 5) {
            bool added = db.add(makeBook(id, "t", author, 2000, 1.0, true));
            if (added != (expected.count(id) == 0)) stressOk = false;
            if (added) expected[id] = author;
        } else if (op < 7) {
            bool removed = db.removeById(id);
            if (removed != (expected.count(id) == 1)) stressOk = false;
            expected.erase(id);
        } else if (op < 9) {
            int newId = (rng() % 3 == 0) ? (int)(rng() % 2000) : id;
            bool canUpdate = expected.count(id) == 1 && (newId == id || expected.count(newId) == 0);
            bool updated = db.update(id, makeBook(newId, "t", author, 2000, 1.0, true));
            if (updated != canUpdate) stressOk = false;
            if (updated) {
                expected.erase(id);
                expected[newId] = author;
            }
        } else if (rng() % 20 == 0) {
            int removed = db.removeByAuthor(author);
            int expectedRemoved = 0;
            for (auto it = expected.begin(); it != expected.end();) {
                if (it->second == author) {
                    it = expected.erase(it);
                    expectedRemoved++;
                } else {
                    ++it;
                }
            }
            if (removed != expectedRemoved) stressOk = false;
        }

        // Периодически сверяем все цепочки с эталоном.
        if (step % 1000 == 0) {
            for (const std::string& a : authors) {
                std::set<int> want;
                for (auto& [k, v] : expected) if (v == a) want.insert(k);
                std::vector<Book> got = db.findByAuthor(a);
                if (idsOf(got) != want || got.size() != want.size()) stressOk = false;
            }
            if (db.count() != (int64_t)expected.size()) stressOk = false;
        }
    }
    check(stressOk, "20 000 случайных операций совпадают с эталоном");
    db.close();

    // 8. После повторного открытия цепочки на месте.
    check(db.open(base), "повторное открытие");
    for (const std::string& a : authors) {
        std::set<int> want;
        for (auto& [k, v] : expected) if (v == a) want.insert(k);
        check(idsOf(db.findByAuthor(a)) == want, "цепочка " + a + " сохранилась");
    }
    db.close();

    std::remove((base + ".dat").c_str());
    std::remove((base + ".pidx").c_str());
    std::remove((base + ".sidx").c_str());

    if (failures == 0) {
        std::cout << "author_test: все проверки пройдены" << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
