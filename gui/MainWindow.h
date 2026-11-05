#pragma once

#include "Database.h"

#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <functional>
#include <vector>

class MainWindow : public QMainWindow {
public:
    MainWindow();
    ~MainWindow() override;

    bool openFile(const QString& datFile);

private:
    void buildMenu();
    void buildButtons();

    void createDatabase();
    void openDatabase();
    void saveDatabase();
    void clearDatabase();
    void deleteDatabase();

    void addBook();
    void editBook();
    void removeBooks();
    void searchBooks();
    void showAll();

    void makeBackup();
    void restoreBackup();
    void exportXlsx();
    void exportCsv();
    void generateBooks();

    void reloadTable();
    void fillTable(const std::vector<Book>& books);
    void appendRow(const Book& book);

    void timed(const QString& name, const std::function<void()>& action);

    bool requireOpen();

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
