#include "ch32fun.h"
#include "fsusb.h"

// Status LED for this (custom) board.
// The WCH eval board typically uses a different pin; keep this project pinned to PB3.
#define LED PB3

// Set to 1 if your LED is wired from VDD -> resistor -> LED -> PB3 (active-low sink).
// Set to 0 if your LED is wired from PB3 -> resistor -> LED -> GND (active-high source).
#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW 1
#endif
#define LED_ON  (LED_ACTIVE_LOW ? FUN_LOW : FUN_HIGH)
#define LED_OFF (LED_ACTIVE_LOW ? FUN_HIGH : FUN_LOW)

// SK6812 / WS2812-style status LED data pin.
// Default to PB6 (common on the CH585 dev boards / many badge layouts).
// Override at build time if your board uses a different pin, e.g.:
//   EXTRA_CFLAGS="-DSTATUS_SK6812_PIN=PA6"
#ifndef STATUS_SK6812_PIN
#define STATUS_SK6812_PIN PB6
#endif

// If set, never generate SK6812 waveforms; just hold the data pin low.
// Useful to prove the LED isn't being (accidentally) driven by firmware.
//   make flash EXTRA_CFLAGS="-DSK6812_DISABLE=1"
#ifndef SK6812_DISABLE
#define SK6812_DISABLE 0
#endif

#define AUDIO_PWM_PIN            PB0
#define AUDIO_PWM_CLOCK_DIV      1u
#define AUDIO_SAMPLE_RATE        12000u
#define AUDIO_SAMPLE_TICKS       (FUNCONF_SYSTEM_CORE_CLOCK / AUDIO_SAMPLE_RATE)
#define AUDIO_PWM_MIDPOINT       128u
#define AUDIO_TAG_CUE_NOTES      4u

#define NOTE_C5 523u
#define NOTE_E5 659u
#define NOTE_G5 784u
#define NOTE_A5 880u

#define MS_TO_SAMPLES(ms) ((uint16_t)(((uint32_t)(ms) * AUDIO_SAMPLE_RATE) / 1000u))

// If nonzero, include a long high-time profile that is very likely to register
// as "1" even if our baseline timing is too short.
#ifndef SK6812_FORCE_ONES
#define SK6812_FORCE_ONES 0
#endif

// Some dev boards don't have the PB22 "boot" button wired, and enabling the
// GPIO interrupt can cause an immediate jump back into ISP ROM. Disable by
// default; you can re-enable if you have that button fitted.
#ifndef ENABLE_PB22_BOOT_BUTTON
#define ENABLE_PB22_BOOT_BUTTON 0
#endif

// --- SK6812 (WS2812-compatible) single-pixel status LED on PB6 ---
//
// This is a tiny bit-banged driver for a single SK6812 RGBW (common on dev boards).
// It briefly disables interrupts while sending (4 bytes = 32 bits), then releases.
//
// Timing is done with fixed NOP counts. This avoids relying on SysTick behavior
// on CH5xx, which varies by clock configuration.
// Assumes ~62.4MHz core clock (HSE+PLL or HSI+PLL).

// Note: these counts include overhead from GPIO_Set/Reset memory writes, so
// the numbers are smaller than a pure cycle calculation might suggest.
//
// You can select a preset with -DSK6812_PROFILE=N to avoid long EXTRA_CFLAGS:
//   0: baseline (default)
//   1: shorter highs (helps if LED looks "all white"/all 1s)
//   2: longer highs (helps if LED is mostly off / all 0s)
//   3: mid highs (between 0 and 1)
//   4: very long highs (debug: slow/forgiving timing)
//   5: ultra-short 0H (debug: if everything looks white)
#ifndef SK6812_PROFILE
#define SK6812_PROFILE 0
#endif

#if SK6812_PROFILE == 1
	#ifndef SK6812_T0H_NOPS
	#define SK6812_T0H_NOPS 2
	#endif
	#ifndef SK6812_T0L_NOPS
	#define SK6812_T0L_NOPS 24
	#endif
	#ifndef SK6812_T1H_NOPS
	#define SK6812_T1H_NOPS 6
	#endif
	#ifndef SK6812_T1L_NOPS
	#define SK6812_T1L_NOPS 14
	#endif
#elif SK6812_PROFILE == 3
	#ifndef SK6812_T0H_NOPS
	#define SK6812_T0H_NOPS 4
	#endif
	#ifndef SK6812_T0L_NOPS
	#define SK6812_T0L_NOPS 22
	#endif
	#ifndef SK6812_T1H_NOPS
	#define SK6812_T1H_NOPS 10
	#endif
	#ifndef SK6812_T1L_NOPS
	#define SK6812_T1L_NOPS 12
	#endif
#elif SK6812_PROFILE == 4
	// Deliberately very slow/forgiving. If this doesn't work, it's likely not a
	// timing issue but a signal integrity / logic level issue.
	#ifndef SK6812_T0H_NOPS
	#define SK6812_T0H_NOPS 30
	#endif
	#ifndef SK6812_T0L_NOPS
	#define SK6812_T0L_NOPS 80
	#endif
	#ifndef SK6812_T1H_NOPS
	#define SK6812_T1H_NOPS 70
	#endif
	#ifndef SK6812_T1L_NOPS
	#define SK6812_T1L_NOPS 40
	#endif
#elif SK6812_PROFILE == 5
	// For cases where "0" is being read as "1" (solid white).
	// Make 0-high as short as possible, keep 1-high moderate.
	#ifndef SK6812_T0H_NOPS
	#define SK6812_T0H_NOPS 0
	#endif
	#ifndef SK6812_T0L_NOPS
	#define SK6812_T0L_NOPS 40
	#endif
	#ifndef SK6812_T1H_NOPS
	#define SK6812_T1H_NOPS 12
	#endif
	#ifndef SK6812_T1L_NOPS
	#define SK6812_T1L_NOPS 28
	#endif
#elif SK6812_FORCE_ONES
	// Not a real WS2812 timing profile; just aims to make "1" unmistakably high.
	// Use only for diagnosis: if this lights the LED, the normal profiles have
	// too-short high times.
	#ifndef SK6812_T0H_NOPS
	#define SK6812_T0H_NOPS 6
	#endif
	#ifndef SK6812_T0L_NOPS
	#define SK6812_T0L_NOPS 20
	#endif
	#ifndef SK6812_T1H_NOPS
	#define SK6812_T1H_NOPS 80
	#endif
	#ifndef SK6812_T1L_NOPS
	#define SK6812_T1L_NOPS 6
	#endif
#elif SK6812_PROFILE == 2
	#ifndef SK6812_T0H_NOPS
	#define SK6812_T0H_NOPS 8
	#endif
	#ifndef SK6812_T0L_NOPS
	#define SK6812_T0L_NOPS 18
	#endif
	#ifndef SK6812_T1H_NOPS
	#define SK6812_T1H_NOPS 22
	#endif
	#ifndef SK6812_T1L_NOPS
	#define SK6812_T1L_NOPS 10
	#endif
#else
	#ifndef SK6812_T0H_NOPS
	#define SK6812_T0H_NOPS 6
	#endif
	#ifndef SK6812_T0L_NOPS
	#define SK6812_T0L_NOPS 20
	#endif
	#ifndef SK6812_T1H_NOPS
	#define SK6812_T1H_NOPS 16
	#endif
	#ifndef SK6812_T1L_NOPS
	#define SK6812_T1L_NOPS 10
	#endif
#endif

// ADD_N_NOPS uses stringification, so we need a 2-step macro to expand numeric macros.
#define SK6812__ADD_N_NOPS(n) asm volatile( ".rept " #n "\nc.nop\n.endr" );
#define SK6812_ADD_N_NOPS(n)  SK6812__ADD_N_NOPS(n)

#if !SK6812_DISABLE
// Important: do not branch between SET and CLR. If we branch there, the "0"
// pulse becomes too long and some pixels read everything as '1' (solid white).
__attribute__((noinline))
static void sk6812_send_bit_0(void)
{
	GPIO_SetBits(STATUS_SK6812_PIN);
	SK6812_ADD_N_NOPS(SK6812_T0H_NOPS);
	GPIO_ResetBits(STATUS_SK6812_PIN);
	SK6812_ADD_N_NOPS(SK6812_T0L_NOPS);
}

__attribute__((noinline))
static void sk6812_send_bit_1(void)
{
	GPIO_SetBits(STATUS_SK6812_PIN);
	SK6812_ADD_N_NOPS(SK6812_T1H_NOPS);
	GPIO_ResetBits(STATUS_SK6812_PIN);
	SK6812_ADD_N_NOPS(SK6812_T1L_NOPS);
}

// If set, reverse bit order within each byte (LSB-first). WS2812/SK6812 are
// normally MSB-first, but this is a quick diagnostic toggle.
#ifndef SK6812_LSB_FIRST
#define SK6812_LSB_FIRST 0
#endif

static void sk6812_send_byte(uint8_t b)
{
	for(int i = 0; i < 8; i++)
	{
#if SK6812_LSB_FIRST
		if( b & 0x01 ) sk6812_send_bit_1();
		else           sk6812_send_bit_0();
		b >>= 1;
#else
		if( b & 0x80 ) sk6812_send_bit_1();
		else           sk6812_send_bit_0();
		b <<= 1;
#endif
	}
}

#ifndef SK6812_IS_RGBW
#define SK6812_IS_RGBW 1
#endif

// Optional: drive the RGBW "W" channel too (0..255). Useful for RGBW pixels.
// Must be defined before sk6812_set_rgb().
#ifndef SK6812_W_BRIGHT
#define SK6812_W_BRIGHT 0
#endif

static void sk6812_set_grbw(uint8_t g, uint8_t r, uint8_t b, uint8_t w)
{
	uint8_t irq_was_enabled = __isenabled_irq();
	__disable_irq();

	// Common SK6812 RGBW order is GRBW. Some boards actually fit WS2812 (RGB),
	// in which case only send 3 bytes to avoid "stuck white" weirdness.
	sk6812_send_byte(g);
	sk6812_send_byte(r);
	sk6812_send_byte(b);
#if SK6812_IS_RGBW
	sk6812_send_byte(w);
#else
	(void)w;
#endif

	if( irq_was_enabled ) __enable_irq();

	// Latch (reset) time: keep data line low for >= 80us.
	GPIO_ResetBits(STATUS_SK6812_PIN);
	// Use a very conservative delay here. If the timebase is off, too-short reset
	// will prevent the LED from ever latching new data (appears "stuck").
	Delay_Ms(1);
}

