#pragma once

#include <cstdint>
#include <fstream>
#include <string>

// Хеш-таблица, которая целиком живёт в файле: ключ (строка) -> число.
//
// Используется дважды:
//   books.pidx: id книги       -> номер слота с этой книгой
//   books.sidx: автор (шаг 5)  -> номер первого слота в цепочке книг автора
//
// Устройство файла:
//   [IndexHeader][ячейка 0][ячейка 1]...[ячейка capacity-1]
// Ячейка: [состояние: 1 байт][ключ: keyLen байт][значение: 8 байт]
//
// Ячейка для ключа ищется так: номер = hash(ключ) % capacity.
// Если ячейка занята другим ключом (коллизия), смотрим следующую,
// и так далее по кругу (линейное пробирование).
//
// Сложность: поиск, вставка и удаление — O(1) в среднем,
// пока таблица заполнена не больше чем на 70%. Когда заполнение
// превышает 70%, таблица перестраивается в файл вдвое больше:
// это O(n), но случается редко, поэтому в среднем вставка остаётся O(1).
class HashIndex {
public:
    // Создаёт пустой индекс. keyLen — максимальная длина ключа в байтах.
    bool create(const std::string& path, int keyLen, int64_t capacity = 1024);
    bool open(const std::string& path);
    void close();
    bool isOpen() const;

    // Значение по ключу или -1, если ключа нет.
    int64_t find(const std::string& key);

    // Добавляет ключ или меняет значение у существующего.
    void put(const std::string& key, int64_t value);

    // Удаляет ключ. false, если его не было.
    bool remove(const std::string& key);

    int64_t count() const { return header.used; }
    int64_t capacity() const { return header.capacity; }
    void flush();

private:
    // Состояния ячейки.
    static const char EMPTY = 0;    // пустая, никогда не занималась
    static const char USED = 1;     // в ней лежит ключ
    static const char DELETED = 2;  // ключ удалён ("надгробие", tombstone)

#pragma pack(push, 1)
    struct IndexHeader {
        char magic[4];      // подпись "SHIX"
        int32_t keyLen;     // длина ключа в байтах
        int64_t capacity;   // число ячеек
        int64_t used;       // занятых ячеек
        int64_t deleted;    // ячеек-надгробий
    };
#pragma pack(pop)

    struct Cell {
        char state = EMPTY;
        std::string key;
        int64_t value = -1;
    };

    static uint64_t hashOf(const std::string& key);
    std::string fitKey(const std::string& key) const;

    Cell readCell(int64_t index);
    void writeCell(int64_t index, const Cell& cell);
    void writeHeader();
    int64_t cellSize() const;

    // Номер ячейки с этим ключом или -1.
    int64_t findCell(const std::string& key);

    // Перестраивает таблицу в файл с вдвое большим числом ячеек.
    void grow();

    std::string path;
    std::fstream file;
    IndexHeader header{};
};
