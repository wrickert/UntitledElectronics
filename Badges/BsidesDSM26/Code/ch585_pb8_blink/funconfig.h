#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// Keep this simple and robust: use HSI+PLL so we don't hang if the HSE crystal
// isn't present/working on a clone board.
#define FUNCONF_USE_HSI           1
#define FUNCONF_USE_HSE           0
// For CH584/CH585, CLK_SOURCE_HSI_PLL_62_4MHz == (0x100 | 0x40 | 5) == 0x145.
#define CLK_SOURCE_CH5XX          (0x100 | 0x40 | 5)
#define FUNCONF_SYSTEM_CORE_CLOCK (624 * 100 * 1000)

#define FUNCONF_DEBUG_HARDFAULT   0
#define FUNCONF_USE_CLK_SEC       0
#define FUNCONF_USE_DEBUGPRINTF   0

#endif