static void sk6812_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	// Our low-level function is GRBW for SK6812 RGBW.
	sk6812_set_grbw(g, r, b, SK6812_W_BRIGHT);
}
#else
// Stubbed: hold data pin low; never emit a waveform.
static inline void sk6812_set_grbw(uint8_t g, uint8_t r, uint8_t b, uint8_t w)
{
	(void)g; (void)r; (void)b; (void)w;
	GPIO_ResetBits(STATUS_SK6812_PIN);
}
static inline void sk6812_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	(void)r; (void)g; (void)b;
	GPIO_ResetBits(STATUS_SK6812_PIN);
}
#endif

#define R8_NFC_CMD                    (*(vu8*)0x4000E000)
#define  RB_NFC_ANTENNA_ON            0x4
#define R8_NFC_STATUS                 (*(vu8*)0x4000E001)
#define R16_NFC_INTF_STATUS           (*(vu16*)0x4000E004)
#define R16_NFC_RXTX_LEN              (*(vu16*)0x4000E008)
#define R16_NFC_FIFO                  (*(vu16*)0x4000E00C)
#define R16_NFC_TMR                   (*(vu16*)0x4000E010)
#define R32_NFC_DRV                   (*(vu32*)0x4000E014)

#define PLUSMIN_7(t,r)                ((t >> 3) == (r >> 3))
#define PLUSMIN_15(t,r)               ((t >> 4) == (r >> 4))

#define RB_TMR_FREQ_13_56             0x20

#define USB_DATA_BUF_SIZE                        512 // max nfca frame size is 256, add a bit of headroom (could go up to at least 16384)
#define NFCA_DATA_BUF_SIZE                       256 // we use FSDI 8, so max frame size is 256
#define NFCA_PCD_MAX_SEND_NUM                    NFCA_DATA_BUF_SIZE
#define NFCA_PCD_MAX_RECV_NUM                    NFCA_DATA_BUF_SIZE
#define NFCA_MAX_PARITY_NUM                      NFCA_PCD_MAX_RECV_NUM
#define NFCA_PCD_CHAINING_BUFFERS                4
#define NFCA_PCD_LPCD_THRESHOLD_PERMIL           5
#define NFCA_PCD_LPCD_THRESHOLD_MAX_LIMIT_PERMIL 20
#define NFCA_PCD_WAIT_MAX_MS                     1000
#define NFCA_PCD_TICKS_PER_MILLISECOND           1725

// Some antenna layouts don't produce a big enough LPCD delta for our ADC-based
// "presence" check, which prevents us from ever issuing REQA/WUPA. For bring-up,
// you can bypass LPCD and always attempt REQA:
//   make flash EXTRA_CFLAGS="-DNFCA_BYPASS_LPCD=1"
#ifndef NFCA_BYPASS_LPCD
#define NFCA_BYPASS_LPCD 1
#endif

// Force NFC output drive level (field strength) for bring-up.
// 0 = auto (default), else set to a NFCA_PCD_DRV_CTRL_LEVELx value.
// Example: `-DNFCA_FORCE_DRV=NFCA_PCD_DRV_CTRL_LEVEL0`
#ifndef NFCA_FORCE_DRV
#define NFCA_FORCE_DRV 0
#endif

#define CMD_REQA               0x26
#define CMD_WUPA               0x52
#define CMD_ANTICOLL1          0x93
#define CMD_ANTICOLL2          0x95
#define CMD_ANTICOLL3          0x97
#define CMD_RATS               0xE0
#define CMD_HALT               0x50
#define CMD_IBLOCK             0x02
#define CMD_WTX                0xF2
#define CMD_RBLOCK             0xA0
#define CMD_T2T_GET_VERSION    0x60
#define CMD_T2T_READ           0x30

#define PCD_ANTICOLL_OVER_TIME 10
#define PCD_SELECT_OVER_TIME   10
#define PCD_APDU_OVER_TIME     100
#define PCD_T2T_OVER_TIME      20

#define USB_PKT_NTAG_VERSION   0xA1
#define USB_PKT_NTAG_PAGE_DATA 0xA2
#define USB_PKT_NTAG_DONE      0xA3
#define USB_PKT_DEBUG          0xA4

#define NTAG21X_VERSION_LEN    8
#define NTAG_T2T_READ_BYTES    16
#define NTAG215_TOTAL_PAGES    135

typedef struct {
	uint16_t duration_samples;
	uint16_t hz;
} audio_note_t;

static const int8_t g_sine32[32] = {
	0, 25, 49, 71, 90, 106, 117, 125,
	127, 125, 117, 106, 90, 71, 49, 25,
	0, -25, -49, -71, -90, -106, -117, -125,
	-127, -125, -117, -106, -90, -71, -49, -25,
};

static const audio_note_t g_tag_cue[AUDIO_TAG_CUE_NOTES] = {
	{ MS_TO_SAMPLES(90), NOTE_C5 },
	{ MS_TO_SAMPLES(90), NOTE_E5 },
	{ MS_TO_SAMPLES(90), NOTE_G5 },
	{ MS_TO_SAMPLES(140), NOTE_A5 },
};

#define ISO14443A_CHECK_BCC(B)    ((B[0] ^ B[1] ^ B[2] ^ B[3]) == B[4])
#define NFCA_PCD_SET_OUT_DRV(lvl) (R32_NFC_DRV = ((R32_NFC_DRV & 0x9ff) | lvl))

__attribute__((aligned(4))) static volatile uint8_t gs_usb_data_buf[USB_DATA_BUF_SIZE];
__attribute__((aligned(4))) static uint16_t gs_nfca_data_buf[NFCA_DATA_BUF_SIZE];
__attribute__((aligned(4))) static uint8_t g_nfca_parity_buf[NFCA_MAX_PARITY_NUM];
__attribute__((aligned(4))) static uint8_t g_nfca_pcd_send_buf[((NFCA_PCD_MAX_SEND_NUM + 3) & 0xfffc)];
__attribute__((aligned(4))) static uint8_t g_nfca_pcd_recv_buf[((NFCA_PCD_MAX_RECV_NUM + 3) & 0xfffc)];
__attribute__((aligned(4))) static uint8_t g_nfca_pcd_recv_buf_chain[sizeof(g_nfca_pcd_recv_buf) *NFCA_PCD_CHAINING_BUFFERS];
__attribute__((aligned(4))) static uint16_t gs_lpcd_adc_filter_buf[8];
__attribute__((aligned(4))) static uint8_t gs_picc_uid[10];

static uint16_t gs_lpcd_adc_base_value;
static uint16_t g_nfca_pcd_recv_buf_len;
static volatile uint32_t g_nfca_pcd_recv_bits;
static uint16_t g_nfca_pcd_recv_word_idx;
static uint16_t g_nfca_pcd_send_fifo_bytes;
static uint16_t g_nfca_pcd_send_total_bytes;
static uint8_t g_nfca_pcd_buf_offset;
static uint8_t g_nfca_pcd_intf_mode;
static uint8_t g_nfca_pcd_comm_status;
static volatile uint8_t apdu_iblock_number = 0x02;
static uint8_t g_last_ntag_version[NTAG21X_VERSION_LEN];

// Debug breadcrumbs for NFC bring-up (viewable via SK6812 colors).
static volatile uint16_t g_last_nfc_intf_status = 0;
static volatile uint8_t  g_last_nfc_comm_status = 0;
static volatile uint32_t g_last_nfc_recv_bits = 0;
static volatile uint16_t g_last_nfc_ant_adc = 0;
static volatile uint16_t g_last_lpcd_base = 0;
static volatile uint8_t  g_last_lpcd_hit = 0;
static uint8_t g_audio_pwm_initialized = 0;

static inline uint32_t now_ticks(void)
{
	return SysTick->CNTL;
}

// Periodically emit NFC debug telemetry over USB so the host script can print it.
//   make flash EXTRA_CFLAGS="-DUSB_DEBUG_TELEMETRY=1"
// Default on for bring-up; you can disable with `-DUSB_DEBUG_TELEMETRY=0`.
#ifndef USB_DEBUG_TELEMETRY
#define USB_DEBUG_TELEMETRY 1
#endif

typedef enum {
	NFCA_PCD_CONTROLLER_STATE_FREE = 0,
	NFCA_PCD_CONTROLLER_STATE_SENDING,
	NFCA_PCD_CONTROLLER_STATE_RECEIVING,
	NFCA_PCD_CONTROLLER_STATE_COLLISION,
	NFCA_PCD_CONTROLLER_STATE_OVERTIME,
	NFCA_PCD_CONTROLLER_STATE_DONE,
	NFCA_PCD_CONTROLLER_STATE_ERR,
} nfca_pcd_controller_state_t;

typedef enum {
	NFCA_PCD_DRV_CTRL_LEVEL0 = (0x00 << 13),
	NFCA_PCD_DRV_CTRL_LEVEL1 = (0x01 << 13),
	NFCA_PCD_DRV_CTRL_LEVEL2 = (0x02 << 13),
	NFCA_PCD_DRV_CTRL_LEVEL3 = (0x03 << 13),
} NFCA_PCD_DRV_CTRL_Def;

typedef enum {
	NFCA_PCD_REC_MODE_NONE   = 0,
	NFCA_PCD_REC_MODE_NORMAL = 1,
	NFCA_PCD_REC_MODE_COLI   = 0x10,
} NFCA_PCD_REC_MODE_Def;

typedef enum {
	SampleFreq_8 = 0,
	SampleFreq_8_or_4,
	SampleFreq_5_33_or_2_67,
	SampleFreq_4_or_2,
} ADC_SampClkTypeDef;

typedef enum {
	ADC_PGA_1_4 = 0,
	ADC_PGA_1_2,
	ADC_PGA_0,
	ADC_PGA_2,
	ADC_PGA_2_ = 0x10,
	ADC_PGA_4,
	ADC_PGA_8,
	ADC_PGA_16,
} ADC_SignalPGATypeDef;

