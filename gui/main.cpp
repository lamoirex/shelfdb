#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();

    // shelfdb /путь/books.dat — сразу открыть базу.
    if (argc > 1) window.openFile(QString::fromLocal8Bit(argv[1]));

    return app.exec();
}
