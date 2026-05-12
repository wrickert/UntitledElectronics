#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// Use the internal HSI + PLL so the demo is resilient on clone boards or
// boards where the external crystal is not populated.
#define FUNCONF_USE_HSI           1
#define FUNCONF_USE_HSE           0
#define CLK_SOURCE_CH5XX          (0x100 | 0x40 | 5)
#define FUNCONF_SYSTEM_CORE_CLOCK (624 * 100 * 1000)

#define FUNCONF_DEBUG_HARDFAULT   0
#define FUNCONF_USE_CLK_SEC       0
#define FUNCONF_USE_DEBUGPRINTF   0

#endif
