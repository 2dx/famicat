#include "apu.h"

BYTE length_lookup[] = {
    0x0A, 0xFE,
    0x14, 0x02,
    0x28, 0x04,
    0x50, 0x06,
    0xA0, 0x08,
    0x3C, 0x0A,
    0x0E, 0x0C,
    0x1A, 0x0E,
    0x0C, 0x10,
    0x18, 0x12,
    0x30, 0x14,
    0x60, 0x16,
    0xC0, 0x18,
    0x48, 0x1A,
    0x10, 0x1C,
    0x20, 0x1E
};

WORD noise_period_lookup[] = { 0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0, 0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4 };

WORD dmc_period_lookup[] = { 0x1AC, 0x17C, 0x154, 0x140, 0x11E, 0x0FE, 0x0E2, 0x0D6, 0x0BE, 0x0A0, 0x08E, 0x080, 0x06A, 0x054, 0x048, 0x036 };

BYTE square_lookup[] = {
    0, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 0,
    1, 0, 0, 1, 1, 1, 1, 1
};

BYTE triangle_lookup[] = { 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

APU apu;

void APU_Init() {

    for (int i = 0; i < sizeof(apu.pulseOut)/sizeof(double); i++) {
        apu.pulseOut[i] = 95.52 / (8128.0 / i + 100);
    }
    for (int i = 0; i < sizeof(apu.tndOut)/sizeof(double); i++) {
        apu.tndOut[i] = 163.67 / (24329.0 / i + 100);
    }
}

double DAC_MixOutput() {
    return apu.pulseOut[apu.square1.sample + apu.square2.sample] + apu.tndOut[3 * apu.triangle.sample + 2 * apu.noise.sample + apu.dmc.sample];
}

void Clock_Square(Square square) {

}

void Clock_Triangle() {

}

void Clock_Noise() {

}

void Clock_DMC() {

}
