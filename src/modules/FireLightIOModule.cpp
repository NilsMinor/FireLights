////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// **
// ****************************************************************************/
////////////////////////////////////////////////////////////////////////////////

#include <FireLightIOModule.h>
#include <Logger.h>
#include <Utility.h>
#include <Node.h>
#include <GlobalState.h>
#include <FireLightTypes.h>
#include "app_util_platform.h"
#include <cmath>  



FireLightIOModule::FireLightIOModule()
	: Module(ModuleId::FireLightIO_MODULE, "FireLight-io")
{
	//Register callbacks n' stuff

	//Save configuration to base class variables
	//sizeof configuration must be a multiple of 4 bytes
	configurationPointer = &configuration;
	configurationLength = sizeof(FireLightIOModuleConfiguration);

	//Set defaults
	ResetToDefaultConfiguration();

    FireLightExtPins FireLightPinConfig;
	GS->boardconf.getCustomPinset(&FireLightPinConfig);

    this->InitADC( FireLightPinConfig.vbatPin, FireLightPinConfig.vsolarPin );
    this->initPIR( FireLightPinConfig.pirPin );
	this->initPWM (FireLightPinConfig.lio1, FireLightPinConfig.lio2, FireLightPinConfig.lio3,
				  FireLightPinConfig.lio4, FireLightPinConfig.lio5);

	animationIsRunning = false;
}

void FireLightIOModule::ResetToDefaultConfiguration()
{
	//Set default configuration values
	configuration.moduleId = moduleId;
	configuration.moduleActive = true;
	configuration.moduleVersion = FireLight_IO_MODULE_CONFIG_VERSION;

	//Set additional config values...
}

void FireLightIOModule::ConfigurationLoadedHandler(u8* migratableConfig, u16 migratableConfigLength)
{
	//Version migration can be added here, e.g. if module has version 2 and config is version 1
	//if(migratableConfig->moduleVersion == 1){/* ... */};

	//Do additional initialization upon loading the config

}

void FireLightIOModule::TimerEventHandler(u16 passedTimeDs)
{
	// Trigger adc measurement
	if (isADCInitialized && adcUpdateActive && SHOULD_IV_TRIGGER(GS->appTimerDs, passedTimeDs, 10)){
        ErrorType error = FruityHal::AdcSample(*m_buffer, ADC_SAMPLES);
	    FRUITYMESH_ERROR_CHECK((u32) error);
	}
	// Update ADC 
	if (isADCInitialized && SHOULD_IV_TRIGGER(GS->appTimerDs, passedTimeDs, SEC_TO_DS(ADC_UPDATE_INTERVALL_SEC))){
        adcUpdateActive = true;
	}
}

TerminalCommandHandlerReturnType FireLightIOModule::TerminalCommandHandler(const char* commandArgs[], u8 commandArgsSize)
{
	//React on commands, return true if handled, false otherwise
	if(commandArgsSize >= 3 && TERMARGS(2, moduleName))
	{
		if(TERMARGS(0, "action"))
		{
			if(!TERMARGS(2, moduleName)) return TerminalCommandHandlerReturnType::UNKNOWN;

			if(commandArgsSize >= 4 && TERMARGS(3, "argument_a"))
			{

			return TerminalCommandHandlerReturnType::SUCCESS;
			}
			else if(commandArgsSize >= 4 && TERMARGS(3, "argument_b"))
			{

			return TerminalCommandHandlerReturnType::SUCCESS;
			}

			return TerminalCommandHandlerReturnType::UNKNOWN;

		}
	}

	//Must be called to allow the module to get and set the config
	return Module::TerminalCommandHandler(commandArgs, commandArgsSize);
}

void FireLightIOModule::ButtonHandler(u8 buttonId, u32 holdTimeDs) {

}

void FireLightIOModule::MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader) 
{
	//Must call superclass for handling
	Module::MeshMessageReceivedHandler(connection, sendData, packetHeader);

	/* if(packetHeader->messageType == MessageType::MODULE_TRIGGER_ACTION){
		ConnPacketModule const * packet = (ConnPacketModule const *)packetHeader;
	}

	//Parse Module responses
	if(packetHeader->messageType == MessageType::MODULE_ACTION_RESPONSE){
		ConnPacketModule const * packet = (ConnPacketModule const *)packetHeader;

		//Check if our module is meant and we should trigger an action
		if(packet->moduleId == moduleId)
		{
			
		}
	} */
}


// ADC driver

