

#pragma once
#include <stdint.h>
#include <PrimitiveTypes.h>

enum InukStates
{
	ERROR        			= -1,
	STARTED 				= 0,
	WAITING_FOR_TRIGGER    	= 1,
	TRIGGER_OCCURED      	= 2,
} ; 

struct InukExtPins : CustomPins {
    i16 lio1 = -1;
	i16 lio2 = -1;
	i16 lio3 = -1;
	i16 lio4 = -1;
	i16 lio5 = -1;

	i16 vbatPin = -1;
	i16 vsolarPin = -1;
	i16 pirPin = -1;
};

enum PIRState {
	PIR_ON = 0,
	PIR_OFF = 1
};

enum LIOState {
	LIO_ON = 0,
	LIO_OFF = 1,
	LIO_GLOW = 2
};
enum PWMLightStates {
	IS_OFF = 0,
	FADE_ON = 1,
	STAY_ON = 2,
	FADE_OFF = 3,
	SEQUENCE_FINISHED = 4,

	GLOW_STARTED = 10
};

struct PWMLightTimeSettings {
	u32 fadeOnTime;
	u32 stayOnTime;
	u32 fadeOffTime;
	u32 maxValue;
	u32 glowTime;
	u32 maxGlowValue;
	u32 refreshInterval;
};
typedef void (*pirCallbackType)(u16);  
