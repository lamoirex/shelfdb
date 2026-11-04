#include "MainWindow.h"

#include "Backup.h"
#include "BookDialog.h"
#include "Exporter.h"
#include "FieldValueDialog.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <random>

namespace {

// Больше строк на экран не выводим: таблица на миллион строк будет тормозить.
// Сама база при этом может быть любого размера.
const int MAX_ROWS_ON_SCREEN = 10000;

// Путь без расширения: "/a/books.dat" -> "/a/books".
std::string basePathOf(const QString& file) {
    QFileInfo info(file);
    return (info.absolutePath() + "/" + info.completeBaseName()).toStdString();
}

}  // namespace

MainWindow::MainWindow() {
    setWindowTitle("ShelfDB — библиотека");
    resize(960, 600);

    tableTitle = new QLabel;
    tableTitle->setStyleSheet("font-weight: bold; padding: 4px;");

    table = new QTableWidget(0, 6);
    table->setHorizontalHeaderLabels({"ID", "Название", "Автор", "Год", "Цена", "В наличии"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);   // править только через диалог
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    connect(table, &QTableWidget::cellDoubleClicked, this, [this] { editBook(); });

    auto* central = new QWidget;
    auto* layout = new QVBoxLayout(central);
    buildButtons();
    auto* buttonRow = new QHBoxLayout;
    for (QPushButton* b : needOpenButtons) buttonRow->addWidget(b);
    buttonRow->addStretch();
    layout->addLayout(buttonRow);
    layout->addWidget(tableTitle);
    layout->addWidget(table);
    setCentralWidget(central);

    pathLabel = new QLabel;
    timeLabel = new QLabel;
    statusBar()->addWidget(pathLabel, 1);
    statusBar()->addPermanentWidget(timeLabel);

    buildMenu();
    updateState();
}

MainWindow::~MainWindow() {
    db.close();
}

// ---------- построение интерфейса ----------

void MainWindow::buildMenu() {
    QMenu* base = menuBar()->addMenu("База");
    base->addAction("Создать…", QKeySequence::New, this, [this] { createDatabase(); });
    base->addAction("Открыть…", QKeySequence::Open, this, [this] { openDatabase(); });
    needOpenActions.push_back(base->addAction("Сохранить", QKeySequence::Save, this, [this] { saveDatabase(); }));
    base->addSeparator();
    needOpenActions.push_back(base->addAction("Очистить", this, [this] { clearDatabase(); }));
    needOpenActions.push_back(base->addAction("Удалить базу…", this, [this] { deleteDatabase(); }));

    QMenu* service = menuBar()->addMenu("Сервис");
    needOpenActions.push_back(service->addAction("Создать backup", this, [this] { makeBackup(); }));
    needOpenActions.push_back(service->addAction("Восстановить из backup…", this, [this] { restoreBackup(); }));
    service->addSeparator();
    needOpenActions.push_back(service->addAction("Экспорт в Excel (.xlsx)…", this, [this] { exportXlsx(); }));
    needOpenActions.push_back(service->addAction("Экспорт в CSV…", this, [this] { exportCsv(); }));
    service->addSeparator();
    needOpenActions.push_back(service->addAction("Сгенерировать книги…", this, [this] { generateBooks(); }));
}

void MainWindow::buildButtons() {
    auto add = [this](const QString& text, std::function<void()> action) {
        auto* button = new QPushButton(text);
        connect(button, &QPushButton::clicked, this, action);
        needOpenButtons.push_back(button);
    };
    add("Добавить", [this] { addBook(); });
    add("Редактировать", [this] { editBook(); });
    add("Удалить…", [this] { removeBooks(); });
    add("Найти…", [this] { searchBooks(); });
    add("Показать все", [this] { showAll(); });
}

void MainWindow::updateState() {
    bool open = db.isOpen();
    for (QAction* a : needOpenActions) a->setEnabled(open);
    for (QPushButton* b : needOpenButtons) b->setEnabled(open);

    if (open) {
        pathLabel->setText(QString("%1.dat   •   книг: %2")
                               .arg(QString::fromStdString(db.basePath()))
                               .arg(db.count()));
    } else {
        pathLabel->setText("База не открыта. Меню «База» → «Создать» или «Открыть».");
        table->setRowCount(0);
        tableTitle->clear();
    }
}

// ---------- вспомогательное ----------

void MainWindow::timed(const QString& name, const std::function<void()>& action) {
    QElapsedTimer timer;
    timer.start();
    action();
    double us = timer.nsecsElapsed() / 1000.0;
    timeLabel->setText(QString("%1: %2 мкс").arg(name).arg(us, 0, 'f', 1));
}

bool MainWindow::requireOpen() {
    if (db.isOpen()) return true;
    QMessageBox::information(this, "Нет базы", "Сначала создайте или откройте базу.");
    return false;
}

bool MainWindow::selectedId(int& id) {
    int row = table->currentRow();
    if (row < 0 || !table->item(row, 0)) return false;
    id = table->item(row, 0)->text().toInt();
    return true;
}

void MainWindow::appendRow(const Book& b) {
    int row = table->rowCount();
    table->insertRow(row);

    auto number = [](double v, int decimals) {
        auto* item = new QTableWidgetItem(QString::number(v, 'f', decimals));
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return item;
    };
    table->setItem(row, 0, number(b.id, 0));
    table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(getText(b.title, TITLE_LEN))));
    table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(getText(b.author, AUTHOR_LEN))));
    table->setItem(row, 3, number(b.year, 0));
    table->setItem(row, 4, number(b.price, 2));
    table->setItem(row, 5, new QTableWidgetItem(b.available ? "да" : "нет"));
}

