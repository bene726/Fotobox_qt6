// Tests fuer die Bildratenbegrenzung des Livebilds.
//
// Die Uhr kommt von aussen, deshalb laeuft hier nichts in Echtzeit: ein
// ganzer Abend Kamerabetrieb ist in Millisekunden durchgerechnet.

#include <QtTest>

#include "core/FrameGate.h"

class TestFrameGate : public QObject
{
    Q_OBJECT

private slots:
    void withoutLimitEverythingPasses_data();
    void withoutLimitEverythingPasses();

    void firstFrameAlwaysPasses();
    void deliveredRateIsReduced_data();
    void deliveredRateIsReduced();
    void sameRateIsNotThrottled();
    void doesNotCatchUpAfterAPause();
    void resetStartsOver();
};

void TestFrameGate::withoutLimitEverythingPasses_data()
{
    QTest::addColumn<int>("fps");

    QTest::newRow("null")     << 0;
    QTest::newRow("negativ")  << -5;
    QTest::newRow("ueber 1000") << 2000;   // 1000/2000 = 0, also keine Schranke
}

void TestFrameGate::withoutLimitEverythingPasses()
{
    QFETCH(int, fps);

    FrameGate gate;
    gate.setFps(fps);
    // Nicht eingestellt heisst: nichts wegwerfen. Lieber zu viel Rechenzeit
    // als ein Livebild, das ohne Grund ruckelt.
    for (qint64 t = 0; t < 1000; t += 33)
        QVERIFY2(gate.accept(t), qPrintable(QString("Bild bei %1 ms verworfen").arg(t)));
}

void TestFrameGate::firstFrameAlwaysPasses()
{
    FrameGate gate;
    gate.setFps(1);
    // Sonst bliebe der Bildschirm nach dem Start bis zu einer Sekunde leer.
    QVERIFY(gate.accept(123456));
}

void TestFrameGate::deliveredRateIsReduced_data()
{
    QTest::addColumn<int>("kameraFps");
    QTest::addColumn<int>("zielFps");

    QTest::newRow("30 -> 20") << 30 << 20;
    QTest::newRow("30 -> 15") << 30 << 15;
    QTest::newRow("30 -> 10") << 30 << 10;
    QTest::newRow("60 -> 20") << 60 << 20;
    QTest::newRow("25 -> 12") << 25 << 12;
}

void TestFrameGate::deliveredRateIsReduced()
{
    QFETCH(int, kameraFps);
    QFETCH(int, zielFps);

    FrameGate gate;
    gate.setFps(zielFps);

    const int sekunden = 10;
    int durch = 0;
    const int bilder = kameraFps * sekunden;
    for (int i = 0; i < bilder; ++i)
        if (gate.accept(qint64(i) * 1000 / kameraFps))
            durch++;

    const double gemessen = double(durch) / sekunden;
    // Genau zu treffen ist unmoeglich: es lassen sich nur ganze Bilder
    // nehmen. 20 von 30 heisst zwei von drei. Eine Abweichung von einem
    // Fuenftel ist fuer eine Vorschau belanglos, ein Faktor zwei nicht.
    QVERIFY2(gemessen <= zielFps * 1.2 && gemessen >= zielFps * 0.8,
             qPrintable(QString("%1 fps durchgelassen, erwartet rund %2")
                            .arg(gemessen, 0, 'f', 1).arg(zielFps)));
}

void TestFrameGate::sameRateIsNotThrottled()
{
    FrameGate gate;
    gate.setFps(30);

    int durch = 0;
    for (int i = 0; i < 300; ++i)
        if (gate.accept(qint64(i) * 1000 / 30))
            durch++;

    // Wer 30 einstellt und 30 bekommt, darf kein Ruckeln geschenkt bekommen.
    QCOMPARE(durch, 300);
}

void TestFrameGate::doesNotCatchUpAfterAPause()
{
    FrameGate gate;
    gate.setFps(10);              // alle 100 ms
    QVERIFY(gate.accept(0));

    // Waehrend einer Aufnahme kommt sekundenlang nichts an.
    QVERIFY(gate.accept(5000));

    // Danach darf nicht der ganze Rueckstand auf einmal durchgelassen werden.
    QVERIFY(!gate.accept(5010));
    QVERIFY(!gate.accept(5050));
    QVERIFY(gate.accept(5100));
}

void TestFrameGate::resetStartsOver()
{
    FrameGate gate;
    gate.setFps(10);
    QVERIFY(gate.accept(0));
    QVERIFY(!gate.accept(50));

    gate.reset();
    // Nach einem Kamerawechsel faengt die Zeitrechnung neu an.
    QVERIFY(gate.accept(50));
}

QTEST_GUILESS_MAIN(TestFrameGate)
#include "tst_framegate.moc"
