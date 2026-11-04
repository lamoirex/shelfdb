#include "BookDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QPushButton>
#include <climits>

BookDialog::BookDialog(QWidget* parent, const QString& title, const Book* book)
    : QDialog(parent) {
    setWindowTitle(title);

    idEdit = new QSpinBox;
    idEdit->setRange(INT_MIN, INT_MAX);

    titleEdit = new QLineEdit;
    authorEdit = new QLineEdit;

    yearEdit = new QSpinBox;
    yearEdit->setRange(-3000, 3000);
    yearEdit->setValue(2024);

    priceEdit = new QDoubleSpinBox;
    priceEdit->setRange(0, 1e9);
    priceEdit->setDecimals(2);

    availableEdit = new QCheckBox("Есть в наличии");
    availableEdit->setChecked(true);

    if (book) {
        idEdit->setValue(book->id);
        titleEdit->setText(QString::fromStdString(getText(book->title, TITLE_LEN)));
        authorEdit->setText(QString::fromStdString(getText(book->author, AUTHOR_LEN)));
        yearEdit->setValue(book->year);
        priceEdit->setValue(book->price);
        availableEdit->setChecked(book->available);
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
    connect(buttons, &QDialogButtonBox::accepted, this, [this] { acceptIfValid(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* form = new QFormLayout(this);
    form->addRow("ID:", idEdit);
    form->addRow("Название:", titleEdit);
    form->addRow("Автор:", authorEdit);
    form->addRow("Год:", yearEdit);
    form->addRow("Цена:", priceEdit);
    form->addRow("", availableEdit);
    form->addRow(buttons);

    setMinimumWidth(380);
}

void BookDialog::acceptIfValid() {
    // Длина поля в файле считается в байтах UTF-8, а не в символах.
    int titleBytes = titleEdit->text().toUtf8().size();
    int authorBytes = authorEdit->text().toUtf8().size();

    if (titleBytes > TITLE_LEN || authorBytes > AUTHOR_LEN) {
        QMessageBox::warning(this, "Слишком длинная строка",
            QString("Название — не больше %1 байт, автор — не больше %2 байт.\n"
                    "Русская буква занимает 2 байта.\n\n"
                    "Сейчас: название %3 байт, автор %4 байт.")
                .arg(TITLE_LEN).arg(AUTHOR_LEN).arg(titleBytes).arg(authorBytes));
        return;
    }
    if (authorEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Пустой автор", "Укажите автора.");
        return;
    }
    accept();
}

Book BookDialog::book() const {
    return makeBook(idEdit->value(),
                    titleEdit->text().trimmed().toStdString(),
                    authorEdit->text().trimmed().toStdString(),
                    yearEdit->value(),
                    priceEdit->value(),
                    availableEdit->isChecked());
}
