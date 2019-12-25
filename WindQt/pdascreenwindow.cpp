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
	} else if (strcmp(who, "Series 5mx") == 0) {
		int leftW = emu->getLCDOffsetX();
		int leftH = emu->getLCDHeight() / 5;
		(new QLabel("➡️",       this))->setGeometry(0, leftH * 0, leftW, leftH);
		(new QLabel("📄",       this))->setGeometry(0, leftH * 1, leftW, leftH);
		(new QLabel("📡",       this))->setGeometry(0, leftH * 2, leftW, leftH);
		(new QLabel("+",       this))->setGeometry(0, leftH * 3, leftW, leftH);
		(new QLabel("-",       this))->setGeometry(0, leftH * 4, leftW, leftH);

		int barX = 50;
		int barY = leftH * 5;
		int barW = (emu->getDigitiserWidth() - barX) / 8;
		int barH = emu->getDigitiserHeight() - emu->getLCDHeight();
		(new QLabel("System",   this))->setGeometry(0, barY, barX, barH);
		(new QLabel("Word",     this))->setGeometry(barX + barW * 0, barY, barW, barH);
		(new QLabel("Sheet",    this))->setGeometry(barX + barW * 1, barY, barW, barH);
		(new QLabel("Contacts", this))->setGeometry(barX + barW * 2, barY, barW, barH);
		(new QLabel("Agenda",   this))->setGeometry(barX + barW * 3, barY, barW, barH);
		(new QLabel("Email",    this))->setGeometry(barX + barW * 4, barY, barW, barH);
		(new QLabel("Calc",     this))->setGeometry(barX + barW * 5, barY, barW, barH);
		(new QLabel("Jotter",   this))->setGeometry(barX + barW * 6, barY, barW, barH);
		(new QLabel("Extras",   this))->setGeometry(barX + barW * 7, barY, barW, barH);
	}
}

void PDAScreenWindow::updateScreen() {
	uint8_t *lines[1024];
	QImage img(emu->getLCDWidth(), emu->getLCDHeight(), QImage::Format_Grayscale8);
	for (int y = 0; y < img.height(); y++)
		lines[y] = img.scanLine(y);
	emu->readLCDIntoBuffer(lines, false);

	lcd->setPixmap(QPixmap::fromImage(std::move(img)));
}

#ifdef Q_OS_MAC
static EpocKey resolveKey(int key, int vk) {
	// Although Cocoa/Carbon's virtual keycodes include
	// modifiers, Qt doesn't expose them through QKeyEvent...
	switch (key) {
	case Qt::Key_Control: return EStdKeyLeftFunc;
	case Qt::Key_Shift: return EStdKeyLeftShift;
	case Qt::Key_Alt: return EStdKeyMenu;
	case Qt::Key_Meta: return EStdKeyLeftCtrl;
	}

	// https://github.com/phracker/MacOSX-SDKs/blob/master/MacOSX10.6.sdk/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/Events.h#L182
	switch (vk) {
	case 0x00: return (EpocKey)'A';
	case 0x01: return (EpocKey)'S';
	case 0x02: return (EpocKey)'D';
	case 0x03: return (EpocKey)'F';
	case 0x04: return (EpocKey)'H';
	case 0x05: return (EpocKey)'G';
	case 0x06: return (EpocKey)'Z';
	case 0x07: return (EpocKey)'X';
	case 0x08: return (EpocKey)'C';
	case 0x09: return (EpocKey)'V';
	case 0x0B: return (EpocKey)'B';
	case 0x0C: return (EpocKey)'Q';
	case 0x0D: return (EpocKey)'W';
	case 0x0E: return (EpocKey)'E';
	case 0x0F: return (EpocKey)'R';

	case 0x10: return (EpocKey)'Y';
	case 0x11: return (EpocKey)'T';
	case 0x12: return (EpocKey)'1';
	case 0x13: return (EpocKey)'2';
	case 0x14: return (EpocKey)'3';
	case 0x15: return (EpocKey)'4';
	case 0x16: return (EpocKey)'6';
	case 0x17: return (EpocKey)'5';
	case 0x19: return (EpocKey)'9';
	case 0x1A: return (EpocKey)'7';
	case 0x1C: return (EpocKey)'8';
	case 0x1D: return (EpocKey)'0';
	case 0x1F: return (EpocKey)'O';

	case 0x20: return (EpocKey)'U';
	case 0x22: return (EpocKey)'I';
	case 0x23: return (EpocKey)'P';
	case 0x24: return EStdKeyEnter;
	case 0x25: return (EpocKey)'L';
	case 0x26: return (EpocKey)'J';
	case 0x27: return EStdKeySingleQuote;
	case 0x28: return (EpocKey)'K';
	case 0x2B: return EStdKeyComma;
	case 0x2D: return (EpocKey)'N';
	case 0x2E: return (EpocKey)'M';
	case 0x2F: return EStdKeyFullStop;

	case 0x30: return EStdKeyTab;
	case 0x31: return EStdKeySpace;
	case 0x33: return EStdKeyBackspace;
	case 0x35: return EStdKeyEscape;

	case 0x7B: return EStdKeyLeftArrow;
	case 0x7C: return EStdKeyRightArrow;
	case 0x7D: return EStdKeyDownArrow;
	case 0x7E: return EStdKeyUpArrow;
	}

	return EStdKeyNull;
}
#else
#error "Unsupported platform (for now! fix me in pdascreenwindow.cpp)"
static EpocKey resolveKey(int key) {
	// Placeholder, doesn't work for all keys
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
#endif


void PDAScreenWindow::keyPressEvent(QKeyEvent *event)
{
	emu->log("KeyPress: QtKey=%d nativeVirtualKey=%x nativeModifiers=%x", event->key(), event->nativeVirtualKey(), event->nativeModifiers());
	EpocKey k = resolveKey(event->key(), event->nativeVirtualKey());
	if (k != EStdKeyNull)
		emu->setKeyboardKey(k, true);
}

void PDAScreenWindow::keyReleaseEvent(QKeyEvent *event)
{
	EpocKey k = resolveKey(event->key(), event->nativeVirtualKey());
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
