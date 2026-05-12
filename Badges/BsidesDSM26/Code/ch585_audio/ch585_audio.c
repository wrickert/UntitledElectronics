#include "ch32fun.h"

#define AUDIO_OUTPUT_PIN PB0

#define NOTE_C5 523u
#define NOTE_E5 659u
#define NOTE_G5 784u
#define NOTE_A5 880u

typedef struct
{
	uint16_t hz;
	uint16_t duration_ms;
} note_event_t;

static const note_event_t g_test_pattern[] = {
	{ NOTE_C5, 160 },
	{ 0,       40  },
	{ NOTE_E5, 160 },
	{ 0,       40  },
	{ NOTE_G5, 160 },
	{ 0,       40  },
	{ NOTE_A5, 240 },
	{ 0,       280 },
};

static void audio_pin_init(void)
{
	funPinMode(AUDIO_OUTPUT_PIN, GPIO_ModeOut_PP_20mA);
	GPIO_ResetBits(AUDIO_OUTPUT_PIN);
}

static void play_tone(uint16_t hz, uint16_t duration_ms)
{
	if (!hz)
	{
		GPIO_ResetBits(AUDIO_OUTPUT_PIN);
		Delay_Ms(duration_ms);
		return;
	}

	uint16_t half_period_us = (uint16_t)(500000u / hz);
	uint32_t cycles = ((uint32_t)hz * duration_ms) / 1000u;

	if (!half_period_us)
	{
		half_period_us = 1;
	}

	if (!cycles)
	{
		cycles = 1;
	}

	for (uint32_t i = 0; i < cycles; i++)
	{
		GPIO_SetBits(AUDIO_OUTPUT_PIN);
		Delay_Us(half_period_us);
		GPIO_ResetBits(AUDIO_OUTPUT_PIN);
		Delay_Us(half_period_us);
	}
}

int main(void)
{
	SystemInit();
	funGpioInitAll();
	audio_pin_init();

	while (1)
	{
		for (unsigned i = 0; i < (sizeof(g_test_pattern) / sizeof(g_test_pattern[0])); i++)
		{
			play_tone(g_test_pattern[i].hz, g_test_pattern[i].duration_ms);
		}
	}
}
