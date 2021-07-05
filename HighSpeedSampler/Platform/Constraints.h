// -----------------------------------------
// Constraints for tunable parameters
// ----------------------------------------

#ifndef __CONSTRAINTS_H
#define __CONSTRAINTS_H

// Include
#include <stdint.h>
//
#include "DataTable.h"
#include "Controller\Global.h"

// Types
//
typedef struct __TableItemConstraint
{
	uint16_t Min;
	uint16_t Max;
	uint16_t Default;
} TableItemConstraint;


// Restrictions
//
#define X_D_DEF0				10
#define X_D_DEF1				100
#define X_D_DEF2				1000
#define X_D_DEF3				10000
//
#define CURRENT_AMPL_MIN		10		// in A
#define CURRENT_AMPL_MAX		500		// in A
#define CURRENT_AMPL_DEF		200		// in A
//
#define VOLTAGE_AMPL_MIN		100		// in V
#define VOLTAGE_AMPL_MAX		4500	// in V
#define VOLTAGE_AMPL_DEF		2000	// in V


// Variables
//
extern const TableItemConstraint NVConstraint[DATA_TABLE_NV_SIZE];
extern const TableItemConstraint VConstraint[DATA_TABLE_WP_START - DATA_TABLE_WR_START];


#endif // __CONSTRAINTS_H