void MainWindow::reloadTable() {
    table->setUpdatesEnabled(false);
    table->setRowCount(0);

    int shown = 0;
    db.forEach([&](const Book& b) {
        if (shown < MAX_ROWS_ON_SCREEN) appendRow(b);
        shown++;
    });

    table->setUpdatesEnabled(true);
    if (shown > MAX_ROWS_ON_SCREEN) {
        tableTitle->setText(QString("Все книги (показаны первые %1 из %2)").arg(MAX_ROWS_ON_SCREEN).arg(shown));
    } else {
        tableTitle->setText(QString("Все книги (%1)").arg(shown));
    }
    updateState();
}

void MainWindow::fillTable(const std::vector<Book>& books) {
    table->setUpdatesEnabled(false);
    table->setRowCount(0);
    for (size_t i = 0; i < books.size() && i < (size_t)MAX_ROWS_ON_SCREEN; i++) {
        appendRow(books[i]);
    }
    table->setUpdatesEnabled(true);
}

// ---------- База ----------

void MainWindow::createDatabase() {
    QString file = QFileDialog::getSaveFileName(this, "Новая база", "books.dat", "База ShelfDB (*.dat)");
    if (file.isEmpty()) return;

    if (!db.create(basePathOf(file))) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать базу.");
    }
    reloadTable();
}

void MainWindow::openDatabase() {
    QString file = QFileDialog::getOpenFileName(this, "Открыть базу", "", "База ShelfDB (*.dat)");
    if (file.isEmpty()) return;
    openFile(file);
}

bool MainWindow::openFile(const QString& datFile) {
    bool ok = db.open(basePathOf(datFile));
    if (!ok) {
        QMessageBox::critical(this, "Ошибка",
                              "Не удалось открыть базу.\nНужны файлы .dat, .pidx и .sidx с одним именем.");
    }
    reloadTable();
    return ok;
}

void MainWindow::saveDatabase() {
    if (!requireOpen()) return;
    timed("Сохранение", [this] { db.save(); });
    statusBar()->showMessage("Сохранено", 2000);
}

void MainWindow::clearDatabase() {
    if (!requireOpen()) return;
    if (QMessageBox::question(this, "Очистить базу", "Удалить все книги из базы?") != QMessageBox::Yes) return;

    timed("Очистка", [this] { db.clear(); });
    reloadTable();
}

