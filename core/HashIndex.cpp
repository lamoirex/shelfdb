#include "HashIndex.h"

#include "Book.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {
const char MAGIC[4] = {'S', 'H', 'I', 'X'};
}


bool HashIndex::create(const std::string& filePath, int keyLen, int64_t capacity) {
    close();
    path = filePath;

    file.open(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return false;

    std::memcpy(header.magic, MAGIC, 4);
    header.keyLen = keyLen;
    header.capacity = capacity;
    header.used = 0;
    header.deleted = 0;
    writeHeader();

    std::vector<char> zeros(cellSize() * 1024, 0);
    int64_t left = capacity;
    while (left > 0) {
        int64_t n = left < 1024 ? left : 1024;
        file.write(zeros.data(), n * cellSize());
        left -= n;
    }
    return true;
}

bool HashIndex::open(const std::string& filePath) {
    close();
    path = filePath;

    file.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0);
    file.read((char*)&header, sizeof(IndexHeader));
    if (!file || std::memcmp(header.magic, MAGIC, 4) != 0) {
        close();
        return false;
    }
    return true;
}

void HashIndex::close() {
    if (file.is_open()) {
        file.flush();
        file.close();
    }
    file.clear();
}

bool HashIndex::isOpen() const {
    return file.is_open();
}

void HashIndex::flush() {
    file.flush();
}

int64_t HashIndex::cellSize() const {
    return 1 + header.keyLen + 8;
}

HashIndex::Cell HashIndex::readCell(int64_t index) {
    std::vector<char> buf(cellSize(), 0);
    file.seekg(sizeof(IndexHeader) + index * cellSize());
    file.read(buf.data(), cellSize());
    file.clear();

    Cell cell;
    cell.state = buf[0];
    cell.key = getText(buf.data() + 1, header.keyLen);
    std::memcpy(&cell.value, buf.data() + 1 + header.keyLen, 8);
    return cell;
}

void HashIndex::writeCell(int64_t index, const Cell& cell) {
    std::vector<char> buf(cellSize(), 0);
    buf[0] = cell.state;
    setText(buf.data() + 1, header.keyLen, cell.key);
    std::memcpy(buf.data() + 1 + header.keyLen, &cell.value, 8);

    file.seekp(sizeof(IndexHeader) + index * cellSize());
    file.write(buf.data(), cellSize());
}

void HashIndex::writeHeader() {
    file.seekp(0);
    file.write((const char*)&header, sizeof(IndexHeader));
}

// FNV-1a: простая и хорошо перемешивающая хеш-функция.
// Для каждого байта: hash ^= байт; hash *= простое число.
uint64_t HashIndex::hashOf(const std::string& key) {
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : key) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string HashIndex::fitKey(const std::string& key) const {
    char buf[256];
    setText(buf, header.keyLen, key);
    return getText(buf, header.keyLen);
}

int64_t HashIndex::findCell(const std::string& key) {
    int64_t i = hashOf(key) % header.capacity;

    for (int64_t step = 0; step < header.capacity; step++) {
        Cell cell = readCell(i);
        if (cell.state == EMPTY) return -1;
        if (cell.state == USED && cell.key == key) return i;
        i = (i + 1) % header.capacity;
    }
    return -1;
}

int64_t HashIndex::find(const std::string& rawKey) {
    std::string key = fitKey(rawKey);
    int64_t i = findCell(key);
    if (i == -1) return -1;
    return readCell(i).value;
}

void HashIndex::put(const std::string& rawKey, int64_t value) {
    std::string key = fitKey(rawKey);

    int64_t existing = findCell(key);
    if (existing != -1) {
        Cell cell = readCell(existing);
        cell.value = value;
        writeCell(existing, cell);
        return;
    }

    int64_t i = hashOf(key) % header.capacity;
    Cell cell = readCell(i);
    while (cell.state == USED) {
        i = (i + 1) % header.capacity;
        cell = readCell(i);
    }

    if (cell.state == DELETED) header.deleted--;
    header.used++;
    writeCell(i, Cell{USED, key, value});
    writeHeader();

    if ((header.used + header.deleted) * 10 > header.capacity * 7) {
        grow();
    }
}

bool HashIndex::remove(const std::string& rawKey) {
    std::string key = fitKey(rawKey);
    int64_t i = findCell(key);
    if (i == -1) return false;

    writeCell(i, Cell{DELETED, "", -1});
    header.used--;
    header.deleted++;
    writeHeader();
    return true;
}

void HashIndex::grow() {
    std::string tmpPath = path + ".tmp";

    HashIndex bigger;
    bigger.create(tmpPath, header.keyLen, header.capacity * 2);

    for (int64_t i = 0; i < header.capacity; i++) {
        Cell cell = readCell(i);
        if (cell.state == USED) {
            bigger.put(cell.key, cell.value);
        }
    }
    bigger.close();
    close();

    std::remove(path.c_str());
    std::rename(tmpPath.c_str(), path.c_str());
    open(path);
}
