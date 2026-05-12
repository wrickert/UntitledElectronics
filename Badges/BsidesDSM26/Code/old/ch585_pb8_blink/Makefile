all : flash

TARGET:=ch585_pb8_blink
TARGET_MCU:=CH585
# WCH CH585 eval board is typically the QFN48 (CH585M). Override if needed:
TARGET_MCU_PACKAGE?=CH585M

include ../ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean

