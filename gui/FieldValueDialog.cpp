#include "FieldValueDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QPushButton>

FieldValueDialog::FieldValueDialog(QWidget* parent, const QString& title, const QString& okText)
    : QDialog(parent) {
    setWindowTitle(title);

    fieldBox = new QComboBox;
    for (Field f : ALL_FIELDS) {
        fieldBox->addItem(QString::fromStdString(fieldName(f)), (int)f);
    }

    valueEdit = new QLineEdit;
    hint = new QLabel;
    hint->setStyleSheet("color: gray;");
    hint->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
    buttons->button(QDialogButtonBox::Ok)->setText(okText);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] { acceptIfValid(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(fieldBox, &QComboBox::currentIndexChanged, this, [this] { updateHint(); });

    auto* form = new QFormLayout(this);
    form->addRow("Поле:", fieldBox);
    form->addRow("Значение:", valueEdit);
    form->addRow(hint);
    form->addRow(buttons);

    setMinimumWidth(380);
    updateHint();
}

Field FieldValueDialog::field() const {
    return (Field)fieldBox->currentData().toInt();
}

std::string FieldValueDialog::value() const {
    return valueEdit->text().trimmed().toStdString();
}

// Подсказка показывает, как будет выполняться операция и с какой сложностью.
void FieldValueDialog::updateHint() {
    switch (field()) {
        case Field::Id:
            hint->setText("Ключевое поле. Через хеш-индекс, O(1).");
            break;
        case Field::Author:
            hint->setText("Индексированное поле. Через хеш-индекс и цепочку книг автора, O(k).");
            break;
        case Field::Available:
            hint->setText("Значение: да / нет. Без индекса, проход по файлу, O(n).");
            break;
        default:
            hint->setText("Без индекса, проход по файлу, O(n).");
    }
}

void FieldValueDialog::acceptIfValid() {
    std::string text = value();
    int i;
    double d;
    bool b;
    bool ok = true;

    switch (field()) {
        case Field::Id:
        case Field::Year:      ok = parseInt(text, i); break;
        case Field::Price:     ok = parseDouble(text, d); break;
        case Field::Available: ok = parseBool(text, b); break;
        default:               ok = true;
    }

    if (!ok) {
        QMessageBox::warning(this, "Неверное значение",
                             "Значение не подходит для поля «" + fieldBox->currentText() + "».");
        return;
    }
    accept();
}
