﻿#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stdint.h>
#include "External\ps5000aApi.h"

// Definitions
//
// Password to unlock non-volatile area for write
#define ENABLE_LOCKING				false
#define UNLOCK_PWD_1				1
#define UNLOCK_PWD_2				1
#define UNLOCK_PWD_3				1
#define UNLOCK_PWD_4				1
//
// Global miscellaneous parameters
#define TIMER_PERIOD				10					// (in ms)
#define	SCCI_TIMEOUT_TICKS			1000				// (in ms)
#define DT_EPROM_ADDRESS			0
#define EP_READ_COUNT				5
#define EP_WRITE_COUNT				1
//
// Measurement mode
#define MODE_QRR					0
#define MODE_QRR_TQ					1
//
// Sampling settings
#define SAMPLING_I_CHANNEL			PS5000A_CHANNEL_A	// Sampling current channel
#define SAMPLING_V_CHANNEL			PS5000A_CHANNEL_B	// Sampling voltage channel
#define SAMPLING_I_DEF_RANGE		PS5000A_1V			// Default sampling current range
#define SAMPLING_V_RANGE			PS5000A_10V			// Sampling voltage range
#define SAMPLING_RESOLUTION			PS5000A_DR_15BIT	// Sampling resolution
#define SAMPLING_TIME_BASE			3					// Sampling timebase
#define SAMPLING_TIME_FRACTION		0.008f				// Sampling time fraction (in us)
#define SAMPLING_SAMPLES			375000L				// Number of samples in block mode
#define SAMPLING_EXT_POWER			false				// Use external power supply
#define SAMPLING_I_INVERT_INPUT		true				// Invert sampled values
#define SAMPLING_V_COEFFICIENT		3753.0f				// Convert ADC result to voltage
//
// Trigger settings
#define TRIGGER_SOURCE				PS5000A_EXTERNAL	// Trigger source channel
#define TRIGGER_MODE				PS5000A_RISING		// Triggering mode
#define TRIGGER_LEVEL				2000				// In ticks of 16bit range, signed value
//
// Serial port
#define SERIAL_READ_TIMEOUT			50					// Read timeout constant (in ms)
#define SERIAL_WRITE_TIMEOUT		50					// Write timeout constant (in ms)
#define SERIAL_BUFFER_LEN			64
//
// Calculation
#define IRR_SEARCH_WND				50					// Search window for minimum value
#define OSV_FALL_DETECT_V			-50.0f				// Voltage fall detect (in V)
#define OSV_RISE_DETECT_V			-5.0f				// Voltage rise detect (in V)
#define MUL_FACTOR_I				4					// Scope range multiply factor (I)
#define MUL_FACTOR_V				2					// Scope range multiply factor (V)

#endif	// __GLOBAL_H__
