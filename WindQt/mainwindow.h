#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../WindCore/emu.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void execTimer();

    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_stepInsnButton_clicked();
    void on_stepTickButton_clicked();

private:
    Ui::MainWindow *ui;
    Emu *emu;
    QTimer *timer;
    void updateScreen();


protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
};

#endif // MAINWINDOW_H
