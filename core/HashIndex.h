#pragma once

#include <cstdint>
#include <fstream>
#include <string>

class HashIndex {
public:
    bool create(const std::string& path, int keyLen, int64_t capacity = 1024);
    bool open(const std::string& path);
    void close();
    bool isOpen() const;

    int64_t find(const std::string& key);

    void put(const std::string& key, int64_t value);

    bool remove(const std::string& key);

    int64_t count() const { return header.used; }
    int64_t capacity() const { return header.capacity; }
    void flush();

private:
    // cостояния ячейки
    static const char EMPTY = 0;
    static const char USED = 1;
    static const char DELETED = 2;

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

    int64_t findCell(const std::string& key);

    void grow();

    std::string path;
    std::fstream file;
    IndexHeader header{};
};
