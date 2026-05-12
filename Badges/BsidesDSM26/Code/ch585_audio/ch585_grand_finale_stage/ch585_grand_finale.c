#include "ch32fun.h"
#include "fsusb.h"
#include <string.h>

#ifndef ENABLE_PB8_STATUS_LED
#define ENABLE_PB8_STATUS_LED 0
#endif

#if ENABLE_PB8_STATUS_LED
#define LED PB8
#endif

#ifndef STATUS_SK6812_PIN
#define STATUS_SK6812_PIN PB6
#endif

#ifndef SK6812_DISABLE
#define SK6812_DISABLE 0
#endif

#ifndef SK6812_PROFILE
#define SK6812_PROFILE 5
#endif

#ifndef SK6812_LSB_FIRST
#define SK6812_LSB_FIRST 0
#endif

#ifndef SK6812_IS_RGBW
#define SK6812_IS_RGBW 1
#endif

#ifndef SK6812_BRIGHT
#define SK6812_BRIGHT 24
#endif

#ifndef SK6812_W_BRIGHT
#define SK6812_W_BRIGHT 0
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
#define USB_APP_MAX_PAYLOAD                      (USB_DATA_BUF_SIZE - 2) // 2 bytes reserved for length
#define USBFS_MAX_PACKET                         64

// PB8/PB9 are NFC analog pins (NFCM/NFCI) on CH585. Don't enable INTX remap / GPIO interrupts that
// route through PB8/PB9, or NFC reception can break.
#ifndef ENABLE_BOOT_BUTTON
#define ENABLE_BOOT_BUTTON                        0
#endif
#define NFCA_DATA_BUF_SIZE                       256 // we use FSDI 8, so max frame size is 256
#define NFCA_PCD_MAX_SEND_NUM                    NFCA_DATA_BUF_SIZE
#define NFCA_PCD_MAX_RECV_NUM                    NFCA_DATA_BUF_SIZE
#define NFCA_MAX_PARITY_NUM                      NFCA_PCD_MAX_RECV_NUM
#define NFCA_PCD_CHAINING_BUFFERS                4
#define NFCA_PCD_LPCD_THRESHOLD_PERMIL           5
#define NFCA_PCD_LPCD_THRESHOLD_MAX_LIMIT_PERMIL 20
#define NFCA_PCD_WAIT_MAX_MS                     1000
#define NFCA_PCD_TICKS_PER_MILLISECOND           1725

// Debug/bring-up: bypass LPCD gating and always attempt REQA/WUPA.
// This keeps the RF field active during scans and is higher power, but avoids tuning/threshold issues.
#ifndef NFCA_FORCE_ACTIVE_SCAN
#define NFCA_FORCE_ACTIVE_SCAN                   1
#endif

// Optional: drive the NFC_CTR pin (often used in reference designs to improve drive/sensitivity
// or to allow wider-voltage operation via an external resistor network).
// If your board doesn't wire NFC_CTR, enabling this is harmless.
#ifndef NFCA_ENABLE_CTR_PIN
#define NFCA_ENABLE_CTR_PIN                      1
#endif

// Bring-up / weak-antenna assist: force the highest driver strength.
// If you see unstable USB resets or excessive current draw, turn this off.
#ifndef NFCA_FORCE_MAX_DRIVE
#define NFCA_FORCE_MAX_DRIVE                     1
#endif

// Number of quick poll attempts per scan window.
#ifndef NFCA_REQA_RETRIES
#define NFCA_REQA_RETRIES                        5
#endif

// Extra delay after RF field is enabled before polling.
#ifndef NFCA_FIELD_SETTLE_MS
#define NFCA_FIELD_SETTLE_MS                     10
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

#define NTAG21X_VERSION_LEN    8
#define NTAG_T2T_READ_BYTES    16
#define NTAG215_TOTAL_PAGES    135

#define ISO14443A_CHECK_BCC(B)    ((B[0] ^ B[1] ^ B[2] ^ B[3]) == B[4])
// Preserve low NFC_DRV configuration bits when adjusting output drive.
// The original mask (0x9ff) clears bits 9..10 which appear to affect the analog front-end on some boards.
#define NFCA_PCD_SET_OUT_DRV(lvl) (R32_NFC_DRV = ((R32_NFC_DRV & 0x1fff) | lvl))

__attribute__((aligned(4))) static volatile uint8_t gs_usb_data_buf[USB_DATA_BUF_SIZE];
__attribute__((aligned(4))) static uint8_t g_usb_tx_buf[USB_DATA_BUF_SIZE];
__attribute__((aligned(4))) static uint16_t gs_nfca_data_buf[NFCA_DATA_BUF_SIZE];
__attribute__((aligned(4))) static uint8_t g_nfca_parity_buf[NFCA_MAX_PARITY_NUM];
__attribute__((aligned(4))) static uint8_t g_nfca_pcd_send_buf[((NFCA_PCD_MAX_SEND_NUM + 3) & 0xfffc)];
__attribute__((aligned(4))) static uint8_t g_nfca_pcd_recv_buf[((NFCA_PCD_MAX_RECV_NUM + 3) & 0xfffc)];
__attribute__((aligned(4))) static uint8_t g_nfca_pcd_recv_buf_chain[sizeof(g_nfca_pcd_recv_buf) *NFCA_PCD_CHAINING_BUFFERS];
__attribute__((aligned(4))) static uint16_t gs_lpcd_adc_filter_buf[8];
__attribute__((aligned(4))) static uint8_t gs_picc_uid[10];

