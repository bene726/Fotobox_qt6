#include "QrCard.h"
#include "QrCodeWidget.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

QrCard::QrCard(const QString& link, const QString& password, int qrSize, QWidget* parent)
    : QWidget(parent)
{
    setObjectName("qrCard");
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    m_qr = new QrCodeWidget(link, this);
    m_qr->setFixedSize(qrSize, qrSize);

    m_textBox = new QWidget(this);
    auto* v = new QVBoxLayout(m_textBox);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(6);

    m_title = new QLabel(m_textBox);
    m_title->setObjectName("qrTitle");
    m_link = new QLabel(link, m_textBox);
    m_link->setObjectName("qrLink");

    auto* pwRow = new QHBoxLayout;
    pwRow->setContentsMargins(0, 0, 0, 0);
    pwRow->setSpacing(8);
    m_passwordLabel = new QLabel("PASSWORT", m_textBox);
    m_passwordLabel->setObjectName("qrPasswordLabel");
    m_password = new QLabel(password, m_textBox);
    m_password->setObjectName("qrPassword");
    pwRow->addWidget(m_passwordLabel);
    pwRow->addWidget(m_password);
    pwRow->addStretch();

    v->addWidget(m_title);
    v->addWidget(m_link);
    v->addLayout(pwRow);

    const bool hasPassword = !password.isEmpty();
    m_passwordLabel->setVisible(hasPassword);
    m_password->setVisible(hasPassword);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(16, 14, 16, 14);
    m_layout->setSpacing(16);
    m_layout->addWidget(m_textBox);
    m_layout->addWidget(m_qr);
}

void QrCard::applyTheme(const Theme& theme)
{
    m_title->setText(theme.qrTitle);

    // Reihenfolge: QR links oder rechts
    m_layout->removeWidget(m_qr);
    m_layout->removeWidget(m_textBox);
    if (theme.qrOnLeft) {
        m_layout->addWidget(m_qr);
        m_layout->addWidget(m_textBox);
    } else {
        m_layout->addWidget(m_textBox);
        m_layout->addWidget(m_qr);
    }

    setStyleSheet(theme.qrCardStyle());

    // Schrift aus dem Stylesheet sofort uebernehmen, damit sizeHint stimmt.
    const QList<QLabel*> labels = findChildren<QLabel*>();
    for (QLabel* l : labels) {
        l->style()->unpolish(l);
        l->style()->polish(l);
        l->updateGeometry();
    }
    m_textBox->adjustSize();
    adjustSize();
}