void MainWindow::deleteDatabase() {
    if (!requireOpen()) return;
    QString name = QString::fromStdString(db.basePath());
    if (QMessageBox::warning(this, "Удалить базу",
                             "Файлы базы будут удалены с диска:\n" + name + ".dat, .pidx, .sidx\n\nПродолжить?",
                             QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    if (!db.destroy()) {
        QMessageBox::critical(this, "Ошибка", "Не все файлы удалось удалить.");
    }
    updateState();
}

// ---------- Записи ----------

void MainWindow::addBook() {
    if (!requireOpen()) return;
    BookDialog dialog(this, "Новая книга");
    if (dialog.exec() != QDialog::Accepted) return;

    Book book = dialog.book();
    bool added = false;
    timed("Добавление", [&] { added = db.add(book); });

    if (!added) {
        QMessageBox::warning(this, "Не добавлено", QString("Книга с ID %1 уже есть в базе.").arg(book.id));
        return;
    }
    reloadTable();
}

void MainWindow::editBook() {
    if (!requireOpen()) return;
    int id;
    if (!selectedId(id)) {
        QMessageBox::information(this, "Редактирование", "Выделите книгу в таблице.");
        return;
    }

    // Берём книгу из файла, а не из таблицы на экране.
    Book current;
    if (!db.findById(id, current)) {
        QMessageBox::warning(this, "Ошибка", "Книга уже удалена.");
        reloadTable();
        return;
    }

    BookDialog dialog(this, "Редактирование книги", &current);
    if (dialog.exec() != QDialog::Accepted) return;

    Book changed = dialog.book();
    bool updated = false;
    timed("Редактирование", [&] { updated = db.update(id, changed); });

    if (!updated) {
        QMessageBox::warning(this, "Не сохранено", QString("Книга с ID %1 уже есть в базе.").arg(changed.id));
        return;
    }
    reloadTable();
}

void MainWindow::removeBooks() {
    if (!requireOpen()) return;
    FieldValueDialog dialog(this, "Удаление книг", "Удалить");
    if (dialog.exec() != QDialog::Accepted) return;

    int removed = 0;
    timed("Удаление", [&] { removed = db.remove(dialog.field(), dialog.value()); });

    QMessageBox::information(this, "Удаление", QString("Удалено книг: %1").arg(removed));
    reloadTable();
}

void MainWindow::searchBooks() {
    if (!requireOpen()) return;
    FieldValueDialog dialog(this, "Поиск книг", "Найти");
    if (dialog.exec() != QDialog::Accepted) return;

    std::vector<Book> found;
    timed("Поиск", [&] { found = db.find(dialog.field(), dialog.value()); });

    fillTable(found);
    tableTitle->setText(QString("Результаты поиска: %1 = «%2» — найдено %3. Нажмите «Показать все», чтобы вернуться.")
                            .arg(QString::fromStdString(fieldName(dialog.field())))
                            .arg(QString::fromStdString(dialog.value()))
                            .arg(found.size()));
}

void MainWindow::showAll() {
    if (!requireOpen()) return;
    timed("Чтение всех книг", [this] { reloadTable(); });
}

// ---------- Сервис ----------

void MainWindow::makeBackup() {
    if (!requireOpen()) return;
    std::string folder;
    timed("Backup", [&] { folder = Backup::create(db); });

    if (folder.empty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать backup.");
    } else {
        QMessageBox::information(this, "Backup", "Копия сохранена в папку:\n" + QString::fromStdString(folder));
    }
}

void MainWindow::restoreBackup() {
    if (!requireOpen()) return;
    QString start = QFileInfo(QString::fromStdString(db.basePath())).absolutePath() + "/backups";
    QString folder = QFileDialog::getExistingDirectory(this, "Папка с backup", start);
    if (folder.isEmpty()) return;

    bool ok = false;
    timed("Восстановление", [&] { ok = Backup::restore(db, folder.toStdString()); });

    if (!ok) {
        QMessageBox::critical(this, "Ошибка", "В этой папке нет корректной копии базы.");
    }
    reloadTable();
}

void MainWindow::exportXlsx() {
    if (!requireOpen()) return;
    QString file = QFileDialog::getSaveFileName(this, "Экспорт в Excel", "books.xlsx", "Excel (*.xlsx)");
    if (file.isEmpty()) return;

    bool ok = false;
    timed("Экспорт", [&] { ok = Exporter::toXlsx(db, file.toStdString()); });
    if (!ok) QMessageBox::critical(this, "Ошибка", "Не удалось записать файл.");
    else statusBar()->showMessage("Экспортировано в " + file, 4000);
}

void MainWindow::exportCsv() {
    if (!requireOpen()) return;
    QString file = QFileDialog::getSaveFileName(this, "Экспорт в CSV", "books.csv", "CSV (*.csv)");
    if (file.isEmpty()) return;

    bool ok = false;
    timed("Экспорт", [&] { ok = Exporter::toCsv(db, file.toStdString()); });
    if (!ok) QMessageBox::critical(this, "Ошибка", "Не удалось записать файл.");
    else statusBar()->showMessage("Экспортировано в " + file, 4000);
}

// Заполняет базу случайными книгами — для демонстрации на защите.
void MainWindow::generateBooks() {
    if (!requireOpen()) return;
    bool ok = false;
    int n = QInputDialog::getInt(this, "Сгенерировать книги", "Сколько книг добавить:", 1000, 1, 1000000, 1000, &ok);
    if (!ok) return;

    const char* authors[] = {"Толстой", "Достоевский", "Чехов", "Пушкин", "Гоголь",
                             "Булгаков", "Тургенев", "Лермонтов", "Набоков", "Пастернак"};
    const char* words[] = {"Война", "Мир", "Ночь", "Сад", "Дом", "Река", "Город", "Время", "Море", "Сон"};
    std::mt19937 rng(std::random_device{}());

    // Новые id идут после самого большого существующего, чтобы не было дубликатов.
    int nextId = 1;
    db.forEach([&](const Book& b) { if (b.id >= nextId) nextId = b.id + 1; });

    QApplication::setOverrideCursor(Qt::WaitCursor);
    timed(QString("Добавление %1 книг").arg(n), [&] {
        for (int i = 0; i < n; i++) {
            std::string title = std::string(words[rng() % 10]) + " и " + words[rng() % 10];
            db.add(makeBook(nextId++, title, authors[rng() % 10],
                            1800 + (int)(rng() % 225), (rng() % 100000) / 100.0, rng() % 2));
        }
    });
    QApplication::restoreOverrideCursor();
    reloadTable();
}
