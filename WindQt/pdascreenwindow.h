#ifndef PDASCREENWINDOW_H
#define PDASCREENWINDOW_H

#include <QWidget>
#include <QLabel>
#include "emubase.h"

class PDAScreenWindow : public QWidget
{
	Q_OBJECT
private:
	EmuBase *emu;
	QLabel *lcd;

public:
	explicit PDAScreenWindow(EmuBase *emu, QWidget *parent = nullptr);

public slots:
	void updateScreen();

protected:
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // PDASCREENWINDOW_H