typedef enum {
	CH_EXTIN_0 = 0,
	CH_EXTIN_1,
	CH_EXTIN_2,
	CH_EXTIN_3,
	CH_EXTIN_4,
	CH_EXTIN_5,
	CH_EXTIN_6,
	CH_EXTIN_7,
	CH_EXTIN_8,
	CH_EXTIN_9,
	CH_EXTIN_10,
	CH_EXTIN_11,
	CH_EXTIN_12,
	CH_EXTIN_13,

	CH_INTE_VBAT = 14,
	CH_INTE_VTEMP = 15,
	CH_INTE_NFC = 16,
} ADC_SingleChannelTypeDef;

enum {
	PCD_NO_ERROR              = 0,
	PCD_COMMUNICATE_ERROR     = 1,
	PCD_ODD_PARITY_ERROR      = 2,
	PCD_UNKNOWN_ERROR         = 3,

	PCD_OVERTIME_ERROR        = 0x0100,
	PCD_FRAME_ERROR           = 0x0101,
	PCD_BCC_ERROR             = 0x0102,
	PCD_CRC_ERROR             = 0x0103,
	PCD_AUTH_ERROR            = 0x0104,
	PCD_DECRYPT_ERROR         = 0x0105,
	PCD_VALUE_BLOCK_INVALID   = 0x0106,
};

typedef enum {
	High_Level = 0,
	Low_Level,
} PWMX_PolarTypeDef;

typedef enum {
	PWM_Times_1 = 0,
	PWM_Times_4,
	PWM_Times_8,
	PWM_Times_16,
} PWM_RepeatTsTypeDef;

typedef enum {
	CAP_NULL = 0,
	Edge_To_Edge,
	FallEdge_To_FallEdge,
	RiseEdge_To_RiseEdge,
} CapModeTypeDef;

