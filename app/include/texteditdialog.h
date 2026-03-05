#ifndef TEXTEDITDIALOG_H
#define TEXTEDITDIALOG_H

#include <QDialog>
#include <QFont>
#include <QColor>

class QPlainTextEdit;
class QFontComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;

class TextEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TextEditDialog(QWidget *parent = 0);

    QString text() const;
    void setText(const QString &t);

    QFont font() const;
    void setFont(const QFont &f);

    QColor textColor() const;
    void setTextColor(const QColor &c);

private slots:
    void chooseColor();

private:
    void updateColorButton();
    QPlainTextEdit *m_textEdit;
    QFontComboBox *m_fontCombo;
    QSpinBox *m_sizeSpin;
    QCheckBox *m_boldCheck;
    QCheckBox *m_italicCheck;
    QPushButton *m_colorButton;
    QColor m_textColor;
};

#endif // TEXTEDITDIALOG_H
