#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../WindCore/emubase.h"
#include "pdascreenwindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
	explicit MainWindow(EmuBase *emu, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void execTimer();

    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_stepInsnButton_clicked();
    void on_stepTickButton_clicked();

    void on_addBreakButton_clicked();

    void on_removeBreakButton_clicked();

    void on_memoryViewAddress_textEdited(const QString &arg1);
    void on_memoryAdd1_clicked();
    void on_memoryAdd4_clicked();
    void on_memoryAdd10_clicked();
    void on_memoryAdd100_clicked();
    void on_memorySub1_clicked();
    void on_memorySub4_clicked();
    void on_memorySub10_clicked();
    void on_memorySub100_clicked();

    void on_writeByteButton_clicked();

    void on_writeDwordButton_clicked();

private:
    Ui::MainWindow *ui;
	PDAScreenWindow pdaScreen;
	EmuBase *emu;
    QTimer *timer;
    void updateScreen();
    void updateBreakpointsList();
    void updateMemory();
    void adjustMemoryAddress(int offset);
};

#endif // MAINWINDOW_H