static const uint8_t byteParityBitsTable[256] = {
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

#ifndef SK6812_BRIGHT
#define SK6812_BRIGHT 32
#endif

// Optional: drive the RGBW "W" channel too (0..255). Useful for RGBW pixels.
#ifndef SK6812_W_BRIGHT
#define SK6812_W_BRIGHT 0
#endif

// Optional diagnostic: blast full white continuously (ignores RGB cycling).
//   make flash EXTRA_CFLAGS="-DSK6812_TEST_WHITE=1"
#ifndef SK6812_TEST_WHITE
#define SK6812_TEST_WHITE 0
#endif

#ifndef SK6812_CYCLE_RGB
#define SK6812_CYCLE_RGB 1
#endif

// When debugging NFC bring-up, the always-on RGB heartbeat can mask state changes.
// Build with `-DSK6812_CYCLE_RGB=0` to let NFC status colors be visible.

static int16_t sine32_wave(uint32_t phase)
{
	return g_sine32[phase >> 27];
}

static uint32_t audio_hz_to_phase_step(uint16_t hz)
{
	if (!hz) return 0;
	return (uint32_t)((((uint64_t)hz) << 32) / AUDIO_SAMPLE_RATE);
}

static void audio_init(void)
{
	if (g_audio_pwm_initialized) return;

	funPinMode(AUDIO_PWM_PIN, GPIO_ModeOut_PP_20mA);
	R8_PWM_OUT_EN &= (uint8_t)~RB_PWM6_OUT_EN;
	R8_PWM_CONFIG &= (uint8_t)~(RB_PWM_CYC_MOD | RB_PWM_CYCLE_SEL | RB_PWM6_7_STAG_EN);
	R8_PWM_CONFIG |= RB_PWM_CYCLE_SEL;
	R8_PWM_POLAR &= (uint8_t)~RB_PWM6_POLAR;
	R8_PWM_CLOCK_DIV = AUDIO_PWM_CLOCK_DIV;
	R8_PWM6_DATA = AUDIO_PWM_MIDPOINT;
	R8_PWM_OUT_EN |= RB_PWM6_OUT_EN;

	g_audio_pwm_initialized = 1;
}

static void audio_play_tag_cue(const uint8_t *uid, uint8_t uid_len)
{
	uint32_t phase = 0;
	int32_t filter_state = 0;
	uint16_t semitone_offset = uid_len ? (uid[uid_len - 1] & 0x03u) : 0u;
	uint32_t next_tick = now_ticks();

	audio_init();

	for (unsigned note_idx = 0; note_idx < AUDIO_TAG_CUE_NOTES; note_idx++)
	{
		uint16_t shifted_hz = g_tag_cue[note_idx].hz;
		uint32_t phase_step;

		if (semitone_offset)
		{
			shifted_hz = (uint16_t)(shifted_hz + (shifted_hz / 16u) * semitone_offset);
		}

		phase_step = audio_hz_to_phase_step(shifted_hz);

		for (uint16_t i = 0; i < g_tag_cue[note_idx].duration_samples; i++)
		{
			int32_t sample;

			while ((int32_t)(now_ticks() - next_tick) < 0) { }
			next_tick += AUDIO_SAMPLE_TICKS;

			phase += phase_step;
			sample = (sine32_wave(phase) * 104) / 128;
			filter_state += ((sample - filter_state) >> 2);
			sample = filter_state + AUDIO_PWM_MIDPOINT;
			if (sample < 0) sample = 0;
			if (sample > 255) sample = 255;
			R8_PWM6_DATA = (uint8_t)sample;
		}
	}

	R8_PWM6_DATA = AUDIO_PWM_MIDPOINT;
}

static void tag_feedback(uint8_t uid_len, uint16_t atqa, uint8_t sak)
{
	uint8_t r = (uid_len ? ((gs_picc_uid[0] * 5u) & 0x1Fu) : 8u) + 4u;
	uint8_t g = ((uint8_t)(atqa) & 0x1Fu) + 4u;
	uint8_t b = (sak & 0x1Fu) + 4u;

	if (r > SK6812_BRIGHT) r = SK6812_BRIGHT;
	if (g > SK6812_BRIGHT) g = SK6812_BRIGHT;
	if (b > SK6812_BRIGHT) b = SK6812_BRIGHT;

	sk6812_set_rgb(r, g, b);
	audio_play_tag_cue(gs_picc_uid, uid_len);
	sk6812_set_rgb(0, 0, 0);
}

__HIGH_CODE
void blink(int n) {
	for(int i = n-1; i >= 0; i--) {
		funDigitalWrite( LED, LED_ON );
		// RGB pulse on the SK6812 (if fitted).
#if !SK6812_DISABLE
		switch( i % 3 )
		{
			case 0: sk6812_set_rgb(SK6812_BRIGHT, 0, 0); break; // red
			case 1: sk6812_set_rgb(0, SK6812_BRIGHT, 0); break; // green
			default: sk6812_set_rgb(0, 0, SK6812_BRIGHT); break; // blue
		}
#endif
		Delay_Ms(33);
		funDigitalWrite( LED, LED_OFF );
#if !SK6812_DISABLE
		sk6812_set_rgb(0, 0, 0);
#endif
		if(i) Delay_Ms(33);
	}
}

__INTERRUPT
void GPIOB_IRQHandler() {
	int status = R16_PB_INT_IF;
	R16_PB_INT_IF = status; // acknowledge

	if(status & PB8) { // PB22 interrupt comes in on PB8 if RB_PIN_INTX is set
		// Only jump if the button is actually held (active-low).
		if( funDigitalRead(PB22) ) return;
		USBFSReset();
		blink(2);
		jump_isprom();
	}
}

void GPIOSetup() {
	funPinMode( LED, GPIO_CFGLR_OUT_2Mhz_PP );
	funDigitalWrite( LED, LED_OFF );
	funPinMode( STATUS_SK6812_PIN, GPIO_ModeOut_PP_20mA );
	funPinMode( AUDIO_PWM_PIN, GPIO_ModeOut_PP_20mA );
	GPIO_ResetBits( STATUS_SK6812_PIN );
	audio_init();
	// Always drive DIN low at boot; avoids random latch from a floating line.
	sk6812_set_grbw(0, 0, 0, 0);

#if ENABLE_PB22_BOOT_BUTTON
	funPinMode( PB22, GPIO_CFGLR_IN_PU ); // input, pullup; button to GND

	R16_PIN_ALTERNATE |= RB_PIN_INTX; // set PB8 interrupt to PB22
	R16_PB_INT_MODE |= (PB8 & ~PB); // edge mode
	funDigitalWrite(PB22, FUN_LOW); // falling edge

	NVIC_EnableIRQ(GPIOB_IRQn);
	R16_PB_INT_IF = (PB8 & ~PB); // reset PB8/PB22 flag
	R16_PB_INT_EN |= (PB8 & ~PB); // enable PB8/PB22 interrupt
#endif

	SYS_SAFE_ACCESS(
		R8_SLP_WAKE_CTRL |= (RB_WAKE_EV_MODE | RB_SLP_GPIO_WAKE);
		R8_SLP_POWER_CTRL &= ~(RB_WAKE_DLY_MOD);
		R8_SLP_POWER_CTRL |= 0x00; // 0x00: long delay, 0x01: short delay (short doesn't work here)
	);
}

int HandleSetupCustom( struct _USBState * ctx, int setup_code) {
	return 0;
}

int HandleInRequest( struct _USBState * ctx, int endp, uint8_t * data, int len ) {
	return 0;
}

__HIGH_CODE
void HandleDataOut( struct _USBState * ctx, int endp, uint8_t * data, int len ) {
	// this is actually the data rx handler
	if ( endp == 0 ) {
		// this is in the hsusb.c default handler
		ctx->USBFS_SetupReqLen = 0; // To ACK
	}
	else if( endp == USB_EP_RX ) {
		if(len == 4 && ((uint32_t*)data)[0] == 0x010001a2) {
			USBFSReset();
			blink(2);
			jump_isprom();
		}
		else if( ((uint16_t*)gs_usb_data_buf)[0] == 0 ) {
			((uint16_t*)gs_usb_data_buf)[0] = len;
			for(int i = 0; i < len; i++) {
				gs_usb_data_buf[2 + i] = data[i];
			}
		}
		else {
			// previous usb buffer not consumed yet, what should we do?
		}

		ctx->USBFS_Endp_Busy[USB_EP_RX] = 0;
	}
}

__INTERRUPT
__HIGH_CODE
void NFC_IRQHandler(void) {
	// Read the interrupt status and write it back to clear the flags.
	uint16_t intf_status = R16_NFC_INTF_STATUS;
	R16_NFC_INTF_STATUS = intf_status;
	g_last_nfc_intf_status = intf_status;

	// --- State 1: TRANSMITTING ---
	if (g_nfca_pcd_comm_status == 1) {
		// Check if the TX FIFO is ready for more data (TX FIFO Empty flag)
		if ((intf_status & 8) && (g_nfca_pcd_send_fifo_bytes < g_nfca_pcd_send_total_bytes)) {
			// Refill the FIFO with up to 5 words
			for (int i = 0; i < 5; i++) {
				R16_NFC_FIFO = gs_nfca_data_buf[g_nfca_pcd_send_fifo_bytes++];
				if (g_nfca_pcd_send_fifo_bytes >= g_nfca_pcd_send_total_bytes) {
					break;
				}
			}
		}

		// Check if the transmission has completed (TX Complete flag)
		if (intf_status & 1) {

			// If mode is Transceive, switch to receive mode
			R8_NFC_CMD &= ~(0x01); // Clear the Start TX bit
			if (g_nfca_pcd_intf_mode) {
				g_nfca_pcd_recv_bits = 0;
				g_nfca_pcd_recv_word_idx = 0;

				// Set control register based on mode (e.g., enable parity)
				R8_NFC_STATUS = (g_nfca_pcd_intf_mode & 0x10) ? 0x36 : 0x26;

				g_nfca_pcd_comm_status = 2; // Change state to Receiving
				R8_NFC_CMD |= 0x18; // Enable RX
			} 
			// If mode is Transmit-only, the operation is complete
			else {
				R8_NFC_STATUS = 0;
				g_nfca_pcd_comm_status = 5; // Status: Success
			}
		}
	}
	// --- State 2: RECEIVING ---
	else if (g_nfca_pcd_comm_status == 2) {
		// Check if there is data in the RX FIFO (FIFO Not Empty flag)
		if (intf_status & 4) { // Note: OV flag is used for Not Empty
			// Drain up to 5 words from the FIFO
			for (int i = 0; i < 5; i++) {
				gs_nfca_data_buf[g_nfca_pcd_recv_word_idx++] = R16_NFC_FIFO;
			}
		}

		// Check if the reception has completed (RX Complete flag)
		if (intf_status & 2) {
			g_nfca_pcd_recv_bits = R16_NFC_RXTX_LEN;
			g_last_nfc_recv_bits = g_nfca_pcd_recv_bits;
			uint16_t received_words = (g_nfca_pcd_recv_bits + 15) / 16;

			// Drain any final words left in the FIFO
			if (g_nfca_pcd_recv_word_idx < received_words) {
				uint16_t words_to_drain = received_words - g_nfca_pcd_recv_word_idx;
				for (int i = 0; i < words_to_drain; i++) {
					gs_nfca_data_buf[g_nfca_pcd_recv_word_idx++] = R16_NFC_FIFO;
				}
			}
			g_nfca_pcd_comm_status = 5; // Status: Success
			g_last_nfc_comm_status = g_nfca_pcd_comm_status;
		}
		// Check for error flags
		else if (intf_status & 0x10) {
			g_nfca_pcd_recv_bits = R16_NFC_RXTX_LEN;
			// Drain FIFO on error
			uint16_t received_words = (g_nfca_pcd_recv_bits + 15) / 16;
			if (g_nfca_pcd_recv_word_idx < received_words) {
				uint16_t words_to_drain = received_words - g_nfca_pcd_recv_word_idx;
				for (int i = 0; i < words_to_drain; i++) {
					gs_nfca_data_buf[g_nfca_pcd_recv_word_idx++] = R16_NFC_FIFO;
				}
			}
			g_nfca_pcd_comm_status = 3; // Status: Parity Error
			g_last_nfc_comm_status = g_nfca_pcd_comm_status;
		}
		else if (intf_status & 0x20) {
			g_nfca_pcd_comm_status = 4; // Status: CRC Error
			g_last_nfc_comm_status = g_nfca_pcd_comm_status;
		}
	}
	// --- Any Other State: ERROR ---
	else {
		g_nfca_pcd_comm_status = 6; // Status: General Error
		g_last_nfc_comm_status = g_nfca_pcd_comm_status;
	}
}

uint16_t sys_get_vdd(void) {
	uint8_t  sensor, channel, config, tkey_cfg;
	uint16_t adc_data;

	tkey_cfg = R8_TKEY_CFG;
	sensor = R8_TEM_SENSOR;
	channel = R8_ADC_CHANNEL;
	config = R8_ADC_CFG;

	R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
	R8_ADC_CHANNEL = CH_INTE_VBAT;
	R8_ADC_CFG = RB_ADC_POWER_ON | RB_ADC_BUF_EN | (0 << 4);
	R8_ADC_CONVERT &= ~RB_ADC_PGA_GAIN2;
	R8_ADC_CONVERT |= (3 << 4);
	R8_ADC_CONVERT |= RB_ADC_START;
	while (R8_ADC_CONVERT & RB_ADC_START);
	adc_data = R16_ADC_DATA;

	R8_TEM_SENSOR = sensor;
	R8_ADC_CHANNEL = channel;
	R8_ADC_CFG = config;
	R8_TKEY_CFG = tkey_cfg;
	return (adc_data);
}

int ADC_VoltConverSignalPGA_MINUS_12dB(uint16_t adc_data) {
	return (((int)adc_data*1050+256)/512 - 3*1050);
}

void nfca_init() {
	funPinMode( PA7, GPIO_CFGLR_IN_FLOAT ); // NFC CTR
	R32_PIN_IN_DIS |= PA7; // disable PA7 digital input

	funPinMode( (PB8 | PB9 | PB16 | PB17), GPIO_CFGLR_IN_FLOAT );
	R32_PIN_IN_DIS |= (((PB8 | PB9) & ~PB) << 16); // disable PB8 PB9 digital input
	R16_PIN_CONFIG |= (((PB16 | PB17) & ~PB) >> 8); // gpio alt func
}

// NFCA PCD functions
void nfca_pcd_start(void) {
	R8_NFC_CMD = 0x24;
	R32_NFC_DRV &= 0xe7ff;
	NVIC_ClearPendingIRQ(NFC_IRQn);
	NVIC_EnableIRQ(NFC_IRQn);
}

void nfca_pcd_stop(void) {
	R8_NFC_STATUS = 0;
	R8_NFC_CMD = 0;
	NVIC_DisableIRQ(NFC_IRQn);
}

void nfca_pcd_wait_ms(uint32_t milliseconds) {
	uint32_t ticks = milliseconds * NFCA_PCD_TICKS_PER_MILLISECOND;

	if (ticks > 0xFFFF) {
		ticks = 0xFFFF;
	}

	R16_NFC_TMR = (uint16_t)ticks;
}

void nfca_pcd_wait_us(uint32_t microseconds) {
	uint64_t ticks = ((uint64_t)microseconds * NFCA_PCD_TICKS_PER_MILLISECOND) / 1000;

	if (ticks > 0xFFFF) {
		ticks = 0xFFFF;
	}

	R16_NFC_TMR = (uint16_t)ticks;
}

void nfca_pcd_lpcd_calibration() {
	uint8_t sensor, channel, config, tkey_cfg;
	uint32_t adc_all;
	uint16_t adc_max, adc_min, adc_value;

	adc_all = 0;
	adc_max = 0;
	adc_min = 0xffff;

	nfca_pcd_start();

	Delay_Ms(200);

	tkey_cfg = R8_TKEY_CFG;
	sensor = R8_TEM_SENSOR;
	channel = R8_ADC_CHANNEL;
	config = R8_ADC_CFG;

	R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
	R8_ADC_CHANNEL = CH_INTE_NFC;
	R8_ADC_CFG = RB_ADC_POWER_ON | RB_ADC_BUF_EN | (SampleFreq_8_or_4 << 6) | (ADC_PGA_1_4 << 4);
	R8_ADC_CONVERT &= ~RB_ADC_PGA_GAIN2;
	R8_ADC_CONVERT &= ~(3 << 4);

	Delay_Ms(100);

	for(int i = 0; i < 10; i++) {
		R8_ADC_CONVERT |= RB_ADC_START;
		while (R8_ADC_CONVERT & (RB_ADC_START | RB_ADC_EOC_X));
		adc_value = R16_ADC_DATA;

		if(adc_value > adc_max) {
			adc_max = adc_value;
		}

		if(adc_value < adc_min) {
			adc_min = adc_value;
		}
		adc_all = adc_all + adc_value;
	}

	R8_TEM_SENSOR = sensor;
	if(channel == CH_INTE_NFC) {
		R8_ADC_CHANNEL = CH_INTE_VBAT;
	}
	else {
		R8_ADC_CHANNEL = channel;
	}
	R8_ADC_CFG = config;
	R8_TKEY_CFG = tkey_cfg;

	adc_all = adc_all - adc_max - adc_min;

	gs_lpcd_adc_base_value = adc_all >> 3;

	for(int i = 0; i < 8; i++) {
		gs_lpcd_adc_filter_buf[i] = gs_lpcd_adc_base_value;
	}

	nfca_pcd_stop();
}

uint16_t nfca_adc_get_ant_signal(void) {
	uint8_t  sensor, channel, config, tkey_cfg;
	uint16_t adc_data;
	uint32_t adc_data_all;

	tkey_cfg = R8_TKEY_CFG;
	sensor = R8_TEM_SENSOR;
	channel = R8_ADC_CHANNEL;
	config = R8_ADC_CFG;

	R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
	R8_ADC_CHANNEL = CH_INTE_NFC;
	R8_ADC_CFG = RB_ADC_POWER_ON | RB_ADC_BUF_EN | (SampleFreq_8_or_4 << 6) | (ADC_PGA_1_4 << 4);
	R8_ADC_CONVERT &= ~RB_ADC_PGA_GAIN2;
	R8_ADC_CONVERT &= ~(3 << 4);

	adc_data_all = 0;

	for(uint8_t i = 0; i < 2; i++) {
		R8_ADC_CONVERT |= RB_ADC_START;
		while (R8_ADC_CONVERT & (RB_ADC_START | RB_ADC_EOC_X));
		adc_data_all = adc_data_all + R16_ADC_DATA;
	}

	adc_data = adc_data_all / 2;
	
	if(channel == CH_INTE_NFC) {
		R8_ADC_CHANNEL = CH_INTE_VBAT;
	}
	else {
		R8_ADC_CHANNEL = channel;
	}

	R8_TEM_SENSOR = sensor;
	R8_ADC_CFG = config;
	R8_TKEY_CFG = tkey_cfg;
	return (adc_data);
}

uint32_t nfca_pcd_separate_recv_data(uint16_t data_buf[], uint16_t num_bits, uint8_t buf_offset, uint8_t recv_buf[], uint8_t parity_buf[], uint16_t output_len) {
	// Current position in the bitstream, combining word index and bit offset.
	uint32_t current_total_bit_pos = buf_offset;
	uint32_t bits_remaining = num_bits;
	int32_t bytes_processed;

	// The main loop processes one output byte per iteration.
	for (bytes_processed = 0; bytes_processed < output_len; ++bytes_processed) {
		// Stop if there aren't enough bits left for a full 9-bit packet.
		if (bits_remaining < 9) {
			break;
		}

		// Determine which word(s) in data_buf hold the next 9 bits.
		uint32_t word_index = current_total_bit_pos / 16;
		uint32_t bit_pos_in_word = current_total_bit_pos % 16;

		// Combine two consecutive 16-bit words into a 32-bit integer to safely
		// extract 9 bits that may cross a word boundary. This is a clean C
		// equivalent of the assembly's more complex boundary-checking logic.
		uint32_t combined_words = data_buf[word_index];
		if (bit_pos_in_word > (16 - 9)) {
			// The 9-bit chunk crosses from data_buf[word_index] to data_buf[word_index + 1].
			combined_words |= (uint32_t)data_buf[word_index + 1] << 16;
		}

		// Shift to the correct starting bit and mask to extract the 9-bit packet.
		// The mask 0x1FF is binary 1 1111 1111.
		uint16_t nine_bit_packet = (combined_words >> bit_pos_in_word) & 0x1FF;

		// The lower 8 bits are the data byte.
		recv_buf[bytes_processed] = (uint8_t)(nine_bit_packet & 0xFF);

		// The 9th bit is the parity bit.
		parity_buf[bytes_processed] = (uint8_t)((nine_bit_packet >> 8) & 0x01);

		// Advance the bit position by 9 for the next iteration.
		current_total_bit_pos += 9;
		bits_remaining -= 9;
	}

	return bytes_processed;
}

nfca_pcd_controller_state_t nfca_pcd_get_comm_status() {
	if(g_nfca_pcd_comm_status > 2) {
		if(g_nfca_pcd_intf_mode) {
			R8_NFC_CMD &= 0xef;
			g_nfca_pcd_recv_buf_len = nfca_pcd_separate_recv_data(gs_nfca_data_buf, g_nfca_pcd_recv_bits, g_nfca_pcd_buf_offset, g_nfca_pcd_recv_buf, g_nfca_parity_buf, NFCA_PCD_MAX_RECV_NUM);
		}
		return g_nfca_pcd_comm_status;
	}
	return 0;
}

uint32_t nfca_pcd_prepare_send_data(uint8_t send_buf[], uint16_t num_bits, uint8_t parity_buf[], uint16_t data_buf[], uint32_t output_len) {
	if (num_bits <= 3) {
		return 0;
	}

	uint16_t num_bytes = (num_bits + 7) / 8;
	if (num_bytes > output_len) {
		num_bytes = output_len;
	}

	for (uint16_t i = 0; i < num_bytes; ++i) {
		data_buf[i] = (uint16_t)send_buf[i] | (parity_buf[i] ? 0x0100 : 0);
	}

	return (num_bytes * 9) - ((num_bytes * 8) - num_bits);
}

uint8_t nfca_pcd_comm(uint16_t data_bits_num, NFCA_PCD_REC_MODE_Def mode, uint8_t offset) {
	// Check for a minimum number of bits.
	if (data_bits_num <= 3) {
		g_nfca_pcd_intf_mode = 0;
		g_nfca_pcd_comm_status = 6; // Status: Error/Invalid Param
		return 1;
	}

	// Prepare the raw data buffers into a single packed buffer for the FIFO.
	int prepared_byte_len = nfca_pcd_prepare_send_data(g_nfca_pcd_send_buf, data_bits_num, g_nfca_parity_buf, gs_nfca_data_buf, NFCA_DATA_BUF_SIZE);

	// --- Configure NFC Hardware ---
	R8_NFC_STATUS = 0; // Clear control register

	// Clear command bits 0 and 4 (CMD_IDLE and CMD_MF_AUTH)
	R8_NFC_CMD &= ~(0x11);

	// Store current communication parameters in global state
	g_nfca_pcd_intf_mode = mode;
	g_nfca_pcd_buf_offset = offset;

	// Set the total number of bytes to be transmitted
	R16_NFC_RXTX_LEN = (uint16_t)prepared_byte_len;

	// Calculate the number of 9-bit words to transmit.
	// The hardware appears to send data in 9-bit frames (8 data + 1 parity).
	int num_words = (prepared_byte_len + 8) / 9;

	// --- Load Data into FIFO ---
	if (prepared_byte_len > 72) {
		// For large transfers, load the first 8 words (16 bytes) into the FIFO.
		// The hardware might use DMA for the rest of the data.
		for (int i = 0; i < 8; i++) {
			R16_NFC_FIFO = gs_nfca_data_buf[i];
		}
		g_nfca_pcd_send_fifo_bytes = 8;
	}
	else {
		// For smaller transfers, load all the words into the FIFO.
		for (int i = 0; i < num_words; i++) {
			R16_NFC_FIFO = gs_nfca_data_buf[i];
		}
		g_nfca_pcd_send_fifo_bytes = num_words;
	}

	// --- Start Transmission ---
	g_nfca_pcd_send_total_bytes = num_words;
	g_nfca_pcd_comm_status = 1; // Status: Transmitting

	// Set registers to indicate 9-bit transmission format
	R16_NFC_INTF_STATUS = 9;
	R8_NFC_STATUS = 9;

	R8_NFC_CMD |= (mode ? 9 : 1);

	return 0; // Success
}

nfca_pcd_controller_state_t nfca_pcd_wait_comm_end(void) {
	nfca_pcd_controller_state_t status;
	uint32_t overtimes;

	overtimes = 0;

	while (1) {
		status = nfca_pcd_get_comm_status();
		if ((status != NFCA_PCD_CONTROLLER_STATE_FREE) || (overtimes > (NFCA_PCD_WAIT_MAX_MS * 10))) {
			break;
		}

		Delay_Us(100);
		overtimes++;
	}

	return status;
}

uint16_t nfca_pcd_lpcd_adc_filter_buf_add(uint16_t lpcd_adc) {
	uint32_t lpcd_adc_all = 0;
	for(uint8_t i = 0; i < 7; i++) {
		gs_lpcd_adc_filter_buf[i] = gs_lpcd_adc_filter_buf[i + 1];
		lpcd_adc_all = lpcd_adc_all + gs_lpcd_adc_filter_buf[i];
	}
	gs_lpcd_adc_filter_buf[7] = lpcd_adc;
	lpcd_adc_all = lpcd_adc_all + gs_lpcd_adc_filter_buf[7];
	lpcd_adc_all = (lpcd_adc_all >> 3);
	return (uint16_t)lpcd_adc_all;
}

__HIGH_CODE
uint8_t nfca_pcd_lpcd_check(void) {
	uint32_t adc_value_diff;
	uint16_t adc_value;
	uint8_t res = 0;

	adc_value = nfca_adc_get_ant_signal();
	g_last_nfc_ant_adc = adc_value;
	if(adc_value > gs_lpcd_adc_base_value) {
		adc_value_diff = adc_value - gs_lpcd_adc_base_value;
	}
	else {
		adc_value_diff = gs_lpcd_adc_base_value - adc_value;
	}
	adc_value_diff = (adc_value_diff * 1000) / gs_lpcd_adc_base_value;

	if((adc_value > gs_lpcd_adc_base_value) && (adc_value_diff > NFCA_PCD_LPCD_THRESHOLD_MAX_LIMIT_PERMIL)) {
		adc_value = ((uint32_t)gs_lpcd_adc_base_value * (1000 + NFCA_PCD_LPCD_THRESHOLD_MAX_LIMIT_PERMIL) / 1000);
	}
	else {
		if(adc_value_diff >= NFCA_PCD_LPCD_THRESHOLD_PERMIL) {
			res = 1;
		}

		if(adc_value < gs_lpcd_adc_base_value) {
			adc_value = gs_lpcd_adc_base_value - 1;
		}
	}

	gs_lpcd_adc_base_value = nfca_pcd_lpcd_adc_filter_buf_add(adc_value);
	g_last_lpcd_base = gs_lpcd_adc_base_value;
	g_last_lpcd_hit = res;
	return res;
}

// ISO 14443-3A functions
__HIGH_CODE
uint16_t ISO14443_CRCA(uint8_t *buf, int len) {
	uint8_t *data = buf;
	uint16_t crc = 0x6363;
	uint8_t ch;
	while (len--) {
		ch = *data++ ^ crc;
		ch = ch ^ (ch << 4);
		crc = (crc >> 8) ^ (ch << 8) ^ (ch << 3) ^ (ch >> 4);
	}
	return crc;
}

__HIGH_CODE
uint16_t ISO14443AAppendCRCA(void *buf, uint16_t len) {
	uint16_t crc = 0x6363;
	uint8_t *data = (uint8_t *) buf;
	uint8_t byte8 = 0;

	while (len--) {
		byte8 = *data++;
		byte8 ^= crc & 0xFF;
		byte8 ^= byte8 << 4;

		crc = ((((uint16_t)byte8 << 8) | (((crc) >> 8) & 0xFF))
				^ (uint8_t)(byte8 >> 4)
				^ ((uint16_t)byte8 << 3));
	}

	data[0] = (crc >> 0) & 0x00FF;
	data[1] = (crc >> 8) & 0x00FF;

	return crc;
}

__HIGH_CODE
uint8_t ISO14443ACheckOddParityBit(uint8_t *data, uint8_t *parity, uint16_t len) {
	for (int i = 0; i < len; i++) {
		if ((byteParityBitsTable[data[i]]) != parity[i]) {
			return 0;
		}
	}
	return 1;
}

__HIGH_CODE
void ISO14443ACalOddParityBit(uint8_t *data, uint8_t *out_parity, uint16_t len) {
	for (int i = 0; i < len; i++) {
		out_parity[i] = byteParityBitsTable[data[i]];
	}
}

// PCD functions
__HIGH_CODE
int PcdReq(int cmd, uint8_t *data, int len) {
	uint16_t res = PCD_COMMUNICATE_ERROR;
	int comm_len = 0;

	if(cmd != CMD_RBLOCK) {
		// clear chaining buffers
		for(int i = 0; i < NFCA_PCD_CHAINING_BUFFERS; i++) {
			g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)] = 0;
		}
	}

	switch(cmd) {
	case CMD_REQA:
	case CMD_WUPA:
		g_nfca_pcd_send_buf[0] = cmd;
		// REQA/WUPA are 7-bit frames but still require a correct odd parity bit.
		// If we leave g_nfca_parity_buf stale here, many tags will never answer.
		g_nfca_parity_buf[0] = byteParityBitsTable[cmd];
		nfca_pcd_wait_us(200);
		comm_len = 7;
		break;
	case CMD_HALT:
		g_nfca_pcd_send_buf[0] = CMD_HALT;
		g_nfca_pcd_send_buf[1] = 0;
		len = 2;
		comm_len = (len +2) * 8;

		ISO14443AAppendCRCA(g_nfca_pcd_send_buf, len);
		ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, len +2);	
		nfca_pcd_wait_ms(5);
		break;
	case CMD_ANTICOLL1:
	case CMD_ANTICOLL2:
	case CMD_ANTICOLL3:
		if(len == 2) {
			g_nfca_pcd_send_buf[0] = cmd;
			g_nfca_pcd_send_buf[1] = 0x20;
			comm_len = len *8;

			ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, len);
			nfca_pcd_wait_ms(PCD_ANTICOLL_OVER_TIME);
		}
		else {
			g_nfca_pcd_send_buf[0] = cmd;
			g_nfca_pcd_send_buf[1] = 0x70;
			g_nfca_pcd_send_buf[6] = 0;
			for(int i = 0; i < 4; i++) {
				g_nfca_pcd_send_buf[i + 2] = data[i];
				g_nfca_pcd_send_buf[6] ^= data[i];
			}
			len = 7;
			comm_len = (len +2) * 8;

			ISO14443AAppendCRCA(g_nfca_pcd_send_buf, len);
			ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, len +2);
			nfca_pcd_wait_ms(PCD_SELECT_OVER_TIME);
		}
		break;
	case CMD_RATS:
		g_nfca_pcd_send_buf[0] = CMD_RATS;
		g_nfca_pcd_send_buf[1] = 0x80; // FSDI, max frame size 256
		len = 2;
		comm_len = (len +2) * 8;
		apdu_iblock_number = 0x02; // we'll start with apdu's in i-blocks after this

		ISO14443AAppendCRCA(g_nfca_pcd_send_buf, len);
		ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, len +2);
		nfca_pcd_wait_ms(PCD_APDU_OVER_TIME);
		break;
	case CMD_IBLOCK:
		g_nfca_pcd_send_buf[0] = apdu_iblock_number;
		apdu_iblock_number ^= 1;
		for(int i = 0; i < len; i++) {
			g_nfca_pcd_send_buf[i +1] = data[i];
		}
		len++;
		comm_len = (len +2) * 8;

		ISO14443AAppendCRCA(g_nfca_pcd_send_buf, len);
		ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, len +2);
		nfca_pcd_wait_ms(PCD_APDU_OVER_TIME);
		break;
	case CMD_WTX:
		g_nfca_pcd_send_buf[0] = data[0];
		g_nfca_pcd_send_buf[1] = data[1];
		len = 2;
		comm_len = (len +2) * 8;

		ISO14443AAppendCRCA(g_nfca_pcd_send_buf, len);
		ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, len +2);
		break;
	case CMD_RBLOCK:
		g_nfca_pcd_send_buf[0] = cmd + apdu_iblock_number;
		apdu_iblock_number ^= 1;
		len = 1;
		comm_len = (len +2) * 8;

		ISO14443AAppendCRCA(g_nfca_pcd_send_buf, len);
		ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, len +2);
		break;
	}

	if (nfca_pcd_comm(comm_len, NFCA_PCD_REC_MODE_NORMAL, 0) == 0) {
		nfca_pcd_controller_state_t st = nfca_pcd_wait_comm_end();
		g_last_nfc_comm_status = g_nfca_pcd_comm_status;
		g_last_nfc_recv_bits = g_nfca_pcd_recv_bits;
		if(st == NFCA_PCD_CONTROLLER_STATE_DONE) {
			if (g_nfca_pcd_recv_bits % 9 == 0) {
				int nbytes = g_nfca_pcd_recv_bits /9;
				if (ISO14443ACheckOddParityBit(g_nfca_pcd_recv_buf, g_nfca_parity_buf, nbytes)) {
					if ((cmd == CMD_REQA) || (cmd == CMD_WUPA)) {
						res = PCD_NO_ERROR;
					}
					else if (((cmd == CMD_ANTICOLL1) || (cmd == CMD_ANTICOLL2) || (cmd == CMD_ANTICOLL3)) && g_nfca_pcd_send_buf[1] == 0x20) {
						if (ISO14443A_CHECK_BCC(g_nfca_pcd_recv_buf)) {
							res = PCD_NO_ERROR;
						}
						else {
							res = PCD_BCC_ERROR;
						}
					}
					else if (ISO14443_CRCA(g_nfca_pcd_recv_buf, nbytes) == 0) {
						res = PCD_NO_ERROR;
					}
					else {
						res = PCD_CRC_ERROR;
					}
				}
				else {
					res = PCD_ODD_PARITY_ERROR;
				}
			}
			else {
				// recv_bits not multiple of 9
				res = PCD_FRAME_ERROR;
			}
		}
	}

	// handle sblocks and rblocks
	if(res == PCD_NO_ERROR) {
		if(g_nfca_pcd_recv_buf[0] == CMD_WTX) {
			res = PcdReq(CMD_WTX, g_nfca_pcd_recv_buf, 2);
		}
		else if ((g_nfca_pcd_recv_buf[0] & 0x10) && ((cmd == CMD_IBLOCK) || (cmd == CMD_RBLOCK))) {
			// Chaining bit is set, copy receive buffer to available chain buffer
			for(int i = 0; i < NFCA_PCD_CHAINING_BUFFERS; i++) {
				if(g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)] == 0) {
					for(int j = 0; j < sizeof(g_nfca_pcd_recv_buf); j++) {
						g_nfca_pcd_recv_buf_chain[(i * sizeof(g_nfca_pcd_recv_buf)) +j] = g_nfca_pcd_recv_buf[j];
					}
					break;
				}
			}
			res = PcdReq(CMD_RBLOCK, 0, 0); // we are ready, request the next part of the chained frame
		}
	}

	return res;
}


