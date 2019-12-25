#include "pdascreenwindow.h"
#include <QKeyEvent>

PDAScreenWindow::PDAScreenWindow(EmuBase *emu, QWidget *parent) :
	QWidget(parent),
	emu(emu),
	lcd(new QLabel(this))
{
	setWindowTitle("WindEmu");
	setFixedSize(emu->getDigitiserWidth(), emu->getDigitiserHeight());
	lcd->setGeometry(emu->getLCDOffsetX(), emu->getLCDOffsetY(), emu->getLCDWidth(), emu->getLCDHeight());

	const char *who = emu->getDeviceName();
	if (strcmp(who, "Osaris") == 0) {
		// some cheap and cheerful placeholders
		int bitW = (emu->getDigitiserWidth() - emu->getLCDWidth()) / 2;
		int bitH = emu->getDigitiserHeight() / 5;
		int leftX = 0;
		int rightX = bitW + emu->getLCDWidth();
		(new QLabel("Word",       this))->setGeometry(leftX, bitH * 0, bitW, bitH);
		(new QLabel("Sheet",      this))->setGeometry(leftX, bitH * 1, bitW, bitH);
		(new QLabel("Data",       this))->setGeometry(leftX, bitH * 2, bitW, bitH);
		(new QLabel("Agenda",     this))->setGeometry(leftX, bitH * 3, bitW, bitH);
		(new QLabel("Extras",     this))->setGeometry(leftX, bitH * 4, bitW, bitH);
		(new QLabel("EPOC",       this))->setGeometry(rightX, bitH * 0, bitW, bitH);
		(new QLabel("Menu",       this))->setGeometry(rightX, bitH * 1, bitW, bitH);
		(new QLabel("Copy/Paste", this))->setGeometry(rightX, bitH * 2, bitW, bitH);
		(new QLabel("Zoom In",    this))->setGeometry(rightX, bitH * 3, bitW, bitH);
		(new QLabel("Zoom Out",   this))->setGeometry(rightX, bitH * 4, bitW, bitH);
	}
}

void PDAScreenWindow::updateScreen() {
	uint8_t *lines[1024];
	QImage img(emu->getLCDWidth(), emu->getLCDHeight(), QImage::Format_Grayscale8);
	for (int y = 0; y < img.height(); y++)
		lines[y] = img.scanLine(y);
	emu->readLCDIntoBuffer(lines);

	lcd->setPixmap(QPixmap::fromImage(std::move(img)));
}

static EpocKey resolveKey(int key) {
	switch (key) {
	case Qt::Key_Apostrophe: return EStdKeySingleQuote;
	case Qt::Key_Backspace: return EStdKeyBackspace;
	case Qt::Key_Escape: return EStdKeyEscape;
	case Qt::Key_Enter: return EStdKeyEnter;
	case Qt::Key_Return: return EStdKeyEnter;
	case Qt::Key_Alt: return EStdKeyMenu;
	case Qt::Key_Tab: return EStdKeyTab;
#ifdef Q_OS_MAC
	case Qt::Key_Meta: return EStdKeyLeftCtrl;
#else
	case Qt::Key_Control: return EStdKeyLeftCtrl;
#endif
	case Qt::Key_Down: return EStdKeyDownArrow;
	case Qt::Key_Period: return EStdKeyFullStop;
#ifdef Q_OS_MAC
	case Qt::Key_Control: return EStdKeyLeftFunc;
#else
	case Qt::Key_Meta: return EStdKeyLeftFunc;
#endif
	case Qt::Key_Shift: return EStdKeyLeftShift;
	case Qt::Key_Right: return EStdKeyRightArrow;
	case Qt::Key_Left: return EStdKeyLeftArrow;
	case Qt::Key_Comma: return EStdKeyComma;
	case Qt::Key_Up: return EStdKeyUpArrow;
	case Qt::Key_Space: return EStdKeySpace;
	}

	if (key >= '0' && key <= '9') return (EpocKey)key;
	if (key >= 'A' && key <= 'Z') return (EpocKey)key;
	return EStdKeyNull;
}


void PDAScreenWindow::keyPressEvent(QKeyEvent *event)
{
	EpocKey k = resolveKey(event->key());
	if (k != EStdKeyNull)
		emu->setKeyboardKey(k, true);
}

void PDAScreenWindow::keyReleaseEvent(QKeyEvent *event)
{
	EpocKey k = resolveKey(event->key());
	if (k != EStdKeyNull)
		emu->setKeyboardKey(k, false);
}


void PDAScreenWindow::mousePressEvent(QMouseEvent *event)
{
	emu->updateTouchInput(event->x(), event->y(), true);
}

void PDAScreenWindow::mouseReleaseEvent(QMouseEvent *event)
{
	emu->updateTouchInput(event->x(), event->y(), false);
}

void PDAScreenWindow::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::LeftButton)
		emu->updateTouchInput(event->x(), event->y(), true);
}
