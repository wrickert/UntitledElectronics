#include "ch32fun.h"
#include "hsusb.h"

#define LED PA8

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

__HIGH_CODE
void blink(int n) {
	for(int i = n-1; i >= 0; i--) {
		funDigitalWrite( LED, FUN_LOW ); // Turn on LED
		Delay_Ms(33);
		funDigitalWrite( LED, FUN_HIGH ); // Turn off LED
		if(i) Delay_Ms(33);
	}
}

__INTERRUPT
void GPIOB_IRQHandler() {
	int status = R16_PB_INT_IF;
	R16_PB_INT_IF = status; // acknowledge

	if(status & PB8) { // PB22 interrupt comes in on PB8 if RB_PIN_INTX is set
		USBHSReset();
		blink(2);
		jump_isprom();
	}
}

void GPIOSetup() {
	funPinMode( LED, GPIO_CFGLR_OUT_2Mhz_PP );
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
		ctx->USBHS_SetupReqLen = 0; // To ACK
	}
	else if( endp == USB_EP_RX ) {
		if(len == 4 && ((uint32_t*)data)[0] == 0x010001a2) {
			USBHSReset();
			blink(2);
			jump_isprom();
		}
		else if( ((uint16_t*)gs_usb_data_buf)[0] == 0 ) {
			((uint16_t*)gs_usb_data_buf)[0] = len;
			mcpy_raw(&gs_usb_data_buf[2], data, data +len);
		}
		else {
			// previous usb buffer not consumed yet, what should we do?
		}

		ctx->USBHS_Endp_Busy[USB_EP_RX] = 0;
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
			uint16_t received_words = (g_nfca_pcd_recv_bits + 15) / 16;

			// Drain any final words left in the FIFO
			if (g_nfca_pcd_recv_word_idx < received_words) {
				uint16_t words_to_drain = received_words - g_nfca_pcd_recv_word_idx;
				for (int i = 0; i < words_to_drain; i++) {
					gs_nfca_data_buf[g_nfca_pcd_recv_word_idx++] = R16_NFC_FIFO;
				}
			}
			g_nfca_pcd_comm_status = 5; // Status: Success
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
		}
		else if (intf_status & 0x20) {
			g_nfca_pcd_comm_status = 4; // Status: CRC Error
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
	}

	if (nfca_pcd_comm(comm_len, NFCA_PCD_REC_MODE_NORMAL, 0) == 0) {
		if(nfca_pcd_wait_comm_end() == NFCA_PCD_CONTROLLER_STATE_DONE) {
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
	if( !(USBHSCTX.USBHS_Endp_Busy[USB_EP_TX] & 1) ) {
		USBHS_SendEndpointNEW( USB_EP_TX, (uint8_t*)version, strlen(version), 1 );
	}
}

__HIGH_CODE
int usb_send_blocking(const uint8_t *data, uint16_t len, uint32_t timeout_ms) {
	uint32_t timeout = SysTick->CNTL + Ticks_from_Ms(timeout_ms);

	while (USBHSCTX.USBHS_Endp_Busy[USB_EP_TX] & 1) {
		if (SysTick->CNTL >= timeout) {
			return 0;
		}
	}

	USBHS_SendEndpointNEW(USB_EP_TX, (uint8_t *)data, len, 1);
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
			USBHS_SendEndpointNEW(USB_EP_TX, g_nfca_pcd_recv_buf, g_nfca_pcd_recv_bits /9, 1);

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
								USBHS_SendEndpointNEW(USB_EP_TX, &g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)], sizeof(g_nfca_pcd_recv_buf), 1);
								g_nfca_pcd_recv_buf_chain[i * sizeof(g_nfca_pcd_recv_buf)] = 0;
							}
							else {
								break;
							}
						}
						// Send final response
						USBHS_SendEndpointNEW(USB_EP_TX, g_nfca_pcd_recv_buf, g_nfca_pcd_recv_bits /9, 1);

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

	// Check VDD once at start or periodically if needed
	int vdd_value = ADC_VoltConverSignalPGA_MINUS_12dB( sys_get_vdd() );
	if(vdd_value > 3400)      NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL0);
	else if(vdd_value > 3000) NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL1);
	else if(vdd_value > 2600) NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL2);
	else                      NFCA_PCD_SET_OUT_DRV(NFCA_PCD_DRV_CTRL_LEVEL3);

	while(1) {
		// 1. Always process USB input (e.g. Reboot commands) immediately
		process_usb_packet();

		// 2. Handle Heartbeat (Non-blocking)
		if (SysTick->CNTL > next_heartbeat_time) {
			send_heartbeat_if_free();
			next_heartbeat_time = SysTick->CNTL + Ticks_from_Ms(333);
		}

		// 3. Handle NFC Scan (Periodically)
		if (SysTick->CNTL > next_scan_time) {

			nfca_pcd_start();

			// LPCD Check (Fast)
			if(nfca_pcd_lpcd_check()) {

				// Found something!
				// Reset antenna field to wake card
				R8_NFC_CMD &= ~RB_NFC_ANTENNA_ON;
				Delay_Ms(5);
				R8_NFC_CMD |= RB_NFC_ANTENNA_ON;
				Delay_Ms(5);

				// --- ANTICOLLISION SEQUENCE ---
				if(PcdReq(CMD_REQA, 0, 0) == PCD_NO_ERROR) {
					atqa = ((uint16_t *)(g_nfca_pcd_recv_buf))[0];
					PcdReq(CMD_HALT, 0, 0);

					if(PcdReq(CMD_WUPA, 0, 0) == PCD_NO_ERROR) {
						atqa = ((uint16_t *)(g_nfca_pcd_recv_buf))[0];
						Delay_Us(350);
					res = PcdReq(CMD_ANTICOLL1, /*0x20*/0, 2);
					}

					if (res == PCD_NO_ERROR) {
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
						atqa = 0;
						sak = 0;
						uuid_len = 0;
					}
				}
			}

			// Turn off NFC to save power/reset state
			nfca_pcd_stop();

			// Schedule next scan for 333ms from NOW
			next_scan_time = SysTick->CNTL + Ticks_from_Ms(333);
		}
	}
}

int main() {
	SystemInit();

	funGpioInitAll(); // no-op on ch5xx
	GPIOSetup();
	USBHSSetup();

	blink(5); // Indicates boot

	nfca_init();
	nfca_pcd_lpcd_calibration();

	main_task_loop(); // Never returns

	while(1);
}
