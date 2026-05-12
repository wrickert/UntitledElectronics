#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// Clocking
//
// Now that the board is alive, default back to using the external 32MHz HSE
// crystal (more accurate, and required for reliable USB on most boards).
//
// If you need to eliminate crystals again for bring-up, build with:
//   EXTRA_CFLAGS=-DFUNCONF_CH585_HSI_ONLY=1
#ifndef FUNCONF_CH585_HSI_ONLY
#define FUNCONF_CH585_HSI_ONLY    0
#endif

#ifndef FUNCONF_CH585_USE_HSE
#define FUNCONF_CH585_USE_HSE     1
#endif

#if FUNCONF_CH585_HSI_ONLY
	#define FUNCONF_USE_HSI           1
	#define FUNCONF_USE_HSE           0
	// CLK_SOURCE_HSI_16MHz == (0x100 | 0x80) == 0x180 on CH584/CH585.
	#define CLK_SOURCE_CH5XX          (0x100 | 0x80)
	#define FUNCONF_SYSTEM_CORE_CLOCK 16000000
	#define FUNCONF_SYSTICK_USE_HCLK  1
#else
	#if FUNCONF_CH585_USE_HSE
		#define FUNCONF_USE_HSI           0
		#define FUNCONF_USE_HSE           1
		// CLK_SOURCE_HSE_PLL_62_4MHz == (0x300 | 0x40 | 5) == 0x345.
		#define CLK_SOURCE_CH5XX          (0x300 | 0x40 | 5)
		#define FUNCONF_SYSTEM_CORE_CLOCK (624 * 100 * 1000)     // keep in line with CLK_SOURCE_CH5XX
	#else
		// Fully internal (HSI+PLL). Useful if the HSE crystal is missing/broken.
		#define FUNCONF_USE_HSI           1
		#define FUNCONF_USE_HSE           0
		// CLK_SOURCE_HSI_PLL_62_4MHz == (0x100 | 0x40 | 5) == 0x145.
		#define CLK_SOURCE_CH5XX          (0x100 | 0x40 | 5)
		#define FUNCONF_SYSTEM_CORE_CLOCK (624 * 100 * 1000)     // keep in line with CLK_SOURCE_CH5XX
	#endif
	#define FUNCONF_SYSTICK_USE_HCLK  1  // Needed for sub-microsecond timing (e.g. SK6812/WS2812 LEDs).
#endif

// The SK6812 driver in this project does not assume a specific SysTick rate.
// If you want to force SysTick to HCLK/8 for more predictable bit timing,
// build with: EXTRA_CFLAGS=-DFUNCONF_SYSTICK_USE_HCLK=0

#define FUNCONF_DEBUG_HARDFAULT   0
#define FUNCONF_USE_CLK_SEC       0
#define FUNCONF_USE_DEBUGPRINTF   0

#ifndef USB_DEBUG_TELEMETRY
#define USB_DEBUG_TELEMETRY 0
#endif

#endif
