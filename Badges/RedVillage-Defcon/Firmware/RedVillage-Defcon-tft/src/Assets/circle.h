// We need this header file to use FLASH as storage with PROGMEM directive:
#include <pgmspace.h>

// Icon width and height
const uint16_t circleWidth = 32;
const uint16_t circleHeight = 32;

const unsigned short  circle[1024] PROGMEM={
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0010 (16) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0020 (32) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xAD55, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514,   // 0x0030 (48) pixels
0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xAD55, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0040 (64) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x9CD3, 0x9CF3, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514,   // 0x0050 (80) pixels
0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xAD55, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0060 (96) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA4D3,   // 0x0070 (112) pixels
0xA4D3, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0080 (128) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8410, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xB430, 0xD2AA, 0xE986, 0xF924, 0xF924,   // 0x0090 (144) pixels
0xF945, 0xF9A6, 0xEA49, 0xD36D, 0xB471, 0xA514, 0xA514, 0xA514, 0xA514, 0x9CF3, 0x8410, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x00A0 (160) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x8410, 0xA514, 0xA514, 0xA514, 0xA514, 0xB430, 0xE1A6, 0xF8A2, 0xF8C3, 0xF904, 0xF924, 0xF965,   // 0x00B0 (176) pixels
0xF986, 0xF9C7, 0xF9E7, 0xFA28, 0xFA49, 0xE32C, 0xB492, 0xA514, 0xA514, 0xA514, 0xA514, 0x8410, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x00C0 (192) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0xA514, 0xA514, 0xA514, 0xA514, 0xC32C, 0xF882, 0xF8A2, 0xF8E3, 0xF904, 0xF945, 0xF965, 0xF9A6,   // 0x00D0 (208) pixels
0xF9C7, 0xFA08, 0xFA28, 0xFA69, 0xFA8A, 0xFACB, 0xFB0C, 0xCC30, 0xA514, 0xA514, 0xA514, 0xA514, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x00E0 (224) pixels
0x0000, 0x0000, 0x0000, 0xA514, 0xA514, 0xA514, 0xA514, 0xD269, 0xF882, 0xF8C3, 0xF8E3, 0xF924, 0xF945, 0xF986, 0xF9A6, 0xF9C7,   // 0x00F0 (240) pixels
0xFA08, 0xFA28, 0xFA69, 0xFA8A, 0xFACB, 0xFAEB, 0xFB2C, 0xFB4D, 0xD430, 0xA514, 0xA514, 0xA514, 0xA514, 0x0000, 0x0000, 0x0000,   // 0x0100 (256) pixels
0x0000, 0x0000, 0xAD55, 0xA514, 0xA514, 0xA514, 0xCB0C, 0xF882, 0xF8C3, 0xF8E3, 0xF924, 0xF945, 0xF986, 0xF9A6, 0xF9E7, 0xFA08,   // 0x0110 (272) pixels
0xFA49, 0xFA69, 0xFAAA, 0xFACB, 0xFB0C, 0xFB2C, 0xFB6D, 0xFB8E, 0xFBCF, 0xC492, 0xA514, 0xA514, 0xA514, 0x9CD3, 0x0000, 0x0000,   // 0x0120 (288) pixels
0x0000, 0x0000, 0x9CF3, 0xA514, 0xA514, 0xB430, 0xF8C3, 0xF8C3, 0xF904, 0xF924, 0xF965, 0xF986, 0xF9C7, 0xF9E7, 0xFA28, 0xFA49,   // 0x0130 (304) pixels
0xFA8A, 0xFAAA, 0xFAEB, 0xFB0C, 0xFB4D, 0xFB6D, 0xFBAE, 0xFBCF, 0xFC10, 0xFC30, 0xB4F3, 0xA514, 0xA514, 0xA514, 0x0000, 0x0000,   // 0x0140 (320) pixels
0x0000, 0xAD55, 0xA514, 0xA514, 0xA514, 0xE1E7, 0xF8E3, 0xF904, 0xF945, 0xF965, 0xF9A6, 0xF9C7, 0xFA08, 0xFA28, 0xFA69, 0xFA8A,   // 0x0150 (336) pixels
0xFACB, 0xFAEB, 0xFB2C, 0xFB4D, 0xFB8E, 0xFBAE, 0xFBEF, 0xFC10, 0xFC51, 0xFC71, 0xE4B2, 0xA514, 0xA514, 0xA514, 0xAD55, 0x0000,   // 0x0160 (352) pixels
0x0000, 0xA514, 0xA514, 0xA514, 0xB430, 0xF8E3, 0xF924, 0xF945, 0xF986, 0xF9A6, 0xF9E7, 0xFA08, 0xFA49, 0xFA69, 0xFAAA, 0xFACB,   // 0x0170 (368) pixels
0xFB0C, 0xFB2C, 0xFB6D, 0xFB8E, 0xFBCF, 0xFBEF, 0xFC30, 0xFC51, 0xFC71, 0xFCB2, 0xFCD3, 0xB514, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x0180 (384) pixels
0x0000, 0xA514, 0xA514, 0xA514, 0xD2EB, 0xF924, 0xF965, 0xF986, 0xF9C7, 0xF9E7, 0xFA28, 0xFA49, 0xFA8A, 0xFAAA, 0xFACB, 0xFB0C,   // 0x0190 (400) pixels
0xFB2C, 0xFB6D, 0xFB8E, 0xFBCF, 0xFBEF, 0xFC30, 0xFC51, 0xFC92, 0xFCB2, 0xFCF3, 0xFD14, 0xD534, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x01A0 (416) pixels
0x0000, 0x9CF3, 0xA514, 0xA514, 0xE9E7, 0xF965, 0xF986, 0xF9C7, 0xF9E7, 0xFA28, 0xFA49, 0xFA8A, 0xFAAA, 0xFAEB, 0xFB0C, 0xFB4D,   // 0x01B0 (432) pixels
0xFB6D, 0xFBAE, 0xFBCF, 0xFC10, 0xFC30, 0xFC71, 0xFC92, 0xFCD3, 0xFCF3, 0xFD34, 0xFD55, 0xED75, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x01C0 (448) pixels
0x0000, 0xA514, 0xA514, 0xA514, 0xF9A6, 0xF9A6, 0xF9C7, 0xFA08, 0xFA28, 0xFA69, 0xFA8A, 0xFACB, 0xFAEB, 0xFB2C, 0xFB4D, 0xFB8E,   // 0x01D0 (464) pixels
0xFBAE, 0xFBEF, 0xFC10, 0xFC51, 0xFC71, 0xFCB2, 0xFCD3, 0xFD14, 0xFD34, 0xFD75, 0xFD96, 0xFDB6, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x01E0 (480) pixels
0x0000, 0xA514, 0xA514, 0xA4D3, 0xF9A6, 0xF9E7, 0xFA08, 0xFA49, 0xFA69, 0xFAAA, 0xFACB, 0xFB0C, 0xFB2C, 0xFB6D, 0xFB8E, 0xFBCF,   // 0x01F0 (496) pixels
0xFBEF, 0xFC30, 0xFC51, 0xFC92, 0xFCB2, 0xFCF3, 0xFD14, 0xFD55, 0xFD75, 0xFDB6, 0xFDD7, 0xFE18, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x0200 (512) pixels
0x0000, 0xA514, 0xA514, 0xA4F3, 0xF9E7, 0xFA28, 0xFA49, 0xFA8A, 0xFAAA, 0xFAEB, 0xFB0C, 0xFB4D, 0xFB6D, 0xFBAE, 0xFBCF, 0xFC10,   // 0x0210 (528) pixels
0xFC30, 0xFC71, 0xFC92, 0xFCD3, 0xFCF3, 0xFD34, 0xFD55, 0xFD96, 0xFDB6, 0xFDD7, 0xFE18, 0xFE38, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x0220 (544) pixels
0x0000, 0xA514, 0xA514, 0xA514, 0xFA49, 0xFA69, 0xFA8A, 0xFACB, 0xFAEB, 0xFB2C, 0xFB4D, 0xFB8E, 0xFBAE, 0xFBEF, 0xFC10, 0xFC30,   // 0x0230 (560) pixels
0xFC71, 0xFC92, 0xFCD3, 0xFCF3, 0xFD34, 0xFD55, 0xFD96, 0xFDB6, 0xFDF7, 0xFE18, 0xFE59, 0xFE79, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x0240 (576) pixels
0x0000, 0xA514, 0xA514, 0xA514, 0xEAEB, 0xFA8A, 0xFACB, 0xFAEB, 0xFB2C, 0xFB4D, 0xFB8E, 0xFBAE, 0xFBEF, 0xFC10, 0xFC51, 0xFC71,   // 0x0250 (592) pixels
0xFCB2, 0xFCD3, 0xFD14, 0xFD34, 0xFD75, 0xFD96, 0xFDD7, 0xFDF7, 0xFE38, 0xFE59, 0xFE9A, 0xEE79, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x0260 (608) pixels
0x0000, 0xA514, 0xA514, 0xA514, 0xD3CF, 0xFACB, 0xFB0C, 0xFB2C, 0xFB6D, 0xFB8E, 0xFBCF, 0xFBEF, 0xFC30, 0xFC51, 0xFC92, 0xFCB2,   // 0x0270 (624) pixels
0xFCF3, 0xFD14, 0xFD55, 0xFD75, 0xFDB6, 0xFDD7, 0xFE18, 0xFE38, 0xFE79, 0xFE9A, 0xFEDB, 0xD5F7, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x0280 (640) pixels
0x0000, 0x9CF3, 0xA514, 0xA514, 0xB492, 0xFB0C, 0xFB4D, 0xFB6D, 0xFBAE, 0xFBCF, 0xFC10, 0xFC30, 0xFC71, 0xFC92, 0xFCD3, 0xFCF3,   // 0x0290 (656) pixels
0xFD34, 0xFD55, 0xFD96, 0xFDB6, 0xFDF7, 0xFE18, 0xFE59, 0xFE79, 0xFEBA, 0xFEDB, 0xFF1C, 0xB575, 0xA514, 0xA514, 0xA514, 0x0000,   // 0x02A0 (672) pixels
0x0000, 0xAD55, 0xA514, 0xA514, 0xA514, 0xE3CF, 0xFB8E, 0xFBAE, 0xFBEF, 0xFC10, 0xFC51, 0xFC71, 0xFCB2, 0xFCD3, 0xFD14, 0xFD34,   // 0x02B0 (688) pixels
0xFD75, 0xFD96, 0xFDD7, 0xFDF7, 0xFE38, 0xFE59, 0xFE9A, 0xFEBA, 0xFEFB, 0xFF1C, 0xE69A, 0xA514, 0xA514, 0xA514, 0x8410, 0x0000,   // 0x02C0 (704) pixels
0x0000, 0x0000, 0xA514, 0xA514, 0xA514, 0xB4D3, 0xFBCF, 0xFBEF, 0xFC30, 0xFC51, 0xFC92, 0xFCB2, 0xFCF3, 0xFD14, 0xFD55, 0xFD75,   // 0x02D0 (720) pixels
0xFD96, 0xFDD7, 0xFDF7, 0xFE38, 0xFE59, 0xFE9A, 0xFEBA, 0xFEFB, 0xFF1C, 0xFF3C, 0xB575, 0xA514, 0xA514, 0xA514, 0x0000, 0x0000,   // 0x02E0 (736) pixels
0x0000, 0x0000, 0x9CF3, 0xA514, 0xA514, 0xA514, 0xCC92, 0xFC30, 0xFC51, 0xFC92, 0xFCB2, 0xFCF3, 0xFD14, 0xFD55, 0xFD75, 0xFDB6,   // 0x02F0 (752) pixels
0xFDD7, 0xFE18, 0xFE38, 0xFE79, 0xFE9A, 0xFEDB, 0xFEFB, 0xFF3C, 0xFF5D, 0xC618, 0xA514, 0xA514, 0xA514, 0x9CF3, 0x0000, 0x0000,   // 0x0300 (768) pixels
0x0000, 0x0000, 0x0000, 0xA514, 0xA514, 0xA514, 0xA514, 0xD4B2, 0xFC92, 0xFCD3, 0xFCF3, 0xFD34, 0xFD55, 0xFD96, 0xFDB6, 0xFDF7,   // 0x0310 (784) pixels
0xFE18, 0xFE59, 0xFE79, 0xFEBA, 0xFEDB, 0xFF1C, 0xFF3C, 0xFF7D, 0xD679, 0xA514, 0xA514, 0xA514, 0x9CF3, 0x0000, 0x0000, 0x0000,   // 0x0320 (800) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x9CF3, 0xA514, 0xA514, 0xA514, 0xC4F3, 0xFD14, 0xFD34, 0xFD75, 0xFD96, 0xFDD7, 0xFDF7, 0xFE38,   // 0x0330 (816) pixels
0xFE59, 0xFE9A, 0xFEBA, 0xFEFB, 0xFF1C, 0xFF5D, 0xFF7D, 0xCE38, 0xA514, 0xA514, 0xA514, 0xA514, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0340 (832) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x8410, 0xA514, 0xA514, 0xA514, 0xA514, 0xB514, 0xE555, 0xFDB6, 0xFDD7, 0xFE18, 0xFE38, 0xFE79,   // 0x0350 (848) pixels
0xFE9A, 0xFEDB, 0xFEFB, 0xFF3C, 0xFF5D, 0xE6DB, 0xB575, 0xA514, 0xA514, 0xA514, 0xA514, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0360 (864) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8410, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xB534, 0xD596, 0xEE18, 0xFE59, 0xFE9A,   // 0x0370 (880) pixels
0xFEDB, 0xFEDB, 0xEEBA, 0xD638, 0xB575, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0380 (896) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514,   // 0x0390 (912) pixels
0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x03A0 (928) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x9CF3, 0x9CF3, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514,   // 0x03B0 (944) pixels
0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0x9CF3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x03C0 (960) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8410, 0xA514, 0xA514, 0xA514, 0xA514, 0xA514,   // 0x03D0 (976) pixels
0xA514, 0xA514, 0xA514, 0xA514, 0xA514, 0x8410, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x03E0 (992) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x03F0 (1008) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0400 (1024) pixels
};