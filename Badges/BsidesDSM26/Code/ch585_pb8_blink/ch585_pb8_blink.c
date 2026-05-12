#include "ch32fun.h"

// Simple sanity-test firmware for the WCH CH585 eval board:
// toggle PB8 at ~2Hz so we can confirm user code is running.

int main( void )
{
	SystemInit();
	funGpioInitAll(); // no-op on CH5xx, kept for consistency

	funPinMode( PB8, GPIO_ModeOut_PP_20mA );

	while( 1 )
	{
		funDigitalWrite( PB8, FUN_HIGH );
		Delay_Ms(250);
		funDigitalWrite( PB8, FUN_LOW );
		Delay_Ms(250);
	}
}

