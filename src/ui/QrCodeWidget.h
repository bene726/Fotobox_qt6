#pragma once
#include <QWidget>
#include <QImage>

class QrCodeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QrCodeWidget(const QString& text, QWidget* parent = nullptr);
    void setText(const QString& text);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QString text;
    QImage qrImage;
};
