#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QKeyEvent>
#include "../WindCore/decoder.h"

MainWindow::MainWindow(EmuBase *emu, QWidget *parent) :
    QMainWindow(parent),
	ui(new Ui::MainWindow),
	emu(emu)
{
    ui->setupUi(this);
	ui->logView->setMaximumBlockCount(1000);

	emu->setLogger([&](const char *str) {
		ui->logView->appendPlainText(str);
	});

    timer = new QTimer(this);
    timer->setInterval(1000/64);
    connect(timer, SIGNAL(timeout()), SLOT(execTimer()));

	updateScreen();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateScreen()
{
    ui->cycleCounter->setText(QString("Cycles: %1").arg(emu->currentCycles()));

	updateMemory();

	char flagDisplay[] = {
		(emu->getCPSR() & 0x80000000) ? 'N' : '-',
		(emu->getCPSR() & 0x40000000) ? 'Z' : '-',
		(emu->getCPSR() & 0x20000000) ? 'C' : '-',
		(emu->getCPSR() & 0x10000000) ? 'V' : '-',
		0
	};
	const char *modeName = "???";
	switch (emu->getCPSR() & 0x1F) {
	case 0x10: modeName = "User"; break;
	case 0x11: modeName = "FIQ"; break;
	case 0x12: modeName = "IRQ"; break;
	case 0x13: modeName = "Supervisor"; break;
	case 0x17: modeName = "Abort"; break;
	case 0x1B: modeName = "Undefined"; break;
	}

    ui->regsLabel->setText(
				QString("R0: %1 / R1: %2 / R2: %3 / R3: %4 / R4: %5 / R5: %6 / R6: %7 / R7: %8\nR8: %9 / R9: %10 / R10:%11 / R11:%12 / R12:%13 / SP: %14 / LR: %15 / PC: %16\n%17 / Mode: %18")
                .arg(emu->getGPR(0), 8, 16)
                .arg(emu->getGPR(1), 8, 16)
                .arg(emu->getGPR(2), 8, 16)
                .arg(emu->getGPR(3), 8, 16)
                .arg(emu->getGPR(4), 8, 16)
                .arg(emu->getGPR(5), 8, 16)
                .arg(emu->getGPR(6), 8, 16)
                .arg(emu->getGPR(7), 8, 16)
                .arg(emu->getGPR(8), 8, 16)
                .arg(emu->getGPR(9), 8, 16)
                .arg(emu->getGPR(10), 8, 16)
                .arg(emu->getGPR(11), 8, 16)
                .arg(emu->getGPR(12), 8, 16)
                .arg(emu->getGPR(13), 8, 16)
                .arg(emu->getGPR(14), 8, 16)
                .arg(emu->getGPR(15), 8, 16)
				.arg(flagDisplay)
				.arg(modeName)
                );

    // show a crude disassembly
    const int context = 8 * 4;
	uint32_t pc = emu->getGPR(15) - 8;
    uint32_t minCode = pc - context;
	if (minCode >= (UINT32_MAX - context))
        minCode = 0;
    uint32_t maxCode = pc + context;
	if (maxCode < context)
        maxCode = UINT32_MAX;

	QStringList codeLines;
	for (uint32_t addr = minCode; addr >= minCode && addr <= maxCode; addr += 4) {
		const char *prefix = (addr == pc) ? (emu->instructionReady() ? "==>" : "...") : "   ";
        struct ARMInstructionInfo info;
        char buffer[512];

		auto result = emu->readVirtual(addr, ARM710::V32);
		if (result.first.has_value()) {
			uint32_t opcode = result.first.value();
			ARMDecodeARM(opcode, &info);
			ARMDisassemble(&info, addr, buffer, sizeof(buffer));
			codeLines.append(QString("%1 %2 | %3 | %4").arg(prefix).arg(addr, 8, 16).arg(opcode, 8, 16).arg(buffer));
		}
    }
	ui->codeLabel->setText(codeLines.join('\n'));

    // now, the actual screen
	uint8_t *lines[1024];
	QImage img(emu->getLCDWidth(), emu->getLCDHeight(), QImage::Format_Grayscale8);
	for (int y = 0; y < img.height(); y++)
		lines[y] = img.scanLine(y);
	emu->readLCDIntoBuffer(lines);

	ui->screen->setPixmap(QPixmap::fromImage(std::move(img)));
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


void MainWindow::keyPressEvent(QKeyEvent *event)
{
	EpocKey k = resolveKey(event->key());
	if (k != EStdKeyNull)
		emu->setKeyboardKey(k, true);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
	EpocKey k = resolveKey(event->key());
	if (k != EStdKeyNull)
		emu->setKeyboardKey(k, false);
}




void MainWindow::on_startButton_clicked()
{
    timer->start();
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(true);
    ui->stepInsnButton->setEnabled(false);
    ui->stepTickButton->setEnabled(false);
}

void MainWindow::on_stopButton_clicked()
{
    timer->stop();
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->stepInsnButton->setEnabled(true);
    ui->stepTickButton->setEnabled(true);
}

void MainWindow::on_stepTickButton_clicked()
{
//    emu->executeUntil(emu->currentCycles() + (CLOCK_SPEED * 2));
	emu->executeUntil(emu->currentCycles() + 25000000);
	updateScreen();
}

void MainWindow::on_stepInsnButton_clicked()
{
    emu->executeUntil(emu->currentCycles() + 1);
    updateScreen();
}

void MainWindow::execTimer()
{
	if (emu) {
		emu->executeUntil(emu->currentCycles() + (emu->getClockSpeed() / 64));
		updateScreen();
	}
}

void MainWindow::on_addBreakButton_clicked()
{
	uint32_t addr = ui->breakpointAddress->text().toUInt(nullptr, 16);
	emu->breakpoints().insert(addr);
	updateBreakpointsList();
}

void MainWindow::on_removeBreakButton_clicked()
{
	uint32_t addr = ui->breakpointAddress->text().toUInt(nullptr, 16);
	emu->breakpoints().erase(addr);
	updateBreakpointsList();
}

void MainWindow::updateBreakpointsList()
{
	ui->breakpointsList->clear();
	for (uint32_t addr : emu->breakpoints()) {
		ui->breakpointsList->addItem(QString::number(addr, 16));
	}
}

void MainWindow::on_memoryViewAddress_textEdited(const QString &)
{
	updateMemory();
}

void MainWindow::updateMemory()
{
	uint32_t virtBase = ui->memoryViewAddress->text().toUInt(nullptr, 16) & ~0xFF;
	auto physBaseOpt = emu->virtToPhys(virtBase);
	auto physBase = physBaseOpt.value_or(0xFFFFFFFF);
	bool ok = physBaseOpt.has_value();
	if (ok && (virtBase != physBase))
		ui->physicalAddressLabel->setText(QStringLiteral("Physical: %1").arg(physBase, 8, 16, QLatin1Char('0')));

	uint8_t block[0x100];
	if (ok) {
		for (int i = 0; i < 0x100; i++) {
			block[i] = emu->readPhysical(physBase + i, ARM710::V8).value();
		}
	}

	QStringList output;
	for (int row = 0; row < 16; row++) {
		QString outLine;
		outLine.reserve(8 + 2 + (2 * 16) + 3 + 16);
		outLine.append(QStringLiteral("%1 |").arg(virtBase + (row * 16), 8, 16));
		for (int col = 0; col < 16; col++) {
			if (ok)
				outLine.append(QStringLiteral(" %1").arg(block[row*16+col], 2, 16, QLatin1Char('0')));
			else
				outLine.append(QStringLiteral(" ??"));
		}
		outLine.append(QStringLiteral(" | "));
		for (int col = 0; col < 16; col++) {
			uint8_t byte = block[row*16+col];
			if (!ok)
				outLine.append('?');
			else if (byte >= 0x20 && byte <= 0x7E)
				outLine.append(byte);
			else
				outLine.append('.');
		}
		output.append(outLine);
	}

	ui->memoryViewLabel->setText(output.join('\n'));
}

void MainWindow::on_memoryAdd1_clicked() { adjustMemoryAddress(1); }
void MainWindow::on_memoryAdd4_clicked() { adjustMemoryAddress(4); }
void MainWindow::on_memoryAdd10_clicked() { adjustMemoryAddress(0x10); }
void MainWindow::on_memoryAdd100_clicked() { adjustMemoryAddress(0x100); }
void MainWindow::on_memorySub1_clicked() { adjustMemoryAddress(-1); }
void MainWindow::on_memorySub4_clicked() { adjustMemoryAddress(-4); }
void MainWindow::on_memorySub10_clicked() { adjustMemoryAddress(-0x10); }
void MainWindow::on_memorySub100_clicked() { adjustMemoryAddress(-0x100); }

void MainWindow::adjustMemoryAddress(int offset) {
	uint32_t address = ui->memoryViewAddress->text().toUInt(nullptr, 16);
	address += offset;
	ui->memoryViewAddress->setText(QString("%1").arg(address, 8, 16, QLatin1Char('0')));
	updateMemory();
}

void MainWindow::on_writeByteButton_clicked()
{
	uint32_t address = ui->memoryViewAddress->text().toUInt(nullptr, 16);
	uint8_t value = (uint8_t)ui->memoryWriteValue->text().toUInt(nullptr, 16);
	emu->writeVirtual(value, address, ARM710::V8);
	updateMemory();
}

void MainWindow::on_writeDwordButton_clicked()
{
	uint32_t address = ui->memoryViewAddress->text().toUInt(nullptr, 16);
	uint32_t value = ui->memoryWriteValue->text().toUInt(nullptr, 16);
	emu->writeVirtual(value, address, ARM710::V32);
	updateMemory();
}
