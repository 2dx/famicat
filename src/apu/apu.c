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

WORD dmc_frequency_lookup[] = { 0x1AC, 0x17C, 0x154, 0x140, 0x11E, 0x0FE, 0x0E2, 0x0D6, 0x0BE, 0x0A0, 0x08E, 0x080, 0x06A, 0x054, 0x048, 0x036 };

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
    apu.square1.num = 1;
    apu.square2.num = 2;
}

void APU_Step() {
    if (apu.square1.enabled) {
        APU_ClockSquare(&apu.square1);
    }
    if (apu.square2.enabled) {
        APU_ClockSquare(&apu.square2);
    }
    if (apu.triangle.enabled) {
        APU_ClockTriangle();
    }
    if (apu.noise.enabled) {
        APU_ClockNoise();
    }
    if (apu.dmc.enabled) {
        APU_ClockDMC();
    }
}

void APU_Push() {
    APU_MixOutput();
    APU_HipassStrong();
    APU_HipassWeak();
    Audio_AddSample(apu.sample);
}

void APU_HipassStrong() {
    SWORD sample = apu.sample;
    apu.hipass_strong += ((((int64_t)sample << 16) - (apu.hipass_strong >> 16 )) * HI_PASS_STRONG) >> 16;
    apu.sample = (SWORD)((int64_t)sample - (apu.hipass_strong >> 32));
}

void APU_HipassWeak() {
    SWORD sample = apu.sample;
    apu.hipass_weak += ((((int64_t)sample << 16) - (apu.hipass_weak >> 16 )) * HI_PASS_WEAK) >> 16;
    apu.sample = (SWORD)((int64_t)sample - (apu.hipass_weak >> 32));
}

void APU_MixOutput() {
    apu.sample = (apu.pulseOut[apu.square1.sample + apu.square2.sample] + apu.tndOut[3 * apu.triangle.sample + 2 * apu.noise.sample + apu.dmc.sample]) * 40000;
}

void APU_FrameSequencerStep() {
    apu.frame_tick++;
    if (!apu.frame_mode && !apu.irq_disable)
        apu.frame_irq_throw = 1;

    if (apu.frame_tick == 1 || apu.frame_tick == (4 + apu.frame_mode) - 1) {
        if (!apu.square1.halt && apu.square1.length > 0)
            apu.square1.length--;

        if (!apu.square2.halt && apu.square2.length > 0)
            apu.square2.length--;

        if (!apu.triangle.halt && apu.triangle.length > 0)
            apu.triangle.length--;

        if(!apu.noise.halt && apu.noise.length > 0)
            apu.noise.length--;

        APU_ClockSweep(&apu.square1);
        APU_ClockSweep(&apu.square2);
    }
    APU_ClockEnvelope(&apu.square1.envelope);
    APU_ClockEnvelope(&apu.square2.envelope);
    APU_ClockEnvelope(&apu.noise.envelope);
    APU_ClockLinearCounter();

    if (apu.frame_tick >= apu.frame_mode)
        apu.frame_tick++;
}

void APU_ClockSweep(Square *square) {
    square->sweep_counter--;
    if (square->sweep_counter == 0) {
        square->sweep_counter = square->sweep_period;
        if (square->sweep_enable && square->sweep_shift > 0) {
            DWORD tmp = square->period >> square->sweep_shift;
            if (square->sweep_negate){
                tmp = ~tmp;
                if (square->num == 2)
                    tmp += 1;
            }
            if ((square->period + tmp) < 0x800)
                square->period += tmp;
        }
    }

    if (square->sweep_reload) {
        square->sweep_reload = 0;
        square->sweep_counter = square->sweep_period;
    }
}

void APU_ClockEnvelope(Envelope *envelope) {
    if (envelope->reset) {
        envelope->counter = 15;
        envelope->divider_counter = envelope->period;
    }
    else {
        envelope->divider_counter--;
        if (envelope->divider_counter == 0) {
            envelope->divider_counter = envelope->period;
            if (envelope->counter == 0) {
                if (envelope->loop)
                    envelope->counter = 15;
            }
            else
                envelope->counter--;
        }
    }

    if (envelope->disable)
        envelope->volume = envelope->period - 1;
    else
        envelope->volume = envelope->counter;
}

void APU_ClockSquare(Square *square) {

    /*
    if (square->length > 0 && square->timer > 7) {
        if (square->timer_count == 0) {
            square->duty_count = (square->duty_count + 1) & 0x07;
            square->timer_count = (square->timer + 1) << 1;
        }

        if (square->timer < 8)
            square->sample = 0;
        else if (square_lookup[(square->duty_cycle * 8) + square->duty_count] == 1)
            square->sample = (SWORD)square->envelope.volume;
        else
            square->sample = 0;

        square->timer_count--;
    }
    else
        square->sample = 0;
    */
}

