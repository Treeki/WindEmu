#include "etna.h"
#include "arm710.h"
#include <stdio.h>
#include <string.h>

enum EtnaReg {
    regUnk0 = 0,
    regUnk1 = 1,
    regUartIntStatus = 2,
    regUartIntMask = 3,
    regUartBaudRateLo8 = 4,
    regUartBaudRateHi4 = 5,
    regPcCdIntStatus = 6,
    regPcCdIntMask = 7,
    regIntClear = 8,
    regSktVarA0 = 9,
    regSktVarA1 = 0xA,
    regSktCtrl = 0xB,
    regWake1 = 0xC,
    regSktVarB0 = 0xD,
    regSktVarB1 = 0xE,
    regWake2 = 0xF
};

static const char *nameReg(uint32_t reg) {
    switch (reg) {
    case regUnk0: return "unk0";
    case regUnk1: return "unk1";
    case regUartIntStatus: return "UartIntStatus";
    case regUartIntMask: return "UartIntMask";
    case regUartBaudRateLo8: return "UartBaudRateLo8";
    case regUartBaudRateHi4: return "UartBaudRateHi4";
    case regPcCdIntStatus: return "PcCdIntStatus";
    case regPcCdIntMask: return "PcCdIntMask";
    case regIntClear: return "IntClear";
    case regSktVarA0: return "SktVarA0";
    case regSktVarA1: return "SktVarA1";
    case regSktCtrl: return "SktCtrl";
    case regWake1: return "wake1";
    case regSktVarB0: return "SktVarB0";
    case regSktVarB1: return "SktVarB1";
    case regWake2: return "wake2";
    }
    return nullptr;
}


Etna::Etna(ARM710 *owner) {
    this->owner = owner;

    for (int i = 0; i < 0x80; i++)
        prom[i] = 0;

    // some basic stuff to begin with
    // set up the Psion's unique ID
    prom[0x1B] = 0xDE;
    prom[0x1A] = 0xAD;
    prom[0x19] = 0xBE;
    prom[0x18] = 0xEF;

    // give ourselves a neat custom device name
    const char *key = "PSIONPSIONPSION";
    const char *name = "WindEmu!";
    prom[0x28] = strlen(name);
    if (prom[0x28] > 15)
        prom[0x28] = 15;
    for (int i = 0; i < prom[0x28]; i++)
        prom[0x29 + i] = name[i] ^ key[i];

    // calculate the checksum
    uint8_t chk = 0;
    for (int i = 0; i < 0x7F; i++)
        chk ^= prom[i];

    // EPOC is expecting 66
    prom[0x7F] = chk ^ 66;
}


uint32_t Etna::readReg8(uint32_t reg)
{
    if (!promReadActive)
        printf("ETNA readReg8: reg=%s @ pc=%08x,lr=%08x\n", nameReg(reg), owner->getGPR(15) - 4, owner->getGPR(14));
    switch (reg) {
    case regIntClear: return 0;
    case regSktVarA0: return 1; // will store some status flags
    case regSktVarA1: return 0; // will store some more status flags
    case regWake1: return wake1;
    case regWake2: return wake2;
    }
    return 0xFF;
}

uint32_t Etna::readReg32(uint32_t reg)
{
    // may be able to remove this, p. sure Etna is byte addressing only
    printf("ETNA readReg32: reg=%x\n", reg);
    return 0xFFFFFFFF;
}

void Etna::writeReg8(uint32_t reg, uint8_t value)
{
    if (!promReadActive)
        printf("ETNA writeReg8: reg=%s value=%02x @ pc=%08x,lr=%08x\n", nameReg(reg), value, owner->getGPR(15) - 4, owner->getGPR(14));
    switch (reg) {
    case regIntClear: pendingInterrupts &= ~value; break;
    case regWake1: wake1 = value; break;
    case regWake2: wake2 = value; break;
    }
}

void Etna::writeReg32(uint32_t reg, uint32_t value)
{
    // may be able to remove this, p. sure Etna is byte addressing only
    printf("ETNA writeReg32: reg=%x value=%08x\n", reg, value);
}

void Etna::setPromBit0High()
{
    // begin reading a word
    promReadAddress = 0;
    promReadValue = 0;
    promAddressBitsReceived = 0;
    promReadActive = true;
}

void Etna::setPromBit0Low()
{
    promReadActive = false;
}

void Etna::setPromBit1High()
{
    if (promAddressBitsReceived < 10) {
        // we're still receiving the address
        promReadAddress <<= 1;
        promReadAddress |= ((wake1 & 4) >> 2);
        if (++promAddressBitsReceived == 10) {
            // we can fetch the value now
            int addressInBytes = promReadAddress * 2;
            addressInBytes %= sizeof(prom);
            promReadValue = prom[addressInBytes] | (prom[addressInBytes + 1] << 8);
        }
    } else {
        wake1 &= ~8;
        if (promReadValue & 0x8000)
            wake1 |= 8;
        promReadValue <<= 1;
    }
}