void FireLightIOModule::InitADC ( i16 vbatPinInput, i16 vsolarPinInput ) {
    
    isADCInitialized = false;
	adcUpdateActive = false;
    vbatPin = vbatPinInput;
    vsolarPin = vsolarPinInput;

    if (vbatPin != -1 && vsolarPin != -1) {
        
        ErrorType error = FruityHal::AdcInit(AdcEventHandler);
	    FRUITYMESH_ERROR_CHECK((u32)error);
        selectADC(vbatPin);

        isADCInitialized = true;
        logs("InitADC Initialized");
    }
}

void FireLightIOModule::convertADCValues ( void ) {

    u32 adc_sum_value = 0;
	for (u16 i = 0; i < ADC_SAMPLES; i++) {
		adc_sum_value += m_buffer[i];                
	}

    if ( selectedADC == vbatPin ) {  
        vbat = toMilliVolts (adc_sum_value, R1_VBAT, R2_VBAT);  
        selectADC(vsolarPin);
    } else if ( selectedADC == vsolarPin ) {
        vsolar = toMilliVolts (adc_sum_value, R1_VSOLAR, R2_VSOLAR);  
        selectADC(vbatPin);
		adcUpdateActive = false;
    } else {
        logs("no adc channel selected");
     }
    
    //logs("vbat = %u mV, vsolar = %u mV updated pin %u", vbat , vsolar, selectedADC);
}

void FireLightIOModule::AdcEventHandler()
{
    FireLightIOModule * p_iioModule = (FireLightIOModule *)GS->node.GetModuleById(ModuleId::FireLightIO_MODULE);
	if (p_iioModule == nullptr) return;
	p_iioModule->convertADCValues();
}

void FireLightIOModule::selectADC (i16 adcChannel) {
    if (adcChannel != -1) {
        selectedADC = adcChannel;
        //logs("select channel %u", selectedADC);
        FruityHal::AdcConfigureChannel(adcChannel, 
                FruityHal::AdcReference::ADC_REFERENCE_0_6V, 
	            FruityHal::AdcResoultion::ADC_10_BIT, 
                FruityHal::AdcGain::ADC_GAIN_1_6);
    }
}

u16 FireLightIOModule::toMilliVolts(u16 rawValue, u32 Resistor1, u32 Resistor2) {
	double voltageDividerDv =  (double(Resistor1 + Resistor2)) / double(Resistor2);
	return  u16( rawValue * ADC_REF / (1023) * voltageDividerDv ) ;
}

u16 FireLightIOModule::getSolarVoltage ( void ) {
	return vsolar;
}

u16 FireLightIOModule::getBatteryVoltage ( void ) {
	return vbat;
}

u8 FireLightIOModule::getPirSensorState ( void ) {
	return (u8)pirState;
}

// PIR driver 

void FireLightIOModule::initPIR ( i16 pirInput ) {

    if ( pirInput != -1) {
        logs("init pir at pit %u", pirInput );
        u32 error = 0;        
        //Activate GPIOTE if not already active
        nrf_drv_gpiote_init();

        //Register for both HighLow and LowHigh events
        //IF this returns NO_MEM, increase GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS
        nrf_drv_gpiote_in_config_t pirConfig;
        pirConfig.sense = NRF_GPIOTE_POLARITY_TOGGLE;
        pirConfig.pull = NRF_GPIO_PIN_NOPULL;
        pirConfig.is_watcher = 0;
        pirConfig.hi_accuracy = 0;

        //This uses the SENSE low power feature, all pin events are reported
        //at the same GPIOTE channel
        error =  nrf_drv_gpiote_in_init(pirInput, &pirConfig, pirCallback);
        FRUITYMESH_ERROR_CHECK((u32)error);

        //Enable the events
        nrf_drv_gpiote_in_event_enable(pirInput, true);
		pirState = PIRState::PIR_OFF;
    }
}

void FireLightIOModule::pirCallback (nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action ) {
	
	// diasbled for testing only

	/*FireLightIOModule * p_iioModule = (FireLightIOModule *)GS->node.GetModuleById(ModuleId::FireLightIO_MODULE);
	if (p_iioModule == nullptr) return;

	if (nrf_gpio_pin_read(pin) == 0){
		   p_iioModule->pirState = PIRState::PIR_OFF;
    }
    else {
	   p_iioModule->pirState = PIRState::PIR_ON;
    }

	if (p_iioModule->pirCB != nullptr ) {
		p_iioModule->pirCB ( p_iioModule->pirState );
	} else {
		logs("no pir callback was set" );
	}*/
}

