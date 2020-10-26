////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// ** Inuk IO  module
// **
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Module.h>
#include <GlobalState.h>
#include <BoardConfig.h>
#include <InukTypes.h>
#include <stdint.h>
#include <nrf_drv_gpiote.h>
#include "../sdk/sdk14/components/drivers_nrf/pwm/nrf_drv_pwm.h"

constexpr u8 INUK_IO_MODULE_CONFIG_VERSION = 1;

// ADC 

#define ADC_SAMPLES                 1
#define ADC_REF                     3600

#define R1_VBAT                     806000
#define R2_VBAT                     2000000

#define R1_VSOLAR                   2000000
#define R2_VSOLAR                   806000

#define ADC_REFRESH_DS              100
#define ADC_UPDATE_INTERVALL_SEC    10

// PWM

#define APP_TIMER_PRESCALER         0
#define APP_TIMER_OP_QUEUE_SIZE     2
#define PWM_MAX_VALUE               10000

class InukIOModule: public Module
{
	private:

		//Module configuration that is saved persistently (size must be multiple of 4)
		struct InukIOModuleConfiguration : ModuleConfiguration{
			//Insert more persistent config values here
		};

		InukIOModuleConfiguration configuration;

        u16 vsolar;
        u16 vbat;
        PIRState pir;
        pirCallbackType pirCB = nullptr;
        u8 dynamicLevel;

        u8 lioPins [5];
        u16 lightLevels [5];
        i16 vsolarPin;
        i16 vbatPin;
        i16 pirPin;
        i16 selectedADC;
        bool adcUpdateActive;

        i16 m_buffer[ADC_SAMPLES];

        bool isADCInitialized;

        void    InitADC (i16 vbatPinInput, i16 vsolarPinInput );
        void    convertADCValues ( void );
        u16     toMilliVolts(u16 rawValue, u32 Resistor1, u32 Resistor2);
        void    selectADC (i16 adcChannel);
        static void AdcEventHandler ( void );

        void initPIR ( i16 pirInput);
        static void pirCallback (nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action );

        bool pwmInitialized;
        nrf_pwm_sequence_t pwmSequence;
        LIOState lioState;
        void initPWM (i16 p1, i16 p2, i16 p3, i16 p4, i16 p5);
        static void pwm_handler(nrf_drv_pwm_evt_type_t event_type);
        void lioStateMachine ( void );
        void lioManualStateMachine ( void );
        void lioGlowStateMachine ( void );
        u32 pwm_counter;

        PWMLightStates pwmLightState;
        PWMLightTimeSettings pwmLightTimeSettings;

	public:
		InukIOModule();


        u16 getSolarVoltage ( void );
        u16 getBatteryVoltage ( void );
        u8 getPirSensorState ( void );

        void setLIO (LIOState state);
        LIOState getLIO ( void );

        void setLIOManual (u8 level);

        void ConfigurationLoadedHandler(u8* migratableConfig, u16 migratableConfigLength) override final;

		void ResetToDefaultConfiguration() override;

		void TimerEventHandler(u16 passedTimeDs) override;

        void MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader) override;

        TerminalCommandHandlerReturnType TerminalCommandHandler(const char* commandArgs[], u8 commandArgsSize) override;

        void ButtonHandler(u8 buttonId, u32 holdTimeDs) override;

        void setPIRCallback (pirCallbackType cb);
};
