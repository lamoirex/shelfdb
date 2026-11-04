#pragma once

#include "Database.h"

#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <functional>
#include <vector>

// Главное окно.
//
// Таблица на экране только показывает книги: после каждой операции она
// заново читается из файла базы. Все изменения идут через Database,
// то есть сразу в файлы.
class MainWindow : public QMainWindow {
public:
    MainWindow();
    ~MainWindow() override;

    // Открывает базу по пути к файлу .dat (используется при запуске с аргументом).
    bool openFile(const QString& datFile);

private:
    void buildMenu();
    void buildButtons();

    // ---- База ----
    void createDatabase();
    void openDatabase();
    void saveDatabase();
    void clearDatabase();
    void deleteDatabase();

    // ---- Записи ----
    void addBook();
    void editBook();
    void removeBooks();
    void searchBooks();
    void showAll();

    // ---- Сервис ----
    void makeBackup();
    void restoreBackup();
    void exportXlsx();
    void exportCsv();
    void generateBooks();

    // Перечитывает все книги из файла в таблицу.
    void reloadTable();
    void fillTable(const std::vector<Book>& books);
    void appendRow(const Book& book);

    // Выполняет действие и показывает в строке состояния, сколько оно заняло.
    void timed(const QString& name, const std::function<void()>& action);

    // Если база не открыта, показывает сообщение и возвращает false.
    bool requireOpen();

    // id книги в выделенной строке. false, если ничего не выделено.
    bool selectedId(int& id);

    void updateState();

    Database db;

    QTableWidget* table;
    QLabel* tableTitle;
    QLabel* pathLabel;
    QLabel* timeLabel;
    std::vector<QAction*> needOpenActions;   // действия, доступные только с открытой базой
    std::vector<QPushButton*> needOpenButtons;
};