static volatile uint16_t g_usb_tx_len;
static volatile uint16_t g_usb_tx_off;
static volatile uint16_t g_usb_rx_expected;
static volatile uint16_t g_usb_rx_received;
static volatile uint8_t g_usb_rx_len_bytes;
static volatile uint8_t g_usb_rx_len_tmp0;
static volatile uint8_t g_usb_rx_len_tmp1;

static uint16_t gs_lpcd_adc_base_value;
static uint16_t g_nfca_pcd_recv_buf_len;
static volatile uint32_t g_nfca_pcd_recv_bits;
static uint16_t g_nfca_pcd_recv_word_idx;
static uint16_t g_nfca_pcd_send_fifo_bytes;
static uint16_t g_nfca_pcd_send_total_bytes;
static uint8_t g_nfca_pcd_buf_offset;
static uint8_t g_nfca_pcd_intf_mode;
static uint8_t g_nfca_pcd_comm_status;
// Some ISO14443A responses (ATQA, anticollision CLn) do not include CRC_A.
// The CH58x NFC block can flag CRC errors in these cases; we optionally
// disable HW CRC checking and/or treat CRCERR as a "done" condition.
static volatile uint8_t g_nfca_pcd_rx_crc_enable = 1;
static volatile uint8_t g_nfca_pcd_crcerr_is_fatal = 1;
static volatile uint8_t g_nfca_pcd_rx_status_reg = 0x26;
static volatile uint8_t g_nfca_pcd_crcerr_seen = 0;
static volatile uint8_t apdu_iblock_number = 0x02;
static uint8_t g_last_ntag_version[NTAG21X_VERSION_LEN];

static inline uint64_t now_ticks(void) {
	return (uint64_t)funSysTick32();
}

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

#define SK6812__ADD_N_NOPS(n) asm volatile( ".rept " #n "\nc.nop\n.endr" );
#define SK6812_ADD_N_NOPS(n)  SK6812__ADD_N_NOPS(n)

static uint8_t g_audio_pwm_initialized;

void nfca_pcd_wait_ms(uint32_t milliseconds);
uint8_t nfca_pcd_comm(uint16_t data_bits_num, NFCA_PCD_REC_MODE_Def mode, uint8_t offset);
nfca_pcd_controller_state_t nfca_pcd_wait_comm_end(void);
uint16_t ISO14443_CRCA(uint8_t *buf, int len);
uint16_t ISO14443AAppendCRCA(void *buf, uint16_t len);
uint8_t ISO14443ACheckOddParityBit(uint8_t *data, uint8_t *parity, uint16_t len);
void ISO14443ACalOddParityBit(uint8_t *data, uint8_t *out_parity, uint16_t len);

static void __attribute__((noinline)) sk6812_send_bit_0(void)
{
	GPIO_SetBits(STATUS_SK6812_PIN);
	SK6812_ADD_N_NOPS(SK6812_T0H_NOPS);
	GPIO_ResetBits(STATUS_SK6812_PIN);
	SK6812_ADD_N_NOPS(SK6812_T0L_NOPS);
}

static void __attribute__((noinline)) sk6812_send_bit_1(void)
{
	GPIO_SetBits(STATUS_SK6812_PIN);
	SK6812_ADD_N_NOPS(SK6812_T1H_NOPS);
	GPIO_ResetBits(STATUS_SK6812_PIN);
	SK6812_ADD_N_NOPS(SK6812_T1L_NOPS);
}

static void sk6812_send_byte(uint8_t b)
{
	for (int i = 0; i < 8; i++)
	{
#if SK6812_LSB_FIRST
		if (b & 0x01) sk6812_send_bit_1();
		else          sk6812_send_bit_0();
		b >>= 1;
#else
		if (b & 0x80) sk6812_send_bit_1();
		else          sk6812_send_bit_0();
		b <<= 1;
#endif
	}
}

static void sk6812_set_grbw(uint8_t g, uint8_t r, uint8_t b, uint8_t w)
{
#if !SK6812_DISABLE
	uint8_t irq_was_enabled = __isenabled_irq();
	__disable_irq();
	sk6812_send_byte(g);
	sk6812_send_byte(r);
	sk6812_send_byte(b);
#if SK6812_IS_RGBW
	sk6812_send_byte(w);
#else
	(void)w;
#endif
	if (irq_was_enabled) __enable_irq();
#else
	(void)g;
	(void)r;
	(void)b;
	(void)w;
#endif

	GPIO_ResetBits(STATUS_SK6812_PIN);
	Delay_Ms(1);
}

static void sk6812_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	sk6812_set_grbw(g, r, b, SK6812_W_BRIGHT);
}

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
	uint64_t next_tick = now_ticks();

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
			int32_t sample = 0;

			while ((int64_t)(now_ticks() - next_tick) < 0) { }
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