void FireLightIOModule::triggerPIRManual () {

	/*if (pirState == PIRState::PIR_OFF) {
		pirState = PIRState::PIR_ON;
	} else {
		pirState = PIRState::PIR_OFF;
	}*/
	 
	pirState = PIRState::PIR_ON;
	
	if (pirCB != nullptr ) {
		pirCB ( pirState );
	} else {
		logs("no pir callback was set" );
	}
}

/**
 * Note it is important that FireLightIOModule was created before the module that implements the
 * pirCallback method, otherwise it is not able to set a callback 
 */ 
void FireLightIOModule::setPIRCallback (pirCallbackType cb) {
	this->pirCB = cb;
}


// PWM driver
static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);

static uint16_t const              	pwm_top_value  = PWM_MAX_VALUE;
static nrf_pwm_values_individual_t 	pwm_seq_values;
static nrf_pwm_sequence_t      	   	pwm_seuqnce;
static uint16_t						phase_channel = 0;


bool FireLightIOModule::getAnimationIsRunning (void) {
	return animationIsRunning;
}

void FireLightIOModule::setLIOManual (u8 level) {
	animationIsRunning = true;
	this->lioMode = MANUAL_MODE;
	this->pwmLightState = MANUAL_START;
	nrf_drv_pwm_simple_playback(&m_pwm0, &pwm_seuqnce, 1,  NRF_DRV_PWM_FLAG_LOOP);
	dynamicLevel = level;
}

void FireLightIOModule::setLIOGlow(bool run) {
	animationIsRunning = run;
	if (run) {
		this->lioMode = GLOW_MODE;
		this->pwmLightState = IS_OFF;
		this->pwm_counter = 0;
		nrf_drv_pwm_simple_playback(&m_pwm0, &pwm_seuqnce, 1,  NRF_DRV_PWM_FLAG_LOOP);
	}
	else {
		nrf_drv_pwm_stop (&m_pwm0, true);
	}
}

void FireLightIOModule::setLIOLightMode (bool run) {
	animationIsRunning = run;
	if (run) {
		this->lioMode = LIGHT_MODE;
		this->pwmLightState = IS_OFF;
		this->pwm_counter = 0;
		nrf_drv_pwm_simple_playback(&m_pwm0, &pwm_seuqnce, 1,  NRF_DRV_PWM_FLAG_LOOP);
	}
	else {
		nrf_drv_pwm_stop (&m_pwm0, true);
	}
	
}

void FireLightIOModule::lioPing (uint16_t timeOut) {
	animationIsRunning = true;
	pingTimeOut = timeOut;
	this->lioMode = PING_MODE;
	this->pwmLightState = IS_OFF;
	this->pwm_counter = 0;
	nrf_drv_pwm_simple_playback(&m_pwm0, &pwm_seuqnce, 1,  NRF_DRV_PWM_FLAG_LOOP);
	
}

void FireLightIOModule::lioPingStateMachine ( void ) {
	this->pwm_counter++;

	if (this->pwm_counter % this->pwmLightTimeSettings.refreshInterval == 0) {

		uint16_t * p_channels = (uint16_t *)&pwm_seq_values;
		uint16_t value = p_channels[phase_channel];
		uint16_t maxValue = this->pwmLightTimeSettings.maxGlowValue;

		if (this->pwmLightState == IS_OFF) {
			this->pwm_counter = 0;
			this->pwmLightState = GLOW_STARTED;
			logs("change state to GLOW_STARTED");
			value = pwm_top_value;
		} else if (this->pwmLightState == GLOW_STARTED && this->pwm_counter % 20 == 0) {
 
 			uint16_t lastValue = p_channels[phase_channel];
			value = pwm_top_value - maxValue;

			//logs("pwm_counter [%u] rotation [%u] subValue [%u] value [%u]",
			//	pwm_counter, rotation, subValue, value);
			
			p_channels[phase_channel] = pwm_top_value;
			phase_channel = (phase_channel+1) % 4; // set next channel	
			p_channels[phase_channel] = value;

			if (this->pwm_counter * 10 >= pingTimeOut) {
				this->pwmLightState = SEQUENCE_FINISHED;
				logs("change state to SEQUENCE_FINISHED %u", this->pwm_counter * 10);
				this->pwm_counter = 0;
			}
		} else if (this->pwmLightState == SEQUENCE_FINISHED) {
			animationIsRunning = false;
			phase_channel = 0;
			value = pwm_top_value;	
			p_channels[0] = value;
			p_channels[1] = value;
			p_channels[2] = value;
			p_channels[3] = value;
		}
	}
}

