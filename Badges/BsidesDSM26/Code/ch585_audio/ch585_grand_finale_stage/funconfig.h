#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// Clocking
//
// Default to the external 32MHz crystal + PLL for best USB stability.
// For bring-up on boards with no crystal, build with:
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
	#define CLK_SOURCE_CH5XX          (0x100 | 0x80)
	#define FUNCONF_SYSTEM_CORE_CLOCK 16000000
	#define FUNCONF_SYSTICK_USE_HCLK  1
#else
	#if FUNCONF_CH585_USE_HSE
		#define FUNCONF_USE_HSI           0
		#define FUNCONF_USE_HSE           1
		#define CLK_SOURCE_CH5XX          (0x300 | 0x40 | 5)
		#define FUNCONF_SYSTEM_CORE_CLOCK (624 * 100 * 1000)
	#else
		#define FUNCONF_USE_HSI           1
		#define FUNCONF_USE_HSE           0
		#define CLK_SOURCE_CH5XX          (0x100 | 0x40 | 5)
		#define FUNCONF_SYSTEM_CORE_CLOCK (624 * 100 * 1000)
	#endif
	#define FUNCONF_SYSTICK_USE_HCLK  1
#endif

#define FUNCONF_DEBUG_HARDFAULT   0
#define FUNCONF_USE_CLK_SEC       0
#define FUNCONF_USE_DEBUGPRINTF   0

#ifndef USB_DEBUG_TELEMETRY
#define USB_DEBUG_TELEMETRY 0
#endif

#endif
