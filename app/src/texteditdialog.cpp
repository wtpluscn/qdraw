#include "texteditdialog.h"
#include <QPlainTextEdit>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

TextEditDialog::TextEditDialog(QWidget *parent)
    : QDialog(parent)
    , m_textColor(Qt::black)
{
    setWindowTitle(tr("Text"));

    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setPlaceholderText(tr("Enter text..."));
    m_textEdit->setMaximumBlockCount(1024);

    m_fontCombo = new QFontComboBox(this);
    m_sizeSpin = new QSpinBox(this);
    m_sizeSpin->setRange(6, 288);
    m_sizeSpin->setValue(12);
    m_sizeSpin->setSuffix(tr(" pt"));

    m_boldCheck = new QCheckBox(tr("Bold"), this);
    m_italicCheck = new QCheckBox(tr("Italic"), this);

    m_colorButton = new QPushButton(this);
    m_colorButton->setAutoFillBackground(true);
    updateColorButton();

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel(tr("Content:")));
    mainLayout->addWidget(m_textEdit);

    QFormLayout *form = new QFormLayout();
    form->addRow(tr("Font:"), m_fontCombo);
    QHBoxLayout *sizeRow = new QHBoxLayout();
    sizeRow->addWidget(m_sizeSpin);
    sizeRow->addWidget(m_boldCheck);
    sizeRow->addWidget(m_italicCheck);
    form->addRow(tr("Size:"), sizeRow);
    form->addRow(tr("Color:"), m_colorButton);
    mainLayout->addLayout(form);

    mainLayout->addWidget(buttons);

    connect(m_colorButton, &QPushButton::clicked, this, &TextEditDialog::chooseColor);
}

QString TextEditDialog::text() const
{
    return m_textEdit->toPlainText().trimmed();
}

void TextEditDialog::setText(const QString &t)
{
    m_textEdit->setPlainText(t);
}

QFont TextEditDialog::font() const
{
    QFont f = m_fontCombo->currentFont();
    f.setPointSize(m_sizeSpin->value());
    f.setBold(m_boldCheck->isChecked());
    f.setItalic(m_italicCheck->isChecked());
    return f;
}

void TextEditDialog::setFont(const QFont &f)
{
    m_fontCombo->setCurrentFont(f);
    if (f.pointSize() > 0)
        m_sizeSpin->setValue(f.pointSize());
    m_boldCheck->setChecked(f.bold());
    m_italicCheck->setChecked(f.italic());
}

QColor TextEditDialog::textColor() const
{
    return m_textColor;
}

void TextEditDialog::setTextColor(const QColor &c)
{
    m_textColor = c;
    updateColorButton();
}

void TextEditDialog::updateColorButton()
{
    QPalette pal = m_colorButton->palette();
    pal.setColor(QPalette::Button, m_textColor);
    m_colorButton->setPalette(pal);
    m_colorButton->setText(m_textColor.name());
    m_colorButton->update();
}

void TextEditDialog::chooseColor()
{
    QColor c = QColorDialog::getColor(m_textColor, this, tr("Choose text color"));
    if (c.isValid())
        setTextColor(c);
}