static void audio_play_detect_chirp(void)
{
	uint32_t phase = 0;
	int32_t filter_state = 0;
	uint64_t next_tick = now_ticks();
	const uint16_t chirp_notes[] = { 988u, 1319u };
	const uint16_t chirp_lengths[] = { MS_TO_SAMPLES(28), MS_TO_SAMPLES(36) };

	audio_init();

	for (unsigned note_idx = 0; note_idx < (sizeof(chirp_notes) / sizeof(chirp_notes[0])); note_idx++)
	{
		uint32_t phase_step = audio_hz_to_phase_step(chirp_notes[note_idx]);

		for (uint16_t i = 0; i < chirp_lengths[note_idx]; i++)
		{
			int32_t sample = 0;

			while ((int64_t)(now_ticks() - next_tick) < 0) { }
			next_tick += AUDIO_SAMPLE_TICKS;

			phase += phase_step;
			sample = (sine32_wave(phase) * 110) / 128;
			filter_state += ((sample - filter_state) >> 2);
			sample = filter_state + AUDIO_PWM_MIDPOINT;
			if (sample < 0) sample = 0;
			if (sample > 255) sample = 255;
			R8_PWM6_DATA = (uint8_t)sample;
		}
	}

	R8_PWM6_DATA = AUDIO_PWM_MIDPOINT;
}

static void audio_play_boot_debug_beep(void)
{
	funPinMode(AUDIO_PWM_PIN, GPIO_ModeOut_PP_20mA);
	GPIO_ResetBits(AUDIO_PWM_PIN);

	for (unsigned burst = 0; burst < 2; burst++)
	{
		uint8_t high_us = burst ? 90u : 140u;
		uint8_t low_us = high_us;

		for (unsigned i = 0; i < 180; i++)
		{
			GPIO_SetBits(AUDIO_PWM_PIN);
			Delay_Us(high_us);
			GPIO_ResetBits(AUDIO_PWM_PIN);
			Delay_Us(low_us);
		}

		Delay_Ms(30);
	}

	GPIO_ResetBits(AUDIO_PWM_PIN);
	g_audio_pwm_initialized = 0;
	audio_init();
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

static inline void nfca_rx_finalize_from_hw(void) {
	g_nfca_pcd_recv_bits = R16_NFC_RXTX_LEN;
	uint16_t received_words = (g_nfca_pcd_recv_bits + 15) / 16;
	if (g_nfca_pcd_recv_word_idx < received_words) {
		uint16_t words_to_drain = received_words - g_nfca_pcd_recv_word_idx;
		for (int i = 0; i < words_to_drain; i++) {
			gs_nfca_data_buf[g_nfca_pcd_recv_word_idx++] = R16_NFC_FIFO;
		}
	}
}

static inline void usb_rx_reset_assembly(void) {
	g_usb_rx_expected = 0;
	g_usb_rx_received = 0;
	g_usb_rx_len_bytes = 0;
	g_usb_rx_len_tmp0 = 0;
	g_usb_rx_len_tmp1 = 0;
}

static inline int usb_tx_queue_message(const uint8_t *payload, uint16_t payload_len) {
	if (payload_len > USB_APP_MAX_PAYLOAD) return -1;
	if (g_usb_tx_len) return -2; // busy

	g_usb_tx_buf[0] = (uint8_t)(payload_len & 0xff);
	g_usb_tx_buf[1] = (uint8_t)(payload_len >> 8);
	if (payload_len) memcpy(&g_usb_tx_buf[2], payload, payload_len);
	g_usb_tx_off = 0;
	g_usb_tx_len = payload_len + 2;
	return 0;
}

static inline void usb_tx_pump(void) {
	// USBFS uses "prime then host IN" semantics; we must explicitly ACK a packet with USBFS_SendEndpointNEW.
	if (!g_usb_tx_len) return;
	if (USBFSCTX.USBFS_Endp_Busy[USB_EP_TX] & 1) return;

	// Stage 0: send 2-byte header as a short packet.
	if (g_usb_tx_off == 0 && g_usb_tx_len >= 2) {
		USBFS_SendEndpointNEW(USB_EP_TX, (uint8_t*)g_usb_tx_buf, 2, /*copy=*/1);
		g_usb_tx_off = 2;
		if (g_usb_tx_len == 2) {
			g_usb_tx_len = 0;
			g_usb_tx_off = 0;
		}
		return;
	}

	// Stage 1: send payload in 64-byte packets.
	uint16_t remaining = g_usb_tx_len - g_usb_tx_off;
	uint16_t chunk = remaining > USBFS_MAX_PACKET ? USBFS_MAX_PACKET : remaining;
	USBFS_SendEndpointNEW(USB_EP_TX, (uint8_t*)&g_usb_tx_buf[g_usb_tx_off], chunk, /*copy=*/1);
	g_usb_tx_off += chunk;
	if (g_usb_tx_off >= g_usb_tx_len) {
		g_usb_tx_len = 0;
		g_usb_tx_off = 0;
	}
}

static void usb_send_ntag_version_packet(int uid_len)
{
	uint8_t packet[1 + 1 + 1 + 10 + NTAG21X_VERSION_LEN];

	packet[0] = USB_PKT_NTAG_VERSION;
	packet[1] = (uint8_t)uid_len;
	for (int i = 0; i < uid_len; ++i) {
		packet[2 + i] = gs_picc_uid[i];
	}
	for (int i = uid_len; i < 10; ++i) {
		packet[2 + i] = 0;
	}
	for (int i = 0; i < NTAG21X_VERSION_LEN; ++i) {
		packet[12 + i] = g_last_ntag_version[i];
	}

	while (g_usb_tx_len) { }
	usb_tx_queue_message(packet, sizeof(packet));
	usb_tx_pump();
}

static void usb_send_ntag_page_packet(uint8_t start_page, const uint8_t *page_data, uint8_t page_count)
{
	uint8_t packet[2 + NTAG_T2T_READ_BYTES];

	packet[0] = USB_PKT_NTAG_PAGE_DATA;
	packet[1] = start_page;
	memcpy(&packet[2], page_data, NTAG_T2T_READ_BYTES);

	while (g_usb_tx_len) { }
	usb_tx_queue_message(packet, (uint16_t)(2 + (page_count * 4)));
	usb_tx_pump();
}

static void usb_send_ntag_done_packet(uint8_t status_code)
{
	uint8_t packet[2] = { USB_PKT_NTAG_DONE, status_code };

	while (g_usb_tx_len) { }
	usb_tx_queue_message(packet, sizeof(packet));
	usb_tx_pump();
}

static int ntag_get_version(void)
{
	g_nfca_pcd_send_buf[0] = CMD_T2T_GET_VERSION;
	ISO14443AAppendCRCA(g_nfca_pcd_send_buf, 1);
	ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, 3);
	nfca_pcd_wait_ms(PCD_T2T_OVER_TIME);

	if (nfca_pcd_comm(24, NFCA_PCD_REC_MODE_NORMAL, 0) != 0) {
		return 0;
	}
	if (nfca_pcd_wait_comm_end() != NFCA_PCD_CONTROLLER_STATE_DONE) {
		return 0;
	}
	if ((g_nfca_pcd_recv_bits % 9) != 0) {
		return 0;
	}

	int nbytes = g_nfca_pcd_recv_bits / 9;
	if (nbytes != (NTAG21X_VERSION_LEN + 2)) {
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

static int ntag_is_ntag215(void)
{
	return (g_last_ntag_version[1] == 0x04) &&
	       (g_last_ntag_version[2] == 0x04) &&
	       (g_last_ntag_version[3] == 0x02) &&
	       (g_last_ntag_version[6] == 0x11);
}

static int ntag_read_pages(uint8_t start_page)
{
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
	if (nbytes != (NTAG_T2T_READ_BYTES + 2)) {
		return 0;
	}
	if (!ISO14443ACheckOddParityBit(g_nfca_pcd_recv_buf, g_nfca_parity_buf, nbytes)) {
		return 0;
	}

	return ISO14443_CRCA(g_nfca_pcd_recv_buf, nbytes) == 0;
}

static void ntag_dump_read_only(int uid_len)
{
	uint8_t page_starts[(NTAG215_TOTAL_PAGES + 3) / 4];
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

		if ((start_page + 4) > NTAG215_TOTAL_PAGES) {
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

__HIGH_CODE
void blink(int n) {
	for(int i = n-1; i >= 0; i--) {
#if ENABLE_PB8_STATUS_LED
		funDigitalWrite( LED, FUN_LOW ); // Turn on LED
#endif
#if !SK6812_DISABLE
		switch( i % 3 )
		{
			case 0: sk6812_set_rgb(SK6812_BRIGHT, 0, 0); break;
			case 1: sk6812_set_rgb(0, SK6812_BRIGHT, 0); break;
			default: sk6812_set_rgb(0, 0, SK6812_BRIGHT); break;
		}
#endif
		Delay_Ms(33);
#if ENABLE_PB8_STATUS_LED
		funDigitalWrite( LED, FUN_HIGH ); // Turn off LED
#endif
#if !SK6812_DISABLE
		sk6812_set_rgb(0, 0, 0);
#endif
		if(i) Delay_Ms(33);
	}
}

__INTERRUPT
void GPIOB_IRQHandler() {
#if ENABLE_BOOT_BUTTON
	int status = R16_PB_INT_IF;
	R16_PB_INT_IF = status; // acknowledge

	if(status & PB8) { // PB22 interrupt comes in on PB8 if RB_PIN_INTX is set
		USBFSReset();
		blink(2);
		jump_isprom();
	}
#endif
}

void GPIOSetup() {
#if ENABLE_PB8_STATUS_LED
	funPinMode( LED, GPIO_CFGLR_OUT_2Mhz_PP );
#endif
	funPinMode( STATUS_SK6812_PIN, GPIO_ModeOut_PP_20mA );
	GPIO_ResetBits( STATUS_SK6812_PIN );
	audio_init();
	sk6812_set_grbw(0, 0, 0, 0);

#if ENABLE_BOOT_BUTTON
	funPinMode( PB22, GPIO_CFGLR_IN_PU ); // Set PB22 to input, pullup as keypress is to gnd

	R16_PIN_ALTERNATE |= RB_PIN_INTX; // set PB8 interrupt to PB22
	R16_PB_INT_MODE |= (PB8 & ~PB); // edge mode, should go to ch32fun.h
	funDigitalWrite(PB22, FUN_LOW); // falling edge

	NVIC_EnableIRQ(GPIOB_IRQn);
	R16_PB_INT_IF = (PB8 & ~PB); // reset PB8/PB22 flag
	R16_PB_INT_EN |= (PB8 & ~PB); // enable PB8/PB22 interrupt

	SYS_SAFE_ACCESS(
		R8_SLP_WAKE_CTRL |= (RB_WAKE_EV_MODE | RB_SLP_GPIO_WAKE);
		R8_SLP_POWER_CTRL &= ~(RB_WAKE_DLY_MOD);
		R8_SLP_POWER_CTRL |= 0x00; // 0x00: long delay, 0x01: short delay (short doesn't work here)
	);
#endif
}

int HandleSetupCustom( struct _USBState * ctx, int setup_code) {
	return 0;
}

int HandleInRequest( struct _USBState * ctx, int endp, uint8_t * data, int len ) {
	(void)ctx;
	(void)len;

	// For USBFS we "prime" IN packets via USBFS_SendEndpointNEW() (see usb_tx_pump()).
	// Leaving this handler empty keeps the endpoint NAK'd unless we explicitly queue data.
	(void)endp;
	(void)data;
	return 0;
}

__HIGH_CODE
void HandleDataOut( struct _USBState * ctx, int endp, uint8_t * data, int len ) {
	(void)ctx;

	if (endp != USB_EP_RX) return;

	// Protocol over USBFS bulk OUT:
	// [uint16_le payload_len][payload bytes...]
	//
	// We must assemble because USBFS max packet size is 64 bytes.
	int i = 0;
	while (i < len) {
		if (g_usb_rx_len_bytes < 2) {
			if (g_usb_rx_len_bytes == 0) g_usb_rx_len_tmp0 = data[i++];
			else g_usb_rx_len_tmp1 = data[i++];
			g_usb_rx_len_bytes++;

			if (g_usb_rx_len_bytes == 2) {
				g_usb_rx_expected = (uint16_t)g_usb_rx_len_tmp0 | ((uint16_t)g_usb_rx_len_tmp1 << 8);

				// Refuse oversized messages; reset assembly to re-sync.
				if (g_usb_rx_expected > USB_APP_MAX_PAYLOAD) {
					usb_rx_reset_assembly();
					((uint16_t*)gs_usb_data_buf)[0] = 0;
					return;
				}
			}
			continue;
		}

		// If an application packet is already waiting to be consumed, drop new data to avoid corruption.
		if (((uint16_t*)gs_usb_data_buf)[0]) {
			usb_rx_reset_assembly();
			return;
		}

		uint16_t remaining = g_usb_rx_expected - g_usb_rx_received;
		uint16_t take = (uint16_t)(len - i);
		if (take > remaining) take = remaining;

		memcpy((void*)&gs_usb_data_buf[2 + g_usb_rx_received], &data[i], take);
		i += take;
		g_usb_rx_received += take;

		if (g_usb_rx_received >= g_usb_rx_expected) {
			// Message complete. Publish length for main loop.
			((uint16_t*)gs_usb_data_buf)[0] = g_usb_rx_expected;

			// Preserve old reboot-to-bootloader magic (payload 0x010001a2).
			if (g_usb_rx_expected == 4 && ((uint32_t*)&gs_usb_data_buf[2])[0] == 0x010001a2) {
				USBFSReset();
				blink(2);
				jump_isprom();
			}

			usb_rx_reset_assembly();
			return;
		}
	}
}

	__INTERRUPT
	__HIGH_CODE
	void NFC_IRQHandler(void) {
		// Read the interrupt status and write it back to clear the flags.
		uint16_t intf_status = R16_NFC_INTF_STATUS;
		R16_NFC_INTF_STATUS = intf_status;

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

					// Set RX control register (optionally disables HW CRC checking for
					// ISO14443A short responses like ATQA).
					R8_NFC_STATUS = g_nfca_pcd_rx_status_reg;

					g_nfca_pcd_comm_status = 2; // Change state to Receiving
					R8_NFC_CMD |= 0x18; // Enable RX

					// Some CH58x NFC implementations appear to latch RX complete/error flags
					// in the same interrupt as TX complete. If so, consume them immediately
					// to avoid getting stuck in comm_status==2 until the software timeout.
					if (intf_status & 2) {
						nfca_rx_finalize_from_hw();
						g_nfca_pcd_comm_status = 5; // Status: Success
					}
					else if (intf_status & 0x10) {
						nfca_rx_finalize_from_hw();
						g_nfca_pcd_comm_status = 3; // Status: Parity Error
					}
					else if (intf_status & 0x20) {
						g_nfca_pcd_crcerr_seen = 1;
						nfca_rx_finalize_from_hw();
						g_nfca_pcd_comm_status = g_nfca_pcd_crcerr_is_fatal ? 4 : 5;
					}
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
				nfca_rx_finalize_from_hw();
				g_nfca_pcd_comm_status = 5; // Status: Success
			}
			// Check for error flags
			else if (intf_status & 0x10) {
				nfca_rx_finalize_from_hw();
				g_nfca_pcd_comm_status = 3; // Status: Parity Error
			}
			else if (intf_status & 0x20) {
				// CRCERR can be raised for frames without CRC_A (e.g. ATQA, anticollision).
				// Capture any received bits so the higher layer can still parse/parity-check.
				g_nfca_pcd_crcerr_seen = 1;
				nfca_rx_finalize_from_hw();

				g_nfca_pcd_comm_status = g_nfca_pcd_crcerr_is_fatal ? 4 : 5;
			}
		}
	// --- Any Other State: ERROR ---
	else {
		g_nfca_pcd_comm_status = 6; // Status: General Error
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
	{
		uint64_t deadline = now_ticks() + Ticks_from_Ms(10);
		while (R8_ADC_CONVERT & RB_ADC_START) {
			if ((int64_t)(now_ticks() - deadline) > 0) {
				adc_data = 0;
				goto out_restore;
			}
		}
	}
	adc_data = R16_ADC_DATA;

out_restore:
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
	// NFC CTR
	if (NFCA_ENABLE_CTR_PIN) {
		funPinMode(PA7, GPIO_CFGLR_OUT_2Mhz_PP);
		funDigitalWrite(PA7, FUN_HIGH);
	} else {
		funPinMode(PA7, GPIO_CFGLR_IN_FLOAT);
	}
	R32_PIN_IN_DIS |= PA7; // disable PA7 digital input

	funPinMode( (PB8 | PB9 | PB16 | PB17), GPIO_CFGLR_IN_FLOAT );
	R32_PIN_IN_DIS |= (((PB8 | PB9) & ~PB) << 16); // disable PB8 PB9 digital input
	R16_PIN_CONFIG |= (((PB16 | PB17) & ~PB) >> 8); // gpio alt func

	// Enable RF antenna switch outputs on PB16..PB21 (required on some boards for antenna driver routing).
	R16_PIN_ALTERNATE |= RB_RF_ANT_SW_EN;
}

// NFCA PCD functions
void nfca_pcd_start(void) {
	R8_NFC_CMD = 0x24;
	R32_NFC_DRV &= 0xe7ff;
	// Empirical analog-front-end preset: bits 5/9/10 are used by WCH examples in NFC paths.
	// This can materially affect RX sensitivity on some boards.
	R32_NFC_DRV |= 0x0620;
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
		{
			uint64_t deadline = now_ticks() + Ticks_from_Ms(10);
			while (R8_ADC_CONVERT & (RB_ADC_START | RB_ADC_EOC_X)) {
				if ((int64_t)(now_ticks() - deadline) > 0) {
					adc_value = 0;
					break;
				}
			}
		}
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
		{
			uint64_t deadline = now_ticks() + Ticks_from_Ms(10);
			while (R8_ADC_CONVERT & (RB_ADC_START | RB_ADC_EOC_X)) {
				if ((int64_t)(now_ticks() - deadline) > 0) {
					break;
				}
			}
		}
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

	int processed_bytes = 0;
	int num_chunks = 0;

	// Process the data in 8-byte chunks.
	while ((num_bits - processed_bytes) >= 8) {
		if ((processed_bytes + 8) > output_len) {
			break;
		}

		for (int i = 0; i < 8; ++i) {
			// Combine data and parity into a 16-bit word.
			// If the parity byte is non-zero, the 9th bit is set.
			data_buf[processed_bytes +i] = (uint16_t)send_buf[processed_bytes +i] | (parity_buf[processed_bytes +i] > 0 ? 0x0100 : 0);
		}
		processed_bytes += 8;
		num_chunks++;
	}

	int remaining_len = num_bits - processed_bytes;

	// Process any remaining bytes that did not form a full 8-byte chunk.
	if (remaining_len > 0 && processed_bytes < output_len) {
		data_buf[num_chunks] = (uint16_t)send_buf[num_chunks] | (parity_buf[num_chunks] > 0 ? 0x0100 : 0);
	}

	return ((num_bits / 8) *9) + (num_bits % 8);
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
		g_nfca_pcd_crcerr_seen = 0;
		// Keep the original RX control settings; different CH58x variants
		// interpret these bits differently. We handle CRCERR in software.
		g_nfca_pcd_rx_status_reg = (g_nfca_pcd_intf_mode & 0x10) ? 0x36 : 0x26;

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
		case CMD_T2T_READ:
			// Type 2 Tag / Ultralight / NTAG READ (0x30) does not require anticollision if only one tag is present.
			// TX: 0x30, page (1 byte), CRC_A
			// RX: 16 data bytes + CRC_A
			g_nfca_pcd_send_buf[0] = CMD_T2T_READ;
			g_nfca_pcd_send_buf[1] = (data && len >= 1) ? data[0] : 0;
			len = 2;
			comm_len = (len + 2) * 8;
			ISO14443AAppendCRCA(g_nfca_pcd_send_buf, len);
			ISO14443ACalOddParityBit(g_nfca_pcd_send_buf, g_nfca_parity_buf, len + 2);
			nfca_pcd_wait_ms(PCD_APDU_OVER_TIME);
			break;
			}

		// Configure whether HW CRC checking is expected/desired for this exchange.
		// ATQA and anticollision responses do not include CRC_A.
		g_nfca_pcd_rx_crc_enable = 1;
		g_nfca_pcd_crcerr_is_fatal = 1;
		if ((cmd == CMD_REQA) || (cmd == CMD_WUPA)) {
			g_nfca_pcd_rx_crc_enable = 0;
			g_nfca_pcd_crcerr_is_fatal = 0;
		}
		else if (((cmd == CMD_ANTICOLL1) || (cmd == CMD_ANTICOLL2) || (cmd == CMD_ANTICOLL3)) && (g_nfca_pcd_send_buf[1] == 0x20)) {
			g_nfca_pcd_rx_crc_enable = 0;
			g_nfca_pcd_crcerr_is_fatal = 0;
		}

		if (nfca_pcd_comm(comm_len, NFCA_PCD_REC_MODE_NORMAL, 0) == 0) {
			if(nfca_pcd_wait_comm_end() == NFCA_PCD_CONTROLLER_STATE_DONE) {
				if (g_nfca_pcd_recv_bits % 9 == 0) {
					int nbytes = g_nfca_pcd_recv_bits /9;
					if (ISO14443ACheckOddParityBit(g_nfca_pcd_recv_buf, g_nfca_parity_buf, nbytes)) {
						if ((cmd == CMD_REQA) || (cmd == CMD_WUPA)) {
							// ATQA is exactly 2 bytes (no CRC_A). Guard against treating
							// empty/partial frames as a successful poll.
							res = (nbytes >= 2) ? PCD_NO_ERROR : PCD_FRAME_ERROR;
						}
						else if (((cmd == CMD_ANTICOLL1) || (cmd == CMD_ANTICOLL2) || (cmd == CMD_ANTICOLL3)) && g_nfca_pcd_send_buf[1] == 0x20) {
							// Anticollision response is 5 bytes (UID[4] + BCC), no CRC_A.
							if (nbytes < 5) {
								res = PCD_FRAME_ERROR;
							}
							else if (ISO14443A_CHECK_BCC(g_nfca_pcd_recv_buf)) {
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
		// Only send if no message is currently queued.
		if (!g_usb_tx_len) {
			usb_tx_queue_message((const uint8_t*)version, (uint16_t)strlen(version));
		}
		usb_tx_pump();
	}

	__HIGH_CODE
	void card_interaction(int uid_len, int atqa, int sak) {
	int iso14443_4 = ((sak & 0x20) != 0);
	int iso14443_3_devtest_jcop = (sak == 0x00 && gs_picc_uid[0] == 0x00);
	int iso14443_3_type2 = (sak == 0x00 && gs_picc_uid[0] != 0x00);
	uint64_t timeout = 0;

	if (iso14443_3_type2) {
		ntag_dump_read_only(uid_len);
		return;
	}

	if(iso14443_4 || iso14443_3_devtest_jcop) {
		if(PcdReq(CMD_RATS, 0, 0) == PCD_NO_ERROR) {
			// Send RATS response to PC
			while (g_usb_tx_len) { }
			usb_tx_queue_message((const uint8_t*)g_nfca_pcd_recv_buf, (uint16_t)(g_nfca_pcd_recv_bits / 9));

				// Set a timeout for the PC to reply
				timeout = now_ticks() + Ticks_from_Ms(PCD_APDU_OVER_TIME);

				// While waiting for PC response, we block here because timing is strict
				// for the NFC field. Losing the field usually kills the card state.
				while( (int64_t)(now_ticks() - timeout) < 0 ) {
					usb_tx_pump();
					if( *(volatile uint16_t*)gs_usb_data_buf ) {
						// we got a new APDU to send from PC
						if( PcdReq(CMD_IBLOCK, (uint8_t *)&gs_usb_data_buf[2], ((uint16_t*)gs_usb_data_buf)[0]) == PCD_NO_ERROR ) {
						// Send chained responses
						for(int i = 0; i < NFCA_PCD_CHAINING_BUFFERS; i++) {
								if(g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)]) {
									while (g_usb_tx_len) { }
									usb_tx_queue_message((const uint8_t*)&g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)], (uint16_t)sizeof(g_nfca_pcd_recv_buf));
									g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)] = 0;
								}
							else {
								break;
							}
							}
							// Send final response
							while (g_usb_tx_len) { }
							usb_tx_queue_message((const uint8_t*)g_nfca_pcd_recv_buf, (uint16_t)(g_nfca_pcd_recv_bits / 9));

							// Reset timeout for next command
							timeout = now_ticks() + Ticks_from_Ms(PCD_APDU_OVER_TIME);
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
	uint64_t next_scan_time = 0;
	uint64_t next_heartbeat_time = 0;
	uint8_t saw_card_response = 0;
	uint8_t played_full_feedback = 0;

	// Start heartbeats immediately; don't let ADC bring-up block USB.
	next_heartbeat_time = now_ticks();

	// Check VDD once at start or periodically if needed (best-effort).
	{
		uint16_t vdd_raw = sys_get_vdd();
		int vdd_value = ADC_VoltConverSignalPGA_MINUS_12dB(vdd_raw);
		if (NFCA_FORCE_MAX_DRIVE) {
			NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL3);
		} else {
			if(vdd_raw == 0) {
				// If ADC failed, choose a conservative drive level.
				NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL2);
			}
			else if(vdd_value > 3400)      NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL0);
			else if(vdd_value > 3000) NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL1);
			else if(vdd_value > 2600) NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL2);
			else                      NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL3);
		}
	}

	while(1) {
		// 1. Always process USB input (e.g. Reboot commands) immediately
		process_usb_packet();
		usb_tx_pump();

		// 2. Handle heartbeat (non-blocking)
		if ( (int64_t)(now_ticks() - next_heartbeat_time) > 0 ) {
			send_heartbeat_if_free();
			next_heartbeat_time = now_ticks() + Ticks_from_Ms(1000);
		}

		// 3. Handle NFC Scan (Periodically)
		if ( (int64_t)(now_ticks() - next_scan_time) > 0 ) {
			saw_card_response = 0;
			played_full_feedback = 0;

			nfca_pcd_start();

			// LPCD Check (Fast). Optional bring-up bypass.
			if(NFCA_FORCE_ACTIVE_SCAN || nfca_pcd_lpcd_check()) {

					// Found something!
					// Reset antenna field to wake card
					R8_NFC_CMD &= ~RB_NFC_ANTENNA_ON;
					Delay_Ms(5);
					R8_NFC_CMD |= RB_NFC_ANTENNA_ON;
					Delay_Ms(NFCA_FIELD_SETTLE_MS);

				// --- ANTICOLLISION SEQUENCE ---
				// Try a few quick polls; weak antennas often only get a response on a subset of attempts.
				uint16_t reqa_res = PCD_COMMUNICATE_ERROR;
				for (int attempt = 0; attempt < NFCA_REQA_RETRIES; attempt++) {
					// Prefer WUPA: it also wakes tags in HALT state (common after a previous HLTA).
					uint16_t r = (uint16_t)PcdReq(CMD_WUPA, 0, 0);
					if (r != PCD_NO_ERROR) {
						r = (uint16_t)PcdReq(CMD_REQA, 0, 0);
					}
					reqa_res = r;
					if (r == PCD_NO_ERROR && g_nfca_pcd_recv_bits >= (2 * 9)) break;
					Delay_Us(200);
				}

				if(reqa_res == PCD_NO_ERROR) {
						saw_card_response = 1;
						atqa = ((uint16_t *)(g_nfca_pcd_recv_buf))[0];
						// Go straight to anticollision; sending HLTA here is unnecessary and can reduce reliability.
						Delay_Us(350);
						res = PcdReq(CMD_ANTICOLL1, /*0x20*/0, 2);
						int used_t2t_fallback = 0;

						// Fallback for weak Type 2 / NTAG tags: after REQA/WUPA, a single tag can accept a READ(0)
						// even without completing anticollision. This is a shorter exchange and can succeed when
						// anticollision/select is too marginal.
						if (res != PCD_NO_ERROR) {
							uint8_t page0 = 0;
							if (PcdReq(CMD_T2T_READ, &page0, 1) == PCD_NO_ERROR) {
								int nbytes = (int)(g_nfca_pcd_recv_bits / 9);
								if (nbytes >= 18) { // 16 data bytes + CRC_A (2)
									uint8_t *d = g_nfca_pcd_recv_buf; // includes CRC_A at the end
									uint8_t bcc0 = d[3];
									uint8_t bcc1 = d[8];
									uint8_t calc_bcc0 = (uint8_t)(0x88u ^ d[0] ^ d[1] ^ d[2]);
									uint8_t calc_bcc1 = (uint8_t)(d[4] ^ d[5] ^ d[6] ^ d[7]);

									if (bcc0 == calc_bcc0 && bcc1 == calc_bcc1) {
										uuid_len = 7;
										gs_picc_uid[0] = d[0];
										gs_picc_uid[1] = d[1];
										gs_picc_uid[2] = d[2];
										gs_picc_uid[3] = d[4];
										gs_picc_uid[4] = d[5];
										gs_picc_uid[5] = d[6];
										gs_picc_uid[6] = d[7];
										sak = 0x00; // Type 2 tags typically report SAK 0x00
										used_t2t_fallback = 1;
									}
								}
							}
						}

						if (!used_t2t_fallback && res == PCD_NO_ERROR) {
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
							// Report basic card info (UID/ATQA/SAK) for *any* ISO14443A tag.
							// Payload format:
							// 0: 0xA5 (event marker)
							// 1: uid_len
							// 2..: uid bytes
							// next 2: ATQA little-endian
							// last 1: SAK
							uint8_t info[1 + 1 + 10 + 2 + 1];
							uint8_t info_len = 0;
							info[info_len++] = 0xA5;
							info[info_len++] = (uint8_t)uuid_len;
							memcpy(&info[info_len], gs_picc_uid, uuid_len);
							info_len += (uint8_t)uuid_len;
							info[info_len++] = (uint8_t)(atqa & 0xff);
							info[info_len++] = (uint8_t)(atqa >> 8);
							info[info_len++] = (uint8_t)sak;
							while (g_usb_tx_len) { }
							usb_tx_queue_message(info, info_len);
							usb_tx_pump();

							// Card is fully selected, enter interaction mode
							// This function IS allowed to block because we are in a session
							card_interaction(uuid_len, atqa, sak);

							// Clean up the NFC side before we touch audio / SK6812.
							PcdReq(CMD_HALT, 0, 0);
							nfca_pcd_stop();
							tag_feedback((uint8_t)uuid_len, (uint16_t)atqa, (uint8_t)sak);
							played_full_feedback = 1;
							atqa = 0;
							sak = 0;
							uuid_len = 0;
							next_scan_time = now_ticks() + Ticks_from_Ms(333);
							continue;
						}
				}
			}

			// Turn off NFC to save power/reset state
			nfca_pcd_stop();
			if (saw_card_response && !played_full_feedback) {
				audio_play_detect_chirp();
			}

			// Schedule next scan for 333ms from NOW
			next_scan_time = now_ticks() + Ticks_from_Ms(333);
		}
	}
}

int main() {
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx
	GPIOSetup();
	audio_play_boot_debug_beep();
	usb_rx_reset_assembly();
	USBFSSetup();

#if !SK6812_DISABLE
	sk6812_set_rgb(SK6812_BRIGHT, 0, 0);
#endif

	// Prove USB is alive even if later bring-up (ADC/NFC) hangs.
	usb_tx_queue_message((const uint8_t*)"nfca585 boot", (uint16_t)strlen("nfca585 boot"));
	usb_tx_pump();

	blink(5); // Indicates boot

#if !SK6812_DISABLE
	sk6812_set_rgb(0, SK6812_BRIGHT, 0);
#endif

	nfca_init();
	nfca_pcd_lpcd_calibration();

	main_task_loop(); // Never returns

	while(1);
}
