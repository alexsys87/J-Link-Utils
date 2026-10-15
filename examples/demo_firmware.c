/*
 * Demo firmware module for examples/demo.xml.
 *
 * Add this file to any Cortex-M project, call demo_init() once, demo_tick_1khz()
 * from a 1 kHz timer interrupt (e.g. SysTick) and demo_poll() from the main loop.
 *
 * The variables must be at DEMO_VARS_ADDR (0x20001000), the address the XML
 * uses. Either reserve DEMO_VARS_SIZE bytes there in the linker script, or
 * place the struct anywhere and change the addresses in demo.xml (the offset
 * map is at the top of that file).
 */
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEMO_VARS_ADDR 0x20001000u

typedef struct {
    uint32_t counter;        /* 0x000  1 kHz tick counter */
    float    sine;           /* 0x004  PlotLines, LabelText, PlotTooltip */
    float    cosine;         /* 0x008  PlotLines, TextFormattedF */
    float    speed;          /* 0x00C  Gauge, 0..200 km/h */
    float    rpm;            /* 0x010  RPMIndicator, 0..8000 */
    float    level;          /* 0x014  Tank, 0..100 % */
    float    temperature;    /* 0x018  VerticalGauge, deg C */
    float    progress;       /* 0x01C  ProgressBar, 0..1 */
    float    setpoint;       /* 0x020  SliderFloat, ImputFloat, StyledKnob */
    float    audio;          /* 0x024  VUMeter, 0..100 */
    uint16_t combo;          /* 0x028  ComboBox */
    uint16_t radio;          /* 0x02A  RadioButton */
    uint16_t segment;        /* 0x02C  SegmentButton */
    uint16_t rotary;         /* 0x02E  RotarySwitch */
    uint16_t check;          /* 0x030  Checkbox (16-bit) */
    uint8_t  power;          /* 0x032  MechanicalSwitch2, Checkbox (AddrBool) */
    uint8_t  select;         /* 0x033  SelectableText (AddrBool) */
    uint16_t toggle;         /* 0x034  Button (toggle) */
    uint16_t momentary;      /* 0x036  Button (momentary) */
    uint16_t slider_int;     /* 0x038  SliderInt */
    uint16_t input_int;      /* 0x03A  ImputInt */
    uint16_t keypad;         /* 0x03C  Keypad, Button (write) */
    uint16_t display;        /* 0x03E  SevenSegment, GlossyDisplay, TextFormatted */
    uint16_t leds[8];        /* 0x040  Grid of LedIndicator */
    uint8_t  bits8;          /* 0x050  BitControl8Bit */
    uint8_t  pad0[3];
    uint32_t bits32;         /* 0x054  BitControl (32-bit) */
    int16_t  knob;           /* 0x058  NeumorphicKnob (S16) */
    uint16_t glossy;         /* 0x05A  GlossyButton (toggle) */
    float    vslider;        /* 0x05C  CustomVerticalSlider, 0..1 */
    char     lcd1[16];       /* 0x060  LCD_HD44780 line 1 */
    char     lcd2[16];       /* 0x070  LCD_HD44780 line 2 */
    float    eq[8];          /* 0x080  Equalizer */
    uint32_t capture_ready;  /* 0x0A0  DataLog DoneAddr */
    uint32_t hist_ready;     /* 0x0A4  Histogram DoneAddr */
    uint32_t pad1[2];
    float    capture[128];   /* 0x0B0  DataLog array */
    uint16_t histogram[16];  /* 0x2B0  Histogram array */
} demo_vars_t;

#define DEMO_VARS_SIZE sizeof(demo_vars_t)