void FireLightIOModule::lioGlowStateMachine ( void ) {
	this->pwm_counter++;

	if (this->pwm_counter % this->pwmLightTimeSettings.refreshInterval == 0) {

		uint16_t * p_channels = (uint16_t *)&pwm_seq_values;
		uint16_t value = p_channels[phase_channel];
		uint16_t maxValue = this->pwmLightTimeSettings.maxGlowValue;

		if (this->pwmLightState == IS_OFF) {
			this->pwm_counter = 0;
			this->pwmLightState = PING_STARTED;
			logs("change state to PING_STARTED");
			value = pwm_top_value;
		} else if (this->pwmLightState == GLOW_STARTED && this->pwm_counter % 4 == 0) {
			uint16_t rand =  Utility::GetRandomInteger();
			uint16_t lastValue = p_channels[phase_channel];
			uint16_t rotation = this->pwm_counter % 180;
			uint16_t subValue = 100 * sin(rotation * PI/180);
			uint16_t minBrightness = 50;
			value = pwm_top_value - subValue - minBrightness;

			//logs("pwm_counter [%u] rotation [%u] subValue [%u] value [%u]",
			//	pwm_counter, rotation, subValue, value);
			
			phase_channel = (phase_channel+1) % 4; // set next channel	
			p_channels[phase_channel] = value;

			if (this->pwm_counter * 10 >= this->pwmLightTimeSettings.glowTime) {
				this->pwmLightState = SEQUENCE_FINISHED;
				logs("change state to SEQUENCE_FINISHED %u", this->pwm_counter * 10);
				this->pwm_counter = 0;
			}
		} else if (this->pwmLightState == SEQUENCE_FINISHED) {
			animationIsRunning = false;
			value = pwm_top_value;	
			p_channels[0] = value;
			p_channels[1] = value;
			p_channels[2] = value;
			p_channels[3] = value;
		}
	}
}

void FireLightIOModule::lioManualStateMachine ( void ) {
	uint16_t * p_channels = (uint16_t *)&pwm_seq_values;
	uint16_t targetLevel = pwm_top_value - pwm_top_value / 100 * dynamicLevel;
	uint8_t channel = 0;
	uint16_t value = p_channels[channel];
	
	this->pwm_counter++;

	if (this->pwmLightState == MANUAL_START) {
		// save light values
		lightLevels[channel] = p_channels[channel];
		this->pwm_counter = 0;
		this->pwmLightState = MANUAL_FADING;
		logs("change state to MANUAL_FADING : %u", lightLevels[channel]);

	} else if (this->pwmLightState == MANUAL_FADING && this->pwm_counter % this->pwmLightTimeSettings.refreshInterval == 0) {
		// fade lights to target value
		bool fadeHigh = lightLevels[channel] > targetLevel;
		uint16_t delta = abs(lightLevels[channel] - targetLevel);

		logs("pwm delta %u, value %u, targetLevel %u", delta, p_channels[channel], targetLevel);
		
		if(fadeHigh) {
			value -= delta / 50;
		} else {
			value += delta / 50;
		}
		
		if (fadeHigh && value > targetLevel) {	
			p_channels[channel] = value;
		} 
		else if (!fadeHigh && value < targetLevel) {
			p_channels[channel] = value;
		}
		else {
			logs("stopped delta %u, value %u, targetLevel %u", delta, p_channels[channel], targetLevel);
			p_channels[channel] = targetLevel;
			this->pwmLightState = MANUAL_STOPPED;
		}
	}
	else if (this->pwmLightState == MANUAL_STOPPED) {
		// finished
	}
}