void APU_ClockTriangle() {
    if (apu.triangle.length > 0 && apu.triangle.counter > 0) {
        if (apu.triangle.timer_count == 0) {
            apu.triangle.lookup_counter = (apu.triangle.lookup_counter + 1) % 32;
            apu.triangle.timer_count = (apu.triangle.timer_period + 1) * 2;
        }
        apu.triangle.sample = triangle_lookup[apu.triangle.lookup_counter];
        apu.triangle.timer_count--;
    }
}

void APU_ClockLinearCounter() {
    if (apu.triangle.halt)
        apu.triangle.counter = apu.triangle.linear_reload;
    else if (apu.triangle.counter > 0)
        apu.triangle.counter--;

}

void APU_ClockNoise() {
    WORD feedback, tmp;

    if (apu.noise.length == 0) {
        apu.noise.sample = 0;
        return;
    }

    if ((apu.noise.shift & 0x01) == 0)
        apu.noise.sample = (SWORD)apu.noise.envelope.volume;
    else
        apu.noise.sample = 0;

    apu.noise.timer_count--;

    if (apu.noise.timer_count == 0) {
        if (apu.noise.mode)
            tmp = apu.noise.shift & 0x40 >> 6;
        else
            tmp = apu.noise.shift & 0x02 >> 1;

        feedback = (apu.noise.shift & 0x01) ^ tmp;
        apu.noise.shift = (apu.noise.shift >> 1) | (feedback << 14);
        apu.noise.timer_count = apu.noise.timer;
    }
}

void APU_ClockDMC() {

}

void APU_Write(WORD addr, BYTE val) {
    switch (addr & 0x00FF) {
        case 0x01:
            APU_WriteSquare1Control(val); break;
        case 0x02:
            APU_WriteSquare1Sweep(val); break;
        case 0x03:
            APU_WriteSquare1Low(val); break;
        case 0x04:
            APU_WriteSquare1High(val); break;
        case 0x05:
            APU_WriteSquare2Control(val); break;
        case 0x06:
            APU_WriteSquare2Sweep(val); break;
        case 0x07:
            APU_WriteSquare2Low(val); break;
        case 0x08:
            APU_WriteSquare2High(val); break;
        case 0x09:
            APU_WriteTriangleControl(val); break;
        case 0x0A:
            APU_WriteTriangleLow(val); break;
        case 0x0B:
            APU_WriteTriangleHigh(val); break;
        case 0x0C:
            APU_WriteNoiseBase(val); break;
        case 0x0E:
            APU_WriteNoisePeriod(val); break;
        case 0x0F:
            APU_WriteNoiseLength(val); break;
        case 0x10:
            APU_WriteDMCBase(val); break;
        case 0x11:
            APU_WriteDMCSample(val); break;
        case 0x12:
            APU_WriteDMCSampleAddress(val); break;
        case 0x13:
            APU_WriteDMCSampleLength(val); break;
        case 0x15:
            APU_WriteFlags1(val); break;
        case 0x17:
            APU_WriteFlags2(val); break;

    }
}

void APU_WriteSquare1Control(BYTE val) {
    /* ddle nnnn   duty, loop env/disable length, env disable, vol/env */
    apu.square1.duty_cycle = val >> 6;
    apu.square1.envelope.loop = apu.square1.halt = (val >> 5) & 0x01;
    apu.square1.envelope.disable = (val >> 4) & 0x01;
    apu.square1.envelope.volume = val & 0x0F;
    apu.square1.envelope.period = apu.square1.envelope.volume + 1;
    apu.square1.envelope.reset = 1;
}

void APU_WriteSquare1Sweep(BYTE val) {
    /* eppp nsss   enable sweep, period, negative, shift */
    apu.square1.sweep_enable = val >> 7;
    apu.square1.sweep_period = ((val >> 4) & 0x07) + 1;
    apu.square1.sweep_negate = (val >> 3) & 0x01;
    apu.square1.sweep_shift = val & 0x07;
    apu.square1.sweep_reload = 1;
}

void APU_WriteSquare1Low(BYTE val) {
    /* pppp pppp   period low */
    apu.square1.period = (apu.square1.period & 0xFF00) | val;
}

void APU_WriteSquare1High(BYTE val) {
    /* llll lppp   length index, period high */
    apu.square1.length = length_lookup[val >> 3];
    apu.square1.period = (apu.square1.period & 0x00FF) | ((val & 0x07) << 8);
    apu.square1.sequencer_reload = 1;
}