// main process loop functions

__HIGH_CODE
void process_usb_packet(void) {
	// Check if we have received a packet in the global buffer
	if( ((uint16_t*)gs_usb_data_buf)[0]  ) {
		uint16_t len = ((uint16_t*)gs_usb_data_buf)[0];
		volatile uint8_t* data = &gs_usb_data_buf[2];

		// Handle specific commands here if needed (e.g. configuration)
		//

		// If we receive data but we aren't in card_interaction,
		// it might be a mistake or a re-sync. Clear the buffer.
		((uint16_t*)gs_usb_data_buf)[0] = 0;
	}
}

__HIGH_CODE
void send_heartbeat_if_free(void) {
	const char *version = "nfca585 v1";
	// Only send if endpoint is free to avoid blocking
	if( !USBFSCTX.USBFS_Endp_Busy[USB_EP_TX] ) {
		USBFS_SendEndpointNEW( USB_EP_TX, (uint8_t*)version, strlen(version), 1 );
	}
}

__HIGH_CODE
static int usb_send_debug_if_free(void)
{
#if USB_DEBUG_TELEMETRY
	if( USBFSCTX.USBFS_Endp_Busy[USB_EP_TX] ) return 0;

	// Keep packet <= 64 bytes.
	uint8_t p[16] = {0};
	p[0] = USB_PKT_DEBUG;
	p[1] = g_last_nfc_comm_status;
	p[2] = (uint8_t)(g_last_nfc_intf_status & 0xff);
	p[3] = (uint8_t)(g_last_nfc_intf_status >> 8);
	p[4] = (uint8_t)(g_last_nfc_recv_bits & 0xff);
	p[5] = (uint8_t)((g_last_nfc_recv_bits >> 8) & 0xff);
	p[6] = (uint8_t)((g_last_nfc_recv_bits >> 16) & 0xff);
	p[7] = (uint8_t)((g_last_nfc_recv_bits >> 24) & 0xff);
	p[8] = (uint8_t)(g_last_nfc_ant_adc & 0xff);
	p[9] = (uint8_t)(g_last_nfc_ant_adc >> 8);
	p[10] = (uint8_t)(g_last_lpcd_base & 0xff);
	p[11] = (uint8_t)(g_last_lpcd_base >> 8);
	p[12] = g_last_lpcd_hit;
	p[13] = (uint8_t)(R8_NFC_CMD);
	p[14] = (uint8_t)(R8_NFC_STATUS);
	USBFS_SendEndpointNEW( USB_EP_TX, p, sizeof(p), 1 );
	return 1;
#endif
	return 0;
}