/* The XML addresses depend on this layout. */
_Static_assert(offsetof(demo_vars_t, combo) == 0x028, "layout");
_Static_assert(offsetof(demo_vars_t, display) == 0x03E, "layout");
_Static_assert(offsetof(demo_vars_t, leds) == 0x040, "layout");
_Static_assert(offsetof(demo_vars_t, bits32) == 0x054, "layout");
_Static_assert(offsetof(demo_vars_t, vslider) == 0x05C, "layout");
_Static_assert(offsetof(demo_vars_t, lcd1) == 0x060, "layout");
_Static_assert(offsetof(demo_vars_t, eq) == 0x080, "layout");
_Static_assert(offsetof(demo_vars_t, capture_ready) == 0x0A0, "layout");
_Static_assert(offsetof(demo_vars_t, capture) == 0x0B0, "layout");
_Static_assert(offsetof(demo_vars_t, histogram) == 0x2B0, "layout");
_Static_assert(sizeof(demo_vars_t) == 0x2D0, "layout");

#define demo (*(volatile demo_vars_t *)DEMO_VARS_ADDR)

static const float kTwoPi = 6.28318531f;
static uint32_t s_captureIndex;
static uint32_t s_random = 12345u;

static uint32_t next_random(void)
{
    s_random = s_random * 1664525u + 1013904223u;
    return s_random >> 16;
}

void demo_init(void)
{
    memset((void *)&demo, 0, sizeof(demo_vars_t));
    demo.setpoint = 50.0f;
    demo.vslider = 0.5f;
    for (int i = 0; i < 8; ++i)
        demo.eq[i] = 50.0f;
    memcpy((void *)demo.lcd1, "J-Link Utils    ", 16);
}

/* Call at 1 kHz. */
void demo_tick_1khz(void)
{
    const uint32_t n = ++demo.counter;
    const float t = (float)n * 0.001f;

    demo.sine = 100.0f * sinf(kTwoPi * 1.0f * t);
    demo.cosine = 100.0f * cosf(kTwoPi * 1.0f * t);
    demo.speed = 100.0f + 90.0f * sinf(kTwoPi * 0.2f * t);
    demo.rpm = 4000.0f + 3500.0f * sinf(kTwoPi * 0.3f * t);
    demo.level = 50.0f + 45.0f * sinf(kTwoPi * 0.05f * t);
    demo.temperature = 40.0f + 50.0f * sinf(kTwoPi * 0.1f * t);
    demo.progress = fmodf(t * 0.1f, 1.0f);
    demo.audio = fabsf(demo.sine) * (0.7f + 0.3f * sinf(kTwoPi * 3.0f * t));
    demo.display = (uint16_t)((n / 100u) % 10000u);

    for (uint32_t i = 0; i < 8; ++i)
        demo.leds[i] = ((n / 200u) % 8u) == i;

    /* DataLog: fill the capture while the program has not taken the previous one. */
    if (demo.capture_ready == 0) {
        const float noise = (float)(next_random() % 200u) * 0.05f - 5.0f;
        demo.capture[s_captureIndex] = 80.0f * sinf(kTwoPi * 25.0f * t) + noise;
        if (++s_captureIndex == 128u) {
            s_captureIndex = 0;
            demo.capture_ready = 1;  /* cleared by J-Link Utils after the read */
        }
    }
}

/* Call from the main loop. */
void demo_poll(void)
{
    /* LCD line 2: uptime, padded with spaces to the 16 characters of the display. */
    char line[32];
    int len = snprintf(line, sizeof(line), "t=%lu s", (unsigned long)(demo.counter / 1000u));
    if (len < 0) len = 0;
    for (int i = len; i < 16; ++i)
        line[i] = ' ';
    memcpy((void *)demo.lcd2, line, 16);

    /* The equalizer follows the setpoint unless it is being edited. */
    for (int i = 0; i < 8; ++i)
        demo.eq[i] += (demo.setpoint - demo.eq[i]) * 0.001f * (float)(i + 1);

    /* Histogram: a new random distribution each time the program took the last one. */
    if (demo.hist_ready == 0) {
        for (int i = 0; i < 16; ++i)
            demo.histogram[i] = 0;
        for (int k = 0; k < 400; ++k) {
            const uint32_t sum = next_random() % 16u + next_random() % 16u;  /* triangle shape */
            demo.histogram[sum / 2u]++;
        }
        demo.hist_ready = 1;
    }
}
