#include "ch32fun.h"

#define LED_PIN 6  // PB6

// NOP counts tuned for 62.4 MHz (1 cycle ≈ 16 ns).
// SK6812 tolerances: T0H 150-450 ns, T0L 750-1050 ns,
//                   T1H 450-750 ns, T1L 450-750 ns.
// Overhead per bit (branch + loop) ≈ 8 cycles; NOPs are exact via .rept.
static void __attribute__((noinline)) sk6812_send_byte(uint8_t byte)
{
	for (int i = 7; i >= 0; i--)
	{
		if (byte & (1 << i))
		{
			R32_PB_SET = (1 << LED_PIN);
			asm volatile(".rept 32\n nop\n .endr"); // T1H ~600 ns
			R32_PB_CLR = (1 << LED_PIN);
			asm volatile(".rept 28\n nop\n .endr"); // T1L ~600 ns (+ loop overhead)
		}
		else
		{
			R32_PB_SET = (1 << LED_PIN);
			asm volatile(".rept 13\n nop\n .endr"); // T0H ~300 ns
			R32_PB_CLR = (1 << LED_PIN);
			asm volatile(".rept 48\n nop\n .endr"); // T0L ~900 ns (+ loop overhead)
		}
	}
}

// SK6812 wire order: G, R, B
static void sk6812_send(uint8_t r, uint8_t g, uint8_t b)
{
	__disable_irq();
	sk6812_send_byte(g);
	sk6812_send_byte(r);
	sk6812_send_byte(b);
	__enable_irq();
}

int main()
{
	SystemInit();
	funPinMode(PB6, GPIO_ModeOut_PP_20mA);

	while (1)
	{
		sk6812_send(255, 0, 0);
		Delay_Ms(500);

		sk6812_send(0, 255, 0);
		Delay_Ms(500);

		sk6812_send(0, 0, 255);
		Delay_Ms(500);
	}
}