__HIGH_CODE
int usb_send_blocking(const uint8_t *data, uint16_t len, uint32_t timeout_ms) {
	uint32_t timeout = SysTick->CNTL + Ticks_from_Ms(timeout_ms);

	while (len) {
		uint16_t chunk = (len > 64) ? 64 : len;

		while (USBFSCTX.USBFS_Endp_Busy[USB_EP_TX]) {
			if (SysTick->CNTL >= timeout) {
				return 0;
			}
		}

		USBFS_SendEndpointNEW(USB_EP_TX, (uint8_t *)data, chunk, 1);
		data += chunk;
		len -= chunk;
	}
	return 1;
}

__HIGH_CODE
void usb_send_ntag_version_packet(int uid_len) {
	uint8_t packet[1 +1 +10 +NTAG21X_VERSION_LEN];

	packet[0] = USB_PKT_NTAG_VERSION;
	packet[1] = uid_len;
	for (int i = 0; i < 10; ++i) {
		packet[2 +i] = (i < uid_len) ? gs_picc_uid[i] : 0;
	}
	for (int i = 0; i < NTAG21X_VERSION_LEN; ++i) {
		packet[12 +i] = g_last_ntag_version[i];
	}

	usb_send_blocking(packet, sizeof(packet), 50);
}

__HIGH_CODE
void usb_send_ntag_page_packet(uint8_t start_page, const uint8_t *page_data, uint8_t page_count) {
	uint8_t packet[3 +(4 *4)];
	uint8_t copy_len = page_count * 4;

	packet[0] = USB_PKT_NTAG_PAGE_DATA;
	packet[1] = start_page;
	packet[2] = page_count;
	for (int i = 0; i < 16; ++i) {
		packet[3 +i] = (i < copy_len) ? page_data[i] : 0;
	}

	usb_send_blocking(packet, sizeof(packet), 50);
}

