#pragma once
#include <QWidget>

struct Theme;
class QLabel;
class QrCodeWidget;
class QHBoxLayout;

// Karte mit QR-Code, Titel, Link und Passwort.
class QrCard : public QWidget
{
    Q_OBJECT
public:
    QrCard(const QString& link, const QString& partyname, const QString& password, int qrSize,
           QWidget* parent = nullptr);

    void applyTheme(const Theme& theme);
    // Der Link steckt nur im QR-Code, angezeigt wird der Eventname.
    void setContent(const QString& link, const QString& partyname, const QString& password,
                    int qrSize);

private:
    QHBoxLayout* m_layout = nullptr;
    QrCodeWidget* m_qr = nullptr;
    QWidget* m_textBox = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QLabel* m_passwordLabel = nullptr;
    QLabel* m_password = nullptr;
};
