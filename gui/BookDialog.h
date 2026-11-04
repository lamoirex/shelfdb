#pragma once

#include "Book.h"

#include <QCheckBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QSpinBox>

// Окно для ввода книги: используется и для добавления, и для редактирования.
class BookDialog : public QDialog {
public:
    // Если book задана, поля заполняются её значениями (режим редактирования).
    BookDialog(QWidget* parent, const QString& title, const Book* book = nullptr);

    // Книга из введённых значений.
    Book book() const;

private:
    // Проверяет, что строки влезают в поля фиксированной длины.
    void acceptIfValid();

    QSpinBox* idEdit;
    QLineEdit* titleEdit;
    QLineEdit* authorEdit;
    QSpinBox* yearEdit;
    QDoubleSpinBox* priceEdit;
    QCheckBox* availableEdit;
};