__HIGH_CODE
void usb_send_ntag_done_packet(uint8_t status_code) {
	uint8_t packet[2] = { USB_PKT_NTAG_DONE, status_code };
	usb_send_blocking(packet, sizeof(packet), 50);
}

__HIGH_CODE
int ntag_get_version(void) {
	g_nfca_pcd_send_buf[0] = CMD_T2T_GET_VERSION;
	ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, 1);
	nfca_pcd_wait_ms(PCD_T2T_OVER_TIME);

	if (nfca_pcd_comm(8, NFCA_PCD_REC_MODE_NORMAL, 0) != 0) {
		return 0;
	}
	if (nfca_pcd_wait_comm_end() != NFCA_PCD_CONTROLLER_STATE_DONE) {
		return 0;
	}
	if ((g_nfca_pcd_recv_bits % 9) != 0) {
		return 0;
	}

	int nbytes = g_nfca_pcd_recv_bits / 9;
	if (nbytes != (NTAG21X_VERSION_LEN +2)) {
		return 0;
	}
	if (!ISO14443ACheckOddParityBit(g_nfca_pcd_recv_buf, g_nfca_parity_buf, nbytes)) {
		return 0;
	}
	if (ISO14443_CRCA(g_nfca_pcd_recv_buf, nbytes) != 0) {
		return 0;
	}

	for (int i = 0; i < NTAG21X_VERSION_LEN; ++i) {
		g_last_ntag_version[i] = g_nfca_pcd_recv_buf[i];
	}

	return 1;
}

__HIGH_CODE
int ntag_is_ntag215(void) {
	return (g_last_ntag_version[1] == 0x04) &&
	       (g_last_ntag_version[2] == 0x04) &&
	       (g_last_ntag_version[3] == 0x02) &&
	       (g_last_ntag_version[6] == 0x11);
}

__HIGH_CODE
int ntag_read_pages(uint8_t start_page) {
	g_nfca_pcd_send_buf[0] = CMD_T2T_READ;
	g_nfca_pcd_send_buf[1] = start_page;
	ISO14443AAppendCRCA(g_nfca_pcd_send_buf, 2);
	ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, 4);
	nfca_pcd_wait_ms(PCD_T2T_OVER_TIME);

	if (nfca_pcd_comm(32, NFCA_PCD_REC_MODE_NORMAL, 0) != 0) {
		return 0;
	}
	if (nfca_pcd_wait_comm_end() != NFCA_PCD_CONTROLLER_STATE_DONE) {
		return 0;
	}
	if ((g_nfca_pcd_recv_bits % 9) != 0) {
		return 0;
	}

	int nbytes = g_nfca_pcd_recv_bits / 9;
	if (nbytes != (NTAG_T2T_READ_BYTES +2)) {
		return 0;
	}
	if (!ISO14443ACheckOddParityBit(g_nfca_pcd_recv_buf, g_nfca_parity_buf, nbytes)) {
		return 0;
	}

	return ISO14443_CRCA(g_nfca_pcd_recv_buf, nbytes) == 0;
}

__HIGH_CODE
void ntag_dump_read_only(int uid_len) {
	uint8_t page_starts[(NTAG215_TOTAL_PAGES +3) /4];
	int page_start_count = 0;

	for (uint8_t page = 0; page <= 128; page += 4) {
		page_starts[page_start_count++] = page;
	}
	page_starts[page_start_count++] = 131;

	if (!ntag_get_version()) {
		usb_send_ntag_done_packet(1);
		return;
	}

	if (!ntag_is_ntag215()) {
		usb_send_ntag_done_packet(2);
		return;
	}

	usb_send_ntag_version_packet(uid_len);

	for (int i = 0; i < page_start_count; ++i) {
		uint8_t start_page = page_starts[i];
		uint8_t valid_pages = 4;

		if ((start_page +4) > NTAG215_TOTAL_PAGES) {
			valid_pages = NTAG215_TOTAL_PAGES - start_page;
		}

		if (!ntag_read_pages(start_page)) {
			usb_send_ntag_done_packet(3);
			return;
		}

		usb_send_ntag_page_packet(start_page, g_nfca_pcd_recv_buf, valid_pages);
	}

	usb_send_ntag_done_packet(0);
}

__HIGH_CODE
void card_interaction(int uid_len, int sak) {
	int iso14443_3 = (sak == 0x20);
	int iso14443_3_devtest_jcop = (sak == 0x00 && gs_picc_uid[0] == 0x00);
	int iso14443_3_type2 = (sak == 0x00 && gs_picc_uid[0] != 0x00);
	uint32_t timeout = 0;

	if (iso14443_3_type2) {
		ntag_dump_read_only(uid_len);
		return;
	}

	if(iso14443_3 || iso14443_3_devtest_jcop) {
		if(PcdReq(CMD_RATS, 0, 0) == PCD_NO_ERROR) {
			// Send RATS response to PC
			usb_send_blocking((const uint8_t *)g_nfca_pcd_recv_buf, (uint16_t)(g_nfca_pcd_recv_bits / 9), 100);

			// Set a timeout for the PC to reply
			timeout = SysTick->CNTL + Ticks_from_Ms(PCD_APDU_OVER_TIME);

			// While waiting for PC response, we block here because timing is strict
			// for the NFC field. Losing the field usually kills the card state.
			while(SysTick->CNTL < timeout) {
				if( *(volatile uint16_t*)gs_usb_data_buf ) {
					// we got a new APDU to send from PC
					if( PcdReq(CMD_IBLOCK, &gs_usb_data_buf[2], ((uint16_t*)gs_usb_data_buf)[0]) == PCD_NO_ERROR ) {
						// Send chained responses
						for(int i = 0; i < NFCA_PCD_CHAINING_BUFFERS; i++) {
							if(g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)]) {
								usb_send_blocking(&g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)], (uint16_t)sizeof(g_nfca_pcd_recv_buf), 100);
								g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)] = 0;
							}
							else {
								break;
							}
						}
						// Send final response
						usb_send_blocking((const uint8_t *)g_nfca_pcd_recv_buf, (uint16_t)(g_nfca_pcd_recv_bits / 9), 100);

						// Reset timeout for next command
						timeout = SysTick->CNTL + Ticks_from_Ms(PCD_APDU_OVER_TIME);
					}

					// Ack processing of buffer
					((uint16_t*)gs_usb_data_buf)[0] = 0;
				}
			}
		}
	}
}

__HIGH_CODE
void main_task_loop() {
	int res = PCD_COMMUNICATE_ERROR;
	int atqa, sak;
	int uuid_len = 0;
	uint32_t next_scan_time = 0;
	uint32_t next_heartbeat_time = 0;
#if SK6812_CYCLE_RGB
	uint32_t next_rgb_time = 0;
	uint8_t rgb_phase = 0;
#endif
	uint32_t next_led_time = 0;
	uint8_t led_on = 0;
	uint8_t led_step = 0;
	static const uint16_t led_pattern_ms[] = {
		60,  60,   // short blink
		60,  300,  // short blink + pause
		200, 800,  // long blink + long pause
	};

	// NFC status colors (visible even when SK6812_CYCLE_RGB=0).
	uint32_t nfc_color_until = 0;
	uint8_t nfc_r = 0, nfc_g = 0, nfc_b = 0;

	// Check VDD once at start or periodically if needed
	int vdd_value = ADC_VoltConverSignalPGA_MINUS_12dB( sys_get_vdd() );
	if( NFCA_FORCE_DRV )
	{
		NFCA_PCD_SET_OUT_DRV(NFCA_FORCE_DRV);
	}
	else
	{
		if(vdd_value > 3400)      NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL0);
		else if(vdd_value > 3000) NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL1);
		else if(vdd_value > 2600) NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL2);
		else                      NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL3);
	}

	while(1) {
		uint32_t now = SysTick->CNTL;

		// 1. Always process USB input (e.g. Reboot commands) immediately
		process_usb_packet();

#if 1
		// Always blink the PB3 status LED with a distinctive pattern so we can
		// tell at a glance which firmware revision is running.
		if( now > next_led_time )
		{
			led_on ^= 1;
			funDigitalWrite( LED, led_on ? LED_ON : LED_OFF );
			next_led_time = now + Ticks_from_Ms(led_pattern_ms[led_step]);
			led_step++;
			if( led_step >= (sizeof(led_pattern_ms)/sizeof(led_pattern_ms[0])) ) led_step = 0;
		}
#endif

#if SK6812_CYCLE_RGB
		// Simple "alive" indicator: cycle the SK6812 RGB without blocking NFC/USB.
		if( now > next_rgb_time )
		{
			switch( rgb_phase++ & 3 )
			{
				case 0: sk6812_set_rgb(SK6812_BRIGHT, 0, 0); break; // red
				case 1: sk6812_set_rgb(0, SK6812_BRIGHT, 0); break; // green
				case 2: sk6812_set_rgb(0, 0, SK6812_BRIGHT); break; // blue
				default: sk6812_set_rgb(0, 0, 0); break; // off
			}
			next_rgb_time = now + Ticks_from_Ms(250);
		}
#endif

		// If NFC code set a temporary color, keep it visible for a bit.
		if( now < nfc_color_until )
		{
			sk6812_set_rgb(nfc_r, nfc_g, nfc_b);
		}

		// 2. Handle Heartbeat (Non-blocking)
		if (now > next_heartbeat_time) {
			// Note: EP is single-buffered. If we send a heartbeat first, debug will
			// almost never get out. Prefer debug when enabled.
			if( !usb_send_debug_if_free() )
			{
				send_heartbeat_if_free();
			}
			next_heartbeat_time = now + Ticks_from_Ms(333);
		}

		// 3. Handle NFC Scan (Periodically)
		if (now > next_scan_time) {

			nfca_pcd_start();

			// Update debug snapshot even if LPCD is bypassed/disabled.
			g_last_nfc_ant_adc = nfca_adc_get_ant_signal();
			g_last_lpcd_base = gs_lpcd_adc_base_value;
			g_last_lpcd_hit = 0;

			// Diagnostic: sample antenna signal; if this never changes when a card is
			// near, the analog front-end / coil / matching network is likely the issue.
			// Show antenna ADC on the SK6812 when we're not doing the RGB heartbeat:
			// blue intensity ~ antenna reading, so you can see if anything changes.
			// (This runs even if we never get a valid REQA.)
#if !SK6812_CYCLE_RGB
			{
				uint16_t ant_adc = g_last_nfc_ant_adc;
				// Compress 12-bit-ish ADC down to 0..SK6812_BRIGHT.
				uint8_t level = (ant_adc >> 5);
				if( level > SK6812_BRIGHT ) level = SK6812_BRIGHT;
				sk6812_set_rgb(0, 0, level);
			}
#endif

			// LPCD Check (Fast)
			if(NFCA_BYPASS_LPCD || nfca_pcd_lpcd_check()) {
				// Visual debug: we're actually attempting a card transaction.
				nfc_r = 0; nfc_g = 0; nfc_b = SK6812_BRIGHT; // blue
				nfc_color_until = now + Ticks_from_Ms(250);
				sk6812_set_rgb(nfc_r, nfc_g, nfc_b);

				// Found something!
				// Reset antenna field to wake card
				R8_NFC_CMD &= ~RB_NFC_ANTENNA_ON;
				Delay_Ms(5);
				R8_NFC_CMD |= RB_NFC_ANTENNA_ON;
				Delay_Ms(5);

				// --- ANTICOLLISION SEQUENCE ---
				if(PcdReq(CMD_REQA, 0, 0) == PCD_NO_ERROR) {
					nfc_r = 0; nfc_g = SK6812_BRIGHT; nfc_b = 0; // green
					nfc_color_until = now + Ticks_from_Ms(500);
					sk6812_set_rgb(nfc_r, nfc_g, nfc_b);
					atqa = ((uint16_t *)(g_nfca_pcd_recv_buf))[0];

					// Some tags are happier if we go straight into anticoll after REQA.
					// If you want the classic REQA->HALT->WUPA dance, disable bypass and tune.
					res = PcdReq(CMD_ANTICOLL1, /*0x20*/0, 2);

					if (res == PCD_NO_ERROR) {
						nfc_r = SK6812_BRIGHT; nfc_g = 0; nfc_b = SK6812_BRIGHT; // purple-ish
						nfc_color_until = now + Ticks_from_Ms(750);
						sk6812_set_rgb(nfc_r, nfc_g, nfc_b);
						if(g_nfca_pcd_recv_buf[0] == 0x88) {
							gs_picc_uid[0] = g_nfca_pcd_recv_buf[1];
							gs_picc_uid[1] = g_nfca_pcd_recv_buf[2];
							gs_picc_uid[2] = g_nfca_pcd_recv_buf[3];

							res = PcdReq(CMD_ANTICOLL1, g_nfca_pcd_recv_buf, 7);
							if (res == PCD_NO_ERROR) {
								res = PcdReq(CMD_ANTICOLL2, /*0x20*/0, 2);
								if (res == PCD_NO_ERROR) {
									if(g_nfca_pcd_recv_buf[0] == 0x88) {
										gs_picc_uid[3] = g_nfca_pcd_recv_buf[1];
										gs_picc_uid[4] = g_nfca_pcd_recv_buf[2];
										gs_picc_uid[5] = g_nfca_pcd_recv_buf[3];

										res = PcdReq(CMD_ANTICOLL2, g_nfca_pcd_recv_buf, 7);
										if (res == PCD_NO_ERROR) {
											res = PcdReq(CMD_ANTICOLL3, /*0x20*/0, 2);
											if (res == PCD_NO_ERROR) {
												uuid_len = 10;
												gs_picc_uid[6] = g_nfca_pcd_recv_buf[0];
												gs_picc_uid[7] = g_nfca_pcd_recv_buf[1];
												gs_picc_uid[8] = g_nfca_pcd_recv_buf[2];
												gs_picc_uid[9] = g_nfca_pcd_recv_buf[3];

													res = PcdReq(CMD_ANTICOLL3, g_nfca_pcd_recv_buf, 7);
													if (res == PCD_NO_ERROR) {
														sak = g_nfca_pcd_recv_buf[0];
													}
												}
											else {
												// anticoll3 error
											}
										}
										else {
											// uid1 error
										}
									}
									else {
										// 7 byte uid
										uuid_len = 7;
										gs_picc_uid[3] = g_nfca_pcd_recv_buf[0];
										gs_picc_uid[4] = g_nfca_pcd_recv_buf[1];
										gs_picc_uid[5] = g_nfca_pcd_recv_buf[2];
										gs_picc_uid[6] = g_nfca_pcd_recv_buf[3];

										res = PcdReq(CMD_ANTICOLL2, g_nfca_pcd_recv_buf, 7);
										if (res == PCD_NO_ERROR) {
											sak = g_nfca_pcd_recv_buf[0];
										}
									}
								}
								else {
									// anticoll2 error
								}
							}
							else {
								// uid0 error
							}
						}
						else {
							// 4 byte uid
							uuid_len = 4;
							gs_picc_uid[0] = g_nfca_pcd_recv_buf[0];
							gs_picc_uid[1] = g_nfca_pcd_recv_buf[1];
							gs_picc_uid[2] = g_nfca_pcd_recv_buf[2];
							gs_picc_uid[3] = g_nfca_pcd_recv_buf[3];

							res = PcdReq(CMD_ANTICOLL1, g_nfca_pcd_recv_buf, 7);
							if (res == PCD_NO_ERROR) {
								sak = g_nfca_pcd_recv_buf[0];
							}
						}
					}
					else {
						// anticoll1 error
					}

					if(uuid_len) {
						// Card is fully selected, enter interaction mode
						// This function IS allowed to block because we are in a session
						card_interaction(uuid_len, sak);

						// Clean up
						PcdReq(CMD_HALT, 0, 0);
						nfca_pcd_stop();
						tag_feedback((uint8_t)uuid_len, (uint16_t)atqa, (uint8_t)sak);
						atqa = 0;
						sak = 0;
						uuid_len = 0;
					}
				} else {
					// REQA failed. Show *why*:
					// - magenta: timeout / never reached DONE (often no IRQ / no field)
					// - orange: parity error
					// - cyan: CRC error (unlikely for REQA)
					// - red: other / generic fail
					switch( g_last_nfc_comm_status )
					{
						case 3: nfc_r = SK6812_BRIGHT; nfc_g = SK6812_BRIGHT/3; nfc_b = 0; break; // orange
						case 4: nfc_r = 0; nfc_g = SK6812_BRIGHT; nfc_b = SK6812_BRIGHT; break;   // cyan
						case 6: nfc_r = SK6812_BRIGHT; nfc_g = SK6812_BRIGHT; nfc_b = SK6812_BRIGHT; break; // white
						default:
							// If we never left "sending/receiving", g_last_nfc_comm_status will be 1/2.
							if( g_last_nfc_comm_status == 1 || g_last_nfc_comm_status == 2 )
								{ nfc_r = SK6812_BRIGHT; nfc_g = 0; nfc_b = SK6812_BRIGHT; } // magenta
							else
								{ nfc_r = SK6812_BRIGHT; nfc_g = 0; nfc_b = 0; } // red
							break;
					}
					nfc_color_until = now + Ticks_from_Ms(80);
					sk6812_set_rgb(nfc_r, nfc_g, nfc_b);
				}
			}

			// Turn off NFC to save power/reset state
			nfca_pcd_stop();

			// Schedule next scan for 333ms from NOW
			next_scan_time = now + Ticks_from_Ms(333);
		}
	}
}

int main() {
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx
	GPIOSetup();

	// Quick sanity wiggle: regardless of LED polarity, you should see *something*
	// if the LED is wired correctly and the firmware is running.
	for( int i = 0; i < 2; i++ )
	{
		funDigitalWrite( LED, FUN_LOW );
		Delay_Ms(150);
		funDigitalWrite( LED, FUN_HIGH );
		Delay_Ms(150);
	}
	funDigitalWrite( LED, LED_OFF );

	// If firmware is running at all, show a visible "I'm alive" indicator even if USB never enumerates.
#if !SK6812_DISABLE
	sk6812_set_rgb(SK6812_BRIGHT, 0, 0); // red
#endif

#ifdef FW_SAFE_MODE
	// Debug-only mode: do not touch USB/NFC. Just cycle the SK6812 forever so we
	// can verify the app is actually running after flashing.
	while(1)
	{
		funDigitalWrite( LED, LED_ON );
		sk6812_set_rgb(0, SK6812_BRIGHT, 0); // green
		Delay_Ms(200);
		funDigitalWrite( LED, LED_OFF );
		sk6812_set_rgb(SK6812_BRIGHT, 0, 0); // red
		Delay_Ms(200);
		sk6812_set_rgb(0, 0, SK6812_BRIGHT); // blue
		Delay_Ms(200);
		sk6812_set_rgb(0, 0, 0); // off
		Delay_Ms(200);
	}
#endif
	USBFSSetup();

	blink(5); // Indicates boot
#if !SK6812_DISABLE
	sk6812_set_rgb(0, SK6812_BRIGHT, 0); // green after boot
#endif

	nfca_init();
	nfca_pcd_lpcd_calibration();

	main_task_loop(); // Never returns

	while(1);
}
