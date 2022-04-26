#ifndef __GLOBAL_H__
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
#define DIAG_EMULATE_SCOPES			false
//
#define SCOPE_CURRENT_ONLY			true				// Use single oscilloscope for current channel
//
// Measurement mode
#define MODE_QRR					0
#define MODE_QRR_TQ					1
//
// Sampling settings
#define SAMPLING_ACTIVE_CHANNEL		PS5000A_CHANNEL_A	// Sampling active channel
#define SAMPLING_OFF_CHANNEL		PS5000A_CHANNEL_B	// Turned off channel channel
#define SAMPLING_DEFAULT_RANGE		PS5000A_20V			// Default sampling current range
#define SAMPLING_RESOLUTION			PS5000A_DR_15BIT	// Sampling resolution
#define SAMPLING_TIME_BASE			3					// Sampling timebase
#define SAMPLING_TIME_FRACTION		0.008f				// Sampling time fraction (in us)
#define SAMPLING_SAMPLES			375000L				// Number of samples in block mode
#define SAMPLING_SAFE_RANGE_RATIO	1.2f				// Safe zone relative to amplitude
#define SAMPLING_QRR_VR				-100.0f				// Normal reverse recovery voltage for Qrr-only mode
//
// Trigger settings
#define TRIGGER_SOURCE				PS5000A_EXTERNAL	// Trigger source channel
#define TRIGGER_MODE				PS5000A_RISING		// Triggering mode
#define TRIGGER_LEVEL				2000				// In ticks of 16bit range, signed value
//
// Serial port
#define TIMER_FAST_PERIOD			20
#define TIMER_SLOW_PERIOD			50
#define SERIAL_READ_TIMEOUT			0					// Read timeout constant (in ms)
#define SERIAL_WRITE_TIMEOUT		0					// Write timeout constant (in ms)
#define SERIAL_BUFFER_LEN			64
//
// Calculation
#define IRR_SEARCH_WND				800					// Search window for minimum value
#define OSV_PEAK_DETECT_V			50.0f				// Off-state minimum voltage (in V)
#define MUL_FACTOR_I				4					// Scope range multiply factor (I)
#define MUL_FACTOR_V				2					// Scope range multiply factor (V)
//
// V/I endpoints multiplication
#define EP_VOLTAGE_MUL				1
#define EP_CURRENT_MUL				10

#endif	// __GLOBAL_H__