void APU_WriteSquare2Control(BYTE val) {
    /* ddle nnnn   duty, loop env/disable length, env disable, vol/env */
    apu.square2.duty_cycle = val >> 6;
    apu.square2.envelope.loop = apu.square2.halt = (val >> 5) & 0x01;
    apu.square2.envelope.disable = (val >> 4) & 0x01;
    apu.square2.envelope.volume = val & 0x0F;
    apu.square2.envelope.period = apu.square2.envelope.volume + 1;
    apu.square2.envelope.reset = 1;
}

void APU_WriteSquare2Sweep(BYTE val) {
    /* eppp nsss   enable sweep, period, negative, shift */
    apu.square2.sweep_enable = val >> 7;
    apu.square2.sweep_period = ((val >> 4) & 0x07) + 1;
    apu.square2.sweep_negate = (val >> 3) & 0x01;
    apu.square2.sweep_shift = val & 0x07;
    apu.square2.sweep_reload = 1;
}

void APU_WriteSquare2Low(BYTE val) {
    /* pppp pppp   period low */
    apu.square2.period = (apu.square2.period & 0xFF00) | val;
}

void APU_WriteSquare2High(BYTE val) {
    /* llll lppp   length index, period high */
    val = Memory_ReadByte(0x4003 + offset);
    apu.square2.length = length_lookup[val >> 3];
    apu.square2.period = (apu.square2.period & 0x00FF) | ((val & 0x07) << 8);
    apu.square2.sequencer_reload = 1;
}

void APU_WriteTriangleControl(BYTE val) {
    /* clll llll   control, linear counter load */
    apu.triangle.control = apu.triangle.length_halt = val >> 7;
    apu.triangle.linear_reload = val & 0x7F;
}

void APU_WriteTriangleLow(BYTE val) {
    /* pppp pppp   period low */
    apu.triangle.timer_period = val;
}

void APU_WriteTriangleHigh(BYTE val) {
    /* llll lppp   length index, period high */
    apu.triangle.length = length_lookup[val >> 3];
    apu.triangle.timer_period |= val & 0x07;
}

void APU_WriteNoiseBase(BYTE val) {
/* --le nnnn   loop env/disable length, env disable, vol/env period */
    apu.noise.envelope.loop = apu.noise.halt = val >> 5;
    apu.noise.envelope.disable = (val >> 4) & 0x01;
    apu.noise.envelope.volume = val & 0x0F;
    apu.noise.envelope.period = apu.noise.envelope.volume + 1;
}

void APU_WriteNoisePeriod(BYTE val) {
    /* s--- pppp   short mode, period index */
    apu.noise.mode = (val >> 7) & 0x01;
    apu.noise.period = noise_period_lookup[val & 0x0F];
}

void APU_WriteNoiseLength(BYTE val) {
    /* llll l---   length index */
    apu.noise.length = length_lookup[val >> 3];
}

void APU_WriteDMCBase(BYTE val) {
    /* il-- ffff   IRQ enable, loop, frequency index */
    apu.dmc.irq_enable = val >> 7;
    apu.dmc.loop_enable = (val >> 6) & 0x01;
    apu.dmc.frequency = dmc_frequency_lookup[val & 0x0F];
}

void APU_WriteDMCSample(BYTE val) {
    /* -ddd dddd   DAC */
    apu.dmc.sample = val;
}

void APU_WriteDMCSampleAddress(BYTE val) {
    /* aaaa aaaa   sample address */
    apu.dmc.sample_address = val * 0x40 + 0xC000;
}

void APU_WriteDMCSampleLength(BYTE val) {
    /* llll llll   sample length */
    apu.dmc.sample_length = val * 0x10 + 1;
}

void APU_WriteFlags1(BYTE val) {
    /* ---d nt21   length ctr enable: DMC, noise, triangle, pulse 2, 1 */
    apu.dmc.enabled = (val >> 4) & 0x01;
    apu.noise.enabled = (val >> 3) & 0x01;
    apu.triangle.enabled = (val >> 2) & 0x01;
    apu.square2.enabled = (val >> 1) & 0x01;
    apu.square1.enabled = val & 0x01;
}

void APU_WriteFlags2(BYTE val) {
    /* fd-- ----   5-frame cycle, disable frame interrupt */
    apu.frame_mode = val >> 7;
    apu.irq_disable = (val >> 6) & 0x01;
    if(apu.frame_mode)
        APU_FrameSequencerStep();
}

BYTE APU_Read() {
    BYTE val = 0;
    /* if-d nt21   DMC IRQ, frame IRQ, length counter statuses */
    val |= apu.dmc.irq_throw << 7;
    val |= apu.frame_irq_throw << 6;
    val |= apu.dmc.sample_counter << 4;
    val |= apu.noise.length << 3;
    val |= apu.triangle.length << 2;
    val |= apu.square2.length << 1;
    val |= apu.square1.length;
    return val;
}