#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../WindCore/wind.h"
#include <QTimer>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    emu = new Emu;
    emu->loadROM("/Users/ash/src/psion/Sys$rom.bin");

    timer = new QTimer(this);
    timer->start(1000/64);
    connect(timer, SIGNAL(timeout()), SLOT(execTimer()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::execTimer()
{
    emu->executeUntil(emu->currentCycles() + (CLOCK_SPEED / 64));
    ui->cycleCounter->setText(QString("Cycles: %1").arg(emu->currentCycles()));
    updateScreen();
}

void MainWindow::on_stepButton_clicked()
{
    emu->executeUntil(emu->currentCycles() + (CLOCK_SPEED * 2));
    ui->cycleCounter->setText(QString("Cycles: %1").arg(emu->currentCycles()));
    updateScreen();
}

void MainWindow::updateScreen()
{
    const uint8_t *lcdBuf = emu->getLCDBuffer();
    if (lcdBuf) {
        QImage img(640, 240, QImage::Format_Grayscale8);

        // fetch palette
        int bpp = 1 << (lcdBuf[1] >> 4);
        int ppb = 8 / bpp;
        uint16_t palette[16];
        for (int i = 0; i < 16; i++)
            palette[i] = lcdBuf[i*2] | ((lcdBuf[i*2+1] << 8) & 0xF00);

        // build our image out
        int lineWidth = (img.width() * bpp) / 8;
        for (int y = 0; y < img.height(); y++) {
            uint8_t *scanline = img.scanLine(y);
            int lineOffs = 0x20 + (lineWidth * y);
            for (int x = 0; x < img.width(); x++) {
                uint8_t byte = lcdBuf[lineOffs + (x / ppb)];
                int shift = (x & (ppb - 1)) * bpp;
                int mask = (bpp << 1) - 1;
                int palIdx = (byte >> shift) & mask;
                int palValue = palette[palIdx];

                if (bpp <= 1)
                    palValue |= (palValue << 1);
                if (bpp <= 2)
                    palValue |= (palValue << 2);
                if (bpp <= 4)
                    palValue |= (palValue << 4);
                scanline[x] = palValue ^ 0xFF;
            }
        }

        ui->screen->setPixmap(QPixmap::fromImage(std::move(img)));
    }
}


static int resolveKey(int key) {
    switch (key) {
    case Qt::Key_6: return 0;
    case Qt::Key_5: return 1;
    case Qt::Key_4: return 2;
    case Qt::Key_3: return 3;
    case Qt::Key_2: return 4;
    case Qt::Key_1: return 5;
    // missing 6: F13/rec

    case Qt::Key_Apostrophe: return 7;
    case Qt::Key_Backspace: return 8;
    case Qt::Key_0: return 9;
    case Qt::Key_9: return 10;
    case Qt::Key_8: return 11;
    case Qt::Key_7: return 12;
    // missing 13: F15/play

    case Qt::Key_Y: return 14;
    case Qt::Key_T: return 15;
    case Qt::Key_R: return 16;
    case Qt::Key_E: return 17;
    case Qt::Key_W: return 18;
    case Qt::Key_Q: return 19;
    case Qt::Key_Escape: return 20;

    case Qt::Key_Enter: return 21;
    case Qt::Key_Return: return 21;
    case Qt::Key_L: return 22;
    case Qt::Key_P: return 23;
    case Qt::Key_O: return 24;
    case Qt::Key_I: return 25;
    case Qt::Key_U: return 26;
    case Qt::Key_Alt: return 27; // actually Menu

    case Qt::Key_G: return 28;
    case Qt::Key_F: return 29;
    case Qt::Key_D: return 30;
    case Qt::Key_S: return 31;
    case Qt::Key_A: return 32;
    case Qt::Key_Tab: return 33;
#ifdef Q_OS_MAC
    case Qt::Key_Meta: return 34; // Control -> Control
#else
    case Qt::Key_Control: return 34; // Control -> Control
#endif

    case Qt::Key_Down: return 35;
    case Qt::Key_Period: return 36;
    case Qt::Key_M: return 37;
    case Qt::Key_K: return 38;
    case Qt::Key_J: return 39;
    case Qt::Key_H: return 40;
#ifdef Q_OS_MAC
    case Qt::Key_Control: return 41; // Command -> Fn
#else
    case Qt::Key_Meta: return 41; // Super -> Fn
#endif

    case Qt::Key_N: return 42;
    case Qt::Key_B: return 43;
    case Qt::Key_V: return 44;
    case Qt::Key_C: return 45;
    case Qt::Key_X: return 46;
    case Qt::Key_Z: return 47;
    case Qt::Key_Shift: return 48;

    case Qt::Key_Right: return 49;
    case Qt::Key_Left: return 50;
    case Qt::Key_Comma: return 51;
    case Qt::Key_Up: return 52;
    case Qt::Key_Space: return 53;
    // missing 54: F14/stop
    // missing 55: another Shift
    }
    return -1;
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int k = resolveKey(event->key());
    if (k >= 0)
        emu->keyboardKeys[k] = true;
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    int k = resolveKey(event->key());
    if (k >= 0)
        emu->keyboardKeys[k] = false;
}
