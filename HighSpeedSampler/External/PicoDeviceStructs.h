#ifndef __PICO_DEVICE_STRUCTS_H__
#define __PICO_DEVICE_STRUCTS_H__

#include <stdint.h>

#include "PicoConnectProbes.h"
#include "PicoDeviceEnums.h"
#include "PicoStatus.h"

#pragma pack(push,1)
typedef struct tPicoTriggerInfo
{
	PICO_STATUS	status;
	uint64_t	segmentIndex;
	uint64_t	triggerIndex;
	double		triggerTime;
	PICO_TIME_UNITS	timeUnits;
	uint64_t	missedTriggers;
	uint64_t	timeStampCounter;
} PICO_TRIGGER_INFO;

typedef struct tPicoTriggerChannelProperties
{
	int16_t								thresholdUpper;
	uint16_t							thresholdUpperHysteresis;
	int16_t								thresholdLower;
	uint16_t							thresholdLowerHysteresis;
	PICO_CHANNEL					channel;

} PICO_TRIGGER_CHANNEL_PROPERTIES;

typedef struct tPicoCondition
{
	PICO_CHANNEL   		source;
	PICO_TRIGGER_STATE condition;
} PICO_CONDITION;

typedef struct tPicoDirection
{
	PICO_CHANNEL							channel;
	PICO_THRESHOLD_DIRECTION	direction;	
	PICO_THRESHOLD_MODE				thresholdMode;
} PICO_DIRECTION;

typedef struct tPicoUserProbeInteractions
{
	uint16_t connected_;

	PICO_CHANNEL channel_;
	uint16_t enabled_;

	PicoConnectProbe probeName_;

	uint8_t requiresPower_;
	uint8_t isPowered_;

	PICO_STATUS status_;

	PICO_CONNECT_PROBE_RANGE probeOff_;

	PICO_CONNECT_PROBE_RANGE rangeFirst_;
	PICO_CONNECT_PROBE_RANGE rangeLast_;
	PICO_CONNECT_PROBE_RANGE rangeCurrent_;

	PICO_COUPLING couplingFirst_;
	PICO_COUPLING couplingLast_;
	PICO_COUPLING couplingCurrent_;

  PICO_BANDWIDTH_LIMITER_FLAGS filterFlags_;
  PICO_BANDWIDTH_LIMITER_FLAGS filterCurrent_;
	PICO_BANDWIDTH_LIMITER defaultFilter_;
} PICO_USER_PROBE_INTERACTIONS;

typedef struct tPicoDataBuffers
{
	PICO_CHANNEL							channel_;
	uint64_t									waveform_;
	PICO_RATIO_MODE						downSampleRatioMode_;
	PICO_READ_SELECTION				read_;

	PICO_POINTER							bufferMax_;
	PICO_POINTER							bufferMin_;
	PICO_DATA_TYPE						dataType_;

	uint32_t									nDistributionPoints_;
} PICO_DATA_BUFFERS;

typedef struct tPicoStreamingDataInfo
{
	// in
	PICO_CHANNEL channel_;
	PICO_RATIO_MODE mode_;
	PICO_DATA_TYPE type_;

	int32_t		noOfSamples_;
	uint64_t	bufferIndex_;
	int32_t	startIndex_;
	int16_t   overflow_;
} PICO_STREAMING_DATA_INFO;

typedef struct tPicoStreamingDataTriggerInfo
{
	uint64_t	triggerAt_;
	int16_t   triggered_;
	int16_t		autoStop_;
} PICO_STREAMING_DATA_TRIGGER_INFO;

typedef struct tPicoScalingFactors
{
	PICO_CHANNEL							channel;
	PICO_CONNECT_PROBE_RANGE  range;
	int16_t										offset;
	double							      scalingFactor;
} PICO_SCALING_FACTORS_VALUES;

typedef struct tProbeApp
{
	int32_t                   id_;
	int32_t                   appMajorVersion_;
	int32_t                   appMinorVersion_;
} PROBE_APP;

typedef struct tDigitalChannelDirections
{
  PICO_PORT_DIGITAL_CHANNEL channel;
	PICO_DIGITAL_DIRECTION direction;
} PICO_DIGITAL_CHANNEL_DIRECTIONS;

typedef struct tPicoDigitalPortInteractions
{
	uint16_t connected;
	PICO_CHANNEL channel;
	PICO_STATUS status_;

} PICO_DIGITAL_PORT_INTERACTIONS;

#pragma pack(pop)

#endif
