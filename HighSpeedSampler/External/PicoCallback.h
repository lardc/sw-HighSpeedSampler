#pragma once

#include <stdint.h>

#include "PicoDeviceEnums.h"
#include "PicoDeviceStructs.h"

#ifdef WIN32
#define PREF5 __stdcall
#else
#define PREF5
#endif

typedef void (PREF5 * PicoUpdateFirmwareProgress)
(
	int16_t        handle,
	uint16_t			 progress
	);

typedef void (PREF5 *PicoProbeInteractions)
(
  int16_t    											handle,
  PICO_STATUS											status,
  PICO_USER_PROBE_INTERACTIONS *	probes,
  uint32_t												nProbes
);

typedef void (PREF5 *PicoDataReadyUsingReads)
(
  int16_t    					handle,
  PICO_READ_SELECTION read,
  PICO_STATUS					status,
  uint64_t						fromSegmentIndex,
  uint64_t						toSegmentIndex,
  PICO_POINTER				pParameter
  );

typedef void (PREF5 *PicoExternalReferenceInteractions)
(
  int16_t    											handle,
  PICO_STATUS											status,
  PICO_CLOCK_REFERENCE            reference
  );
  
typedef void (PREF5 *PicoAWGOverrangeInteractions)
(
  int16_t    											handle,
  PICO_STATUS											status
  );
