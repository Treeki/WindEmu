#include "mainwindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include "../WindCore/clps7111.h"
#include "../WindCore/windermere.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	auto args = a.arguments();

	QString romFile;
	if (args.length() > 1)
		romFile = args.first();
	else
		romFile = QFileDialog::getOpenFileName(nullptr, "Select a ROM");
	if (romFile.isNull()) return 0;

	// what do we have?
	QFile f(romFile);
	f.open(QFile::ReadOnly);
	auto buffer = f.readAll();
	f.close();

	if (buffer.size() < 0x400000) {
		QMessageBox::critical(nullptr, "WindEmu", "Invalid ROM file!");
		return 0;
	}

	EmuBase *emu = nullptr;
	uint8_t *romData = (uint8_t *)buffer.data();

	// parse this ROM to learn what hardware it's for
	int variantFile = *((uint32_t *)&romData[0x80 + 0x4C]) & 0xFFFFFFF;
	if (variantFile < (buffer.size() - 8)) {
		int variantImg = *((uint32_t *)&romData[variantFile + 4]) & 0xFFFFFFF;
		if (variantImg < (buffer.size() - 0x70)) {
			int variant = *((uint32_t *)&romData[variantImg + 0x60]);

			if (variant == 0x7060001) {
				// 5mx ROM
				emu = new Windermere::Emulator;
			} else if (variant == 0x5040001) {
				// Osaris ROM
				emu = new CLPS7111::Emulator;
			} else {
				QMessageBox::critical(nullptr, "WindEmu", "Unrecognised ROM file!");
				return 0;
			}
		}
	}

	emu->loadROM(romData, buffer.size());
	MainWindow w(emu);
    w.show();

    return a.exec();
}
