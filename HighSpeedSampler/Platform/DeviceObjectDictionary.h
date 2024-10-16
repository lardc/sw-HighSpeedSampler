#ifndef __DEV_OBJ_DIC_H
#define __DEV_OBJ_DIC_H


// ACTIONS
//
#define ACT_CLR_FAULT				3	// Clear fault (try switch state from FAULT to NONE)
#define ACT_CLR_WARNING				4	// Clear warning flag
//
#define ACT_DIAG_GEN_READ_EP		10	// Fill diagnostic endpoint with values
#define ACT_DIAG_SAVE_TO_FILE		11	// Save data to file
#define ACT_DIAG_READ_FRAGMENT		12	// Read fragment of the full array
#define ACT_DIAG_READ_RAW_FRAGMENT	13	// Read fragment of the raw full array
//
#define ACT_START_TEST				100	// Start test with defined parameters
#define ACT_STOP_TEST				101	// Stop test
//
#define ACT_SAVE_TO_ROM				200	// Save parameters to EEPROM module
#define ACT_RESTORE_FROM_ROM		201	// Restore parameters from EEPROM module
#define ACT_RESET_TO_DEFAULT		202	// Reset parameters to default values (only in controller memory)
#define ACT_LOCK_NV_AREA			203	// Lock modifications of parameters area
#define ACT_UNLOCK_NV_AREA			204	// Unlock modifications of parameters area (password-protected)


// REGISTERS
//
#define REG_SHUNT_RES_N				0	// Shunt resistance (in mOhms) (N)
#define REG_SHUNT_RES_D				1	// Shunt resistance (in mOhms) (D)
#define REG_I_FINE_N				2	// Current fine tune K (N)
#define REG_I_FINE_D				3	// Current fine tune K (D)
#define REG_I_FINE_OFFSET			4	// Current fine offset (x10 in A)
#define REG_INVERT_CURRENT			5	// Enable current inversion
#define REG_V_FINE_N				6	// Voltage fine tune K (N)
#define REG_V_FINE_D				7	// Voltage fine tune K (D)
#define REG_V_FINE_OFFSET			8	// Voltage fine offset (x10 in V)
#define REG_VOLTAGE_DIV_N			9	// Voltage divider coefficient (N)
#define REG_VOLTAGE_DIV_D			10	// Voltage divider coefficient (D)
//
#define REG_SP__1					127
//
// ----------------------------------------
//
#define REG_CURRENT_AMPL			128	// Current amplitude (in A)
#define REG_MEASURE_MODE			129	// Select sampling mode (Qrr or Qrr-tq)
#define REG_TR_050_METHOD			130	// Use 50% level of Irr to detect tr time
#define REG_VOLTAGE_AMPL			131	// Voltage amplitude (in V)
//
#define REG_DIAG_FORCE_SECTOR_READ	150	// Force read of defined time sector from scopes (in us)
#define REG_DIAG_FULL_ARR_SCALE		151	// Scale factor for full-array read
//
#define REG_PWD_1					180	// Unlock password location 1
#define REG_PWD_2					181	// Unlock password location 2
#define REG_PWD_3					182	// Unlock password location 3
#define REG_PWD_4					183	// Unlock password location 4
//
#define REG_SP__2					191
//
// ----------------------------------------
//
#define REG_DEV_STATE				192	// Device state
#define REG_FAULT_REASON			193	// Fault reason in the case DeviceState -> FAULT
#define REG_DISABLE_REASON			194	// Fault reason in the case DeviceState -> DISABLED
#define REG_WARNING					195	// Warning if present
#define REG_PROBLEM					196	// Problem reason
#define REG_OP_RESULT				197	// Operation result
//
#define REG_DF_REASON_EX			200	// Fault or disable extended reason
#define REG_RESULT_IRR				201	// Reverse recovery current amplitude (in A)
#define REG_RESULT_TRR				202	// Reverse recovery time (in us x10)
#define REG_RESULT_QRR				203	// Reverse recovery charge (in uQ x10)
#define REG_RESULT_ZERO				204	// Zero-cross time (in us x10)
#define REG_RESULT_ZERO_V			205	// Zero-cross time for on-state voltage (in us x10)
#define REG_RESULT_DIDT				206	// Actual value of dIdt (in A/us x10)
#define REG_RESULT_ID				207	// Direct current amplitude (in A)
#define REG_RESULT_VD				208	// Direct voltage amplitude (in V)
#define REG_RESULT_QRR_B32			209	// Reverse recovery charge (in uQ x10) 32bit part
//
#define REG_EP_ELEMENTARY_FRACT		220	// Elementary fraction length in ns
#define REG_EP_STEP_FRACTION_CNT	221	// Number of elementary fractions in the EP single step
//
#define REG_SP__3					255


// ENDPOINTS
//
#define EP_READ_I					1	// Read endpoint for I value sequence
#define EP_READ_V					2	// Read endpoint for V value sequence
#define EP_READ_DIAG				3	// Diagnostic read endpoint
#define EP_READ_FULL_I_PART			4	// Read a part of full array of I values
#define EP_READ_FULL_V_PART			5	// Read a part of full array of V values
#define EP_WRITE					1	// Write endpoint for debug operations

// OPERATION RESULTS
//
#define OPRESULT_NONE				0	// No information or not finished
#define OPRESULT_OK					1	// Operation was successful
#define OPRESULT_FAIL				2	// Operation failed

// PROBLEM CODES
//
#define PROBLEM_NONE				0	// No problem
#define PROBLEM_CALC_IRR			1	// Problem calculating Irr
#define PROBLEM_CALC_IRR_025		2	// Problem calculating 25% fraction Irr
#define PROBLEM_CALC_IRR_090		3	// Problem calculating 90% fraction Irr
#define PROBLEM_CALC_VZ				4	// Problem calculating V zero crossing
#define PROBLEM_CALC_DIDT			5	// Problem calculating actual dIdt

// FAULT & DISABLE
//
#define DF_NONE						0	// No faults
#define DF_PICOSCOPE				1	// Picoscope error

// WARNING CODES
//
#define WARNING_NONE				0	// No warning

// USER ERROR CODES
//
#define ERR_NONE					0	// No error
#define ERR_CONFIGURATION_LOCKED	1	// Device is locked for writing
#define ERR_OPERATION_BLOCKED		2	// Operation can't be done due to current device state
#define ERR_DEVICE_NOT_READY		3	// Device isn't ready to switch state
#define ERR_WRONG_PWD				4	// Wrong password - unlock failed


#endif // __DEV_OBJ_DIC_H
