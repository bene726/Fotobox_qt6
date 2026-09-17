#include "SettingsPanel.h"
#include "Theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QAbstractItemView>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

SettingsPanel::SettingsPanel(const Config& cfg, QWidget* parent)
    : QWidget(parent)
{
    setObjectName("settingsPanel");
    setAttribute(Qt::WA_StyledBackground, true);

    auto* title = new QLabel("Einstellungen", this);
    title->setObjectName("title");

    m_theme = new QComboBox(this);
    m_theme->setItemDelegate(new QStyledItemDelegate(m_theme)); // damit das Stylesheet fuer die Liste greift
    for (const Theme& t : Theme::all())
        m_theme->addItem(t.name, t.id);

    m_led = new QSlider(Qt::Horizontal, this);
    m_led->setRange(0, 100);
    m_ledValue = new QLabel(this);

    m_countdown = new QSlider(Qt::Horizontal, this);
    m_countdown->setRange(0, 10);
    m_countdownValue = new QLabel(this);

    m_photo = new QSlider(Qt::Horizontal, this);
    m_photo->setRange(1, 15);
    m_photoValue = new QLabel(this);

    m_showQr = new QCheckBox("QR-Code anzeigen", this);

    auto* save = new QPushButton("Speichern", this);
    save->setObjectName("primary");
    auto* close = new QPushButton(QString::fromUtf8("Schließen"), this);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(close);
    buttons->addWidget(save);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 26, 32, 26);
    layout->setSpacing(14);
    layout->addWidget(title);
    layout->addWidget(makeRow("Design", m_theme));
    layout->addWidget(makeRow("LED-Helligkeit", m_led, m_ledValue));
    layout->addWidget(makeRow("Countdown", m_countdown, m_countdownValue));
    layout->addWidget(makeRow("Foto anzeigen", m_photo, m_photoValue));
    layout->addWidget(m_showQr);
    layout->addStretch();
    layout->addLayout(buttons);

    connect(m_theme, &QComboBox::currentIndexChanged, this, [this](int i) {
        emit themeChanged(m_theme->itemData(i).toString());
    });
    connect(m_led, &QSlider::valueChanged, this, [this](int v) {
        m_ledValue->setText(QString::number(v) + " %");
        emit ledBrightnessChanged(v);
    });
    connect(m_countdown, &QSlider::valueChanged, this, [this](int v) {
        m_countdownValue->setText(QString::number(v) + " s");
        emit countdownSecondsChanged(v);
    });
    connect(m_photo, &QSlider::valueChanged, this, [this](int v) {
        m_photoValue->setText(QString::number(v) + " s");
        emit photoDisplayMsChanged(v * 1000);
    });
    connect(m_showQr, &QCheckBox::toggled, this, &SettingsPanel::showQrChanged);
    connect(save, &QPushButton::clicked, this, &SettingsPanel::saveRequested);
    connect(close, &QPushButton::clicked, this, &SettingsPanel::closeRequested);

    setConfig(cfg);
}

QWidget* SettingsPanel::makeRow(const QString& title, QWidget* control, QLabel* value)
{
    auto* row = new QWidget(this);
    row->setStyleSheet("background: transparent;");
    auto* l = new QHBoxLayout(row);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(16);

    auto* t = new QLabel(title, row);
    t->setObjectName("rowTitle");
    t->setMinimumWidth(170);
    control->setMinimumHeight(44);

    l->addWidget(t);
    l->addWidget(control, 1);
    if (value) {
        value->setObjectName("rowValue");
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->addWidget(value);
    }
    return row;
}

void SettingsPanel::applyTheme(const Theme& theme)
{
    setStyleSheet(theme.settingsStyle());

    // Die Dropdown-Liste ist ein eigenes Popup-Fenster und erbt das Stylesheet
    // nicht zuverlaessig (macOS). Deshalb Liste und Rahmen direkt stylen.
    QAbstractItemView* view = m_theme->view();
    view->setStyleSheet(theme.comboPopupStyle());
    if (QWidget* frame = view->parentWidget())
        frame->setStyleSheet(theme.comboPopupStyle());
}

void SettingsPanel::openThemeDropdown()
{
    m_theme->showPopup();
}

void SettingsPanel::setLedBrightness(int percent)
{
    QSignalBlocker b(m_led);
    m_led->setValue(percent);
    m_ledValue->setText(QString::number(percent) + " %");
}

void SettingsPanel::setConfig(const Config& cfg)
{
    setLedBrightness(cfg.ledBrightness);
    {
        QSignalBlocker b(m_theme);
        const int idx = m_theme->findData(cfg.theme);
        m_theme->setCurrentIndex(idx < 0 ? 0 : idx);
    }
    {
        QSignalBlocker b(m_countdown);
        m_countdown->setValue(cfg.countdownSeconds);
        m_countdownValue->setText(QString::number(cfg.countdownSeconds) + " s");
    }
    {
        QSignalBlocker b(m_photo);
        const int s = qMax(1, cfg.photoDisplayMs / 1000);
        m_photo->setValue(s);
        m_photoValue->setText(QString::number(s) + " s");
    }
    {
        QSignalBlocker b(m_showQr);
        m_showQr->setChecked(cfg.showQr);
    }
}
