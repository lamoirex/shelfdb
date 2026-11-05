#pragma once

#include "BookField.h"

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>

class FieldValueDialog : public QDialog {
public:
    FieldValueDialog(QWidget* parent, const QString& title, const QString& okText);

    Field field() const;
    std::string value() const;

private:
    void updateHint();
    void acceptIfValid();

    QComboBox* fieldBox;
    QLineEdit* valueEdit;
    QLabel* hint;
};
