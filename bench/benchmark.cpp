// Замеры времени операций для отчёта.
//
// Для каждого размера базы N (1 000, 10 000, 100 000, 1 000 000):
//   1. создаём пустую базу и добавляем N книг;
//   2. замеряем среднее время каждой операции на случайных данных;
//   3. печатаем таблицу и сохраняем её в benchmark_results.csv.
//
// Чтобы цепочки авторов были одной длины при любом N, у каждого автора ~10 книг:
// авторов N / 10. Тогда видно, что поиск по автору зависит от k (числа найденных),
// а не от размера базы.
//
// Запуск: benchmark [максимальный N], например "benchmark 100000".
// Собирать в режиме Release, иначе цифры будут в разы хуже.

#include "Database.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

const int BOOKS_PER_AUTHOR = 10;

using Clock = std::chrono::steady_clock;

// Среднее время одного вызова action в микросекундах.
template <typename Action>
double averageMicros(int repeats, Action action) {
    auto start = Clock::now();
    for (int i = 0; i < repeats; i++) action(i);
    auto end = Clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count() / repeats;
}

std::string authorName(int n) {
    return "Автор " + std::to_string(n);
}

Book randomBook(int id, int authors, std::mt19937& rng) {
    return makeBook(id, "Книга " + std::to_string(id), authorName(rng() % authors),
                    1800 + (int)(rng() % 225), (rng() % 100000) / 100.0, rng() % 2);
}

struct Row {
    int n;
    double insert, findId, findAuthor, findYear, removeId, removeAuthor, removeYear;
    double fileMb;
};

Row measure(int n, const std::string& base) {
    std::mt19937 rng(12345);
    int authors = std::max(1, n / BOOKS_PER_AUTHOR);
    Row r{};
    r.n = n;

    Database db;
    db.create(base);

    // Добавление: N книг со случайным порядком id.
    std::vector<int> ids(n);
    for (int i = 0; i < n; i++) ids[i] = i;
    std::shuffle(ids.begin(), ids.end(), rng);
    r.insert = averageMicros(n, [&](int i) { db.add(randomBook(ids[i], authors, rng)); });

    // Поиск по id: 10 000 случайных существующих id.
    const int lookups = 10000;
    Book found;
    r.findId = averageMicros(lookups, [&](int) { db.findById(rng() % n, found); });

    // Поиск по автору: возвращает ~10 книг.
    r.findAuthor = averageMicros(1000, [&](int) { db.findByAuthor(authorName(rng() % authors)); });

    // Поиск по году — индекса нет, проход по файлу. На больших N повторов меньше.
    int scans = n >= 1000000 ? 3 : n >= 100000 ? 10 : 50;
    r.findYear = averageMicros(scans, [&](int) { db.find(Field::Year, std::to_string(1800 + rng() % 225)); });

    // Удаление по id: 1 000 разных книг.
    std::shuffle(ids.begin(), ids.end(), rng);
    int removals = std::min(1000, n / 4);
    r.removeId = averageMicros(removals, [&](int i) { db.removeById(ids[i]); });

    // Удаление всех книг автора (~10 штук).
    int authorRemovals = std::min(100, authors / 4);
    r.removeAuthor = averageMicros(authorRemovals, [&](int i) { db.removeByAuthor(authorName(i)); });

    // Удаление по году — проход по файлу.
    r.removeYear = averageMicros(3, [&](int i) { db.remove(Field::Year, std::to_string(1800 + i)); });

    db.close();

    r.fileMb = 0;
    for (const std::string& f : Database::filesOf(base)) {
        r.fileMb += std::filesystem::file_size(f) / (1024.0 * 1024.0);
        std::remove(f.c_str());
    }
    return r;
}

}  // namespace

int main(int argc, char* argv[]) {
    int maxN = argc > 1 ? std::atoi(argv[1]) : 1000000;
    std::vector<int> sizes;
    for (int n = 1000; n <= maxN; n *= 10) sizes.push_back(n);

    std::ofstream csv("benchmark_results.csv");
    csv << "N;insert_us;find_id_us;find_author_us;find_year_us;remove_id_us;remove_author_us;remove_year_us;files_mb\n";

    std::printf("Среднее время одной операции, мкс\n\n");
    std::printf("| %9s | %10s | %10s | %10s | %12s | %10s | %10s | %12s | %8s |\n",
                "N", "добавл.", "поиск id", "поиск авт.", "поиск год", "удал. id", "удал. авт.", "удал. год", "файлы МБ");
    std::printf("|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n");

    for (int n : sizes) {
        Row r = measure(n, "benchmark_db");
        std::printf("| %9d | %10.2f | %10.2f | %10.2f | %12.1f | %10.2f | %10.2f | %12.1f | %8.1f |\n",
                    r.n, r.insert, r.findId, r.findAuthor, r.findYear, r.removeId, r.removeAuthor, r.removeYear, r.fileMb);
        std::fflush(stdout);
        csv << r.n << ';' << r.insert << ';' << r.findId << ';' << r.findAuthor << ';' << r.findYear << ';'
            << r.removeId << ';' << r.removeAuthor << ';' << r.removeYear << ';' << r.fileMb << '\n';
    }

    std::printf("\nПоиск и удаление по id и автору — через хеш-индексы, остальное — проход по файлу.\n");
    std::printf("Результаты сохранены в benchmark_results.csv\n");
    return 0;
}