void FireLightIOModule::lioAutomaticStateMachine ( void ) {
	
	this->pwm_counter++;

	if (this->pwm_counter % this->pwmLightTimeSettings.refreshInterval == 0) {
		//logs("update %u", this->pwm_counter);
		uint8_t channel = 0;
		uint16_t * p_channels = (uint16_t *)&pwm_seq_values;
		uint16_t value = p_channels[channel];
		u32 step = this->pwmLightTimeSettings.maxValue * 10 / this->pwmLightTimeSettings.fadeOnTime;

		if (this->pwmLightState == IS_OFF) {
			this->pwm_counter = 0;
			this->pwmLightState = FADE_ON;
			logs("change state to FADE_ON");
			value = pwm_top_value;

		} else if (this->pwmLightState == FADE_ON) {
			value -= step; 
			if (this->pwm_counter * 10 >= this->pwmLightTimeSettings.fadeOnTime) {
				this->pwmLightState = STAY_ON;
				logs("change state to STAY_ON %u", this->pwm_counter * 10);
				this->pwm_counter = 0;
			}
		} else if (this->pwmLightState == STAY_ON) {
			value = pwm_top_value - this->pwmLightTimeSettings.maxValue;

			if (this->pwm_counter * 10 >= this->pwmLightTimeSettings.stayOnTime) {
				this->pwmLightState = FADE_OFF;
				logs("change state to FADE_OFF  %u", this->pwm_counter * 10);
				this->pwm_counter = 0;
			}

		} else if (this->pwmLightState == FADE_OFF) {
			value += step; 
			
			if (this->pwm_counter * 10 >= this->pwmLightTimeSettings.fadeOffTime) {
				this->pwmLightState = SEQUENCE_FINISHED;
				value = pwm_top_value;
				logs("change state to IS_OFF %u", this->pwm_counter * 10);
				this->pwm_counter = 0;
			}
		} 
		else if (this->pwmLightState == SEQUENCE_FINISHED) { 
			this->pwm_counter = 0;
			animationIsRunning = false;
		}
		else {
			this->pwmLightState = IS_OFF;
		}

		p_channels[channel] = value;
	}
}

void FireLightIOModule::pwm_handler(nrf_drv_pwm_evt_type_t event_type) {
	FireLightIOModule * p_iio = (FireLightIOModule *)GS->node.GetModuleById(ModuleId::FireLightIO_MODULE);
	if (p_iio == nullptr) return;

    if (event_type == NRF_DRV_PWM_EVT_FINISHED)
    {
		switch((i16) p_iio->lioMode) {
			case MANUAL_MODE:
				p_iio->lioManualStateMachine( );
			break;
			case LIGHT_MODE:
				p_iio->lioAutomaticStateMachine( );
			break;
			case GLOW_MODE:
				p_iio->lioGlowStateMachine( );
			break;
			case PING_MODE:
				p_iio->lioPingStateMachine( );
			break;
		}	 
    }
}

void FireLightIOModule::initPWM (i16 p1, i16 p2, i16 p3, i16 p4, i16 p5) {

    pwm_seuqnce.values.p_individual = &pwm_seq_values;
    pwm_seuqnce.length              = NRF_PWM_VALUES_LENGTH(pwm_seq_values);
    pwm_seuqnce.repeats             = 0;
    pwm_seuqnce.end_delay           = 0;
	pwm_counter = 0;

	lioPins[0] = {0};
	pwmInitialized = false;
	lioState = LIOState::LIO_OFF;
	pwmLightState = PWMLightStates::IS_OFF;
	pwmLightTimeSettings.refreshInterval = 1;
	pwmLightTimeSettings.fadeOnTime  	= 1000;
	pwmLightTimeSettings.stayOnTime	 	= 5000;
	pwmLightTimeSettings.fadeOffTime 	= 1000;
	pwmLightTimeSettings.glowTime 		= 10000;
	pwmLightTimeSettings.maxGlowValue	= pwm_top_value / 20;
	pwmLightTimeSettings.maxValue 		= pwm_top_value / 5;


 	if (p1!= -1) {
        lioPins[0] = (u8) p1 ;
        lioPins[1] = (u8) p2 ;
        lioPins[2] = (u8) p3 ;
        lioPins[3] = (u8) p4 ;
        lioPins[4] = (u8) p5 ;

		uint32_t err_code;
		nrf_drv_pwm_config_t const config0 =
		{
				.output_pins =
				{
						(u8) lioPins[0], // channel 0
						(u8) lioPins[1], // channel 1
						(u8) lioPins[2], // channel 2
						(u8) lioPins[3], // channel 3
				},
				.irq_priority = APP_IRQ_PRIORITY_LOWEST,
				.base_clock   = NRF_PWM_CLK_1MHz,
				.count_mode   = NRF_PWM_MODE_UP,
				.top_value    = pwm_top_value,
				.load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
				.step_mode    = NRF_PWM_STEP_AUTO
		};
		
		err_code = nrf_drv_pwm_init(&m_pwm0, &config0, pwm_handler);

		if (err_code != NRF_SUCCESS) {
			logs("init pwm failed");
		} else {
			pwmInitialized = true;
			logs("init pwm success");
		}

		pwm_seq_values.channel_0 = pwm_top_value;
		pwm_seq_values.channel_1 = pwm_top_value;
		pwm_seq_values.channel_2 = pwm_top_value;
		pwm_seq_values.channel_3 = pwm_top_value;
    }
    else {
        logs("No pwm pins found in GS");
    }
}

