// Проверяем хеш-индекс: вставка, поиск, удаление, надгробия, расширение, повторное открытие.

#include "HashIndex.h"

#include <cstdio>
#include <iostream>
#include <string>

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cout << "ОШИБКА: " << what << std::endl;
        failures++;
    }
}

int main() {
    const char* path = "hashindex_test.pidx";
    HashIndex index;

    // 1. Пустой индекс ничего не находит.
    check(index.create(path, 12, 8), "создание индекса");
    check(index.find("1") == -1, "в пустом индексе ничего нет");

    // 2. Вставка и поиск.
    index.put("1", 100);
    index.put("2", 200);
    check(index.find("1") == 100 && index.find("2") == 200, "поиск после вставки");
    check(index.count() == 2, "счётчик ключей");

    // 3. Повторный put меняет значение, а не добавляет второй ключ.
    index.put("1", 111);
    check(index.find("1") == 111, "значение заменилось");
    check(index.count() == 2, "дубликат не добавился");

    // 4. Удаление.
    check(index.remove("1"), "удаление существующего ключа");
    check(!index.remove("1"), "повторное удаление возвращает false");
    check(index.find("1") == -1, "удалённый ключ не находится");
    check(index.find("2") == 200, "соседний ключ на месте");

    // 5. Надгробие не обрывает поиск.
    // В таблице из 8 ячеек коллизии почти неизбежны: часть ключей
    // окажется дальше своей ячейки, и удалённые соседи не должны их "спрятать".
    index.create(path, 12, 8);
    for (int i = 0; i < 5; i++) index.put(std::to_string(i), i);
    for (int i = 0; i < 5; i += 2) index.remove(std::to_string(i));
    for (int i = 1; i < 5; i += 2) {
        check(index.find(std::to_string(i)) == i, "ключ за надгробием находится");
    }

    // 6. Расширение: 10 000 ключей в таблицу, начавшуюся с 8 ячеек.
    index.create(path, 12, 8);
    for (int i = 0; i < 10000; i++) index.put(std::to_string(i), i * 10);
    check(index.count() == 10000, "все ключи на месте после расширения");
    check(index.capacity() >= 10000 / 0.7, "таблица выросла");
    bool allFound = true;
    for (int i = 0; i < 10000; i++) {
        if (index.find(std::to_string(i)) != i * 10) allFound = false;
    }
    check(allFound, "все ключи находятся после расширения");
    check(index.find("10000") == -1, "несуществующий ключ не находится");

    // 7. Удаляем половину и снова проверяем.
    for (int i = 0; i < 10000; i += 2) index.remove(std::to_string(i));
    bool halfOk = true;
    for (int i = 0; i < 10000; i++) {
        int64_t expected = (i % 2 == 0) ? -1 : i * 10;
        if (index.find(std::to_string(i)) != expected) halfOk = false;
    }
    check(halfOk, "после удаления половины всё корректно");
    index.close();

    // 8. После повторного открытия данные сохранились.
    check(index.open(path), "повторное открытие");
    check(index.count() == 5000, "счётчик сохранился");
    check(index.find("9999") == 99990, "ключ сохранился");

    // 9. Русские ключи (понадобятся для индекса по автору).
    index.create(path, 48, 8);
    index.put("Булгаков", 1);
    index.put("Толстой", 2);
    check(index.find("Булгаков") == 1 && index.find("Толстой") == 2, "русские ключи");
    index.close();

    std::remove(path);

    if (failures == 0) {
        std::cout << "hashindex_test: все проверки пройдены" << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
