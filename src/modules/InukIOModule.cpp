////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// **
// ****************************************************************************/
////////////////////////////////////////////////////////////////////////////////

#include <InukIOModule.h>
#include <Logger.h>
#include <Utility.h>
#include <Node.h>
#include <GlobalState.h>
#include <InukTypes.h>
#include "app_util_platform.h"
#include <cmath>  



InukIOModule::InukIOModule()
	: Module(ModuleId::INUKIO_MODULE, "inuk-io")
{
	//Register callbacks n' stuff

	//Save configuration to base class variables
	//sizeof configuration must be a multiple of 4 bytes
	configurationPointer = &configuration;
	configurationLength = sizeof(InukIOModuleConfiguration);

	//Set defaults
	ResetToDefaultConfiguration();

    InukExtPins inukPinConfig;
	GS->boardconf.getCustomPinset(&inukPinConfig);

    this->InitADC( inukPinConfig.vbatPin, inukPinConfig.vsolarPin );
    this->initPIR( inukPinConfig.pirPin );
	this->initPWM (inukPinConfig.lio1, inukPinConfig.lio2, inukPinConfig.lio3,
				   inukPinConfig.lio4, inukPinConfig.lio5);


}

void InukIOModule::ResetToDefaultConfiguration()
{
	//Set default configuration values
	configuration.moduleId = moduleId;
	configuration.moduleActive = true;
	configuration.moduleVersion = INUK_IO_MODULE_CONFIG_VERSION;

	//Set additional config values...
}

void InukIOModule::ConfigurationLoadedHandler(u8* migratableConfig, u16 migratableConfigLength)
{
	//Version migration can be added here, e.g. if module has version 2 and config is version 1
	//if(migratableConfig->moduleVersion == 1){/* ... */};

	//Do additional initialization upon loading the config

}

void InukIOModule::TimerEventHandler(u16 passedTimeDs)
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

TerminalCommandHandlerReturnType InukIOModule::TerminalCommandHandler(const char* commandArgs[], u8 commandArgsSize)
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

void InukIOModule::ButtonHandler(u8 buttonId, u32 holdTimeDs) {

}

void InukIOModule::MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader) 
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

void InukIOModule::InitADC ( i16 vbatPinInput, i16 vsolarPinInput ) {
    
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

void InukIOModule::convertADCValues ( void ) {

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

void InukIOModule::AdcEventHandler()
{
    InukIOModule * p_iioModule = (InukIOModule *)GS->node.GetModuleById(ModuleId::INUKIO_MODULE);
	if (p_iioModule == nullptr) return;
	p_iioModule->convertADCValues();
}

void InukIOModule::selectADC (i16 adcChannel) {
    if (adcChannel != -1) {
        selectedADC = adcChannel;
        //logs("select channel %u", selectedADC);
        FruityHal::AdcConfigureChannel(adcChannel, 
                FruityHal::AdcReference::ADC_REFERENCE_0_6V, 
	            FruityHal::AdcResoultion::ADC_10_BIT, 
                FruityHal::AdcGain::ADC_GAIN_1_6);
    }
}

u16 InukIOModule::toMilliVolts(u16 rawValue, u32 Resistor1, u32 Resistor2) {
	double voltageDividerDv =  (double(Resistor1 + Resistor2)) / double(Resistor2);
	return  u16( rawValue * ADC_REF / (1023) * voltageDividerDv ) ;
}

u16 InukIOModule::getSolarVoltage ( void ) {
	return vsolar;
}

u16 InukIOModule::getBatteryVoltage ( void ) {
	return vbat;
}

u8 InukIOModule::getPirSensorState ( void ) {
	return (u8)pir;
}

// PIR driver 

void InukIOModule::initPIR ( i16 pirInput ) {

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
    }
}

void InukIOModule::pirCallback (nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action ) {
	
	InukIOModule * p_iioModule = (InukIOModule *)GS->node.GetModuleById(ModuleId::INUKIO_MODULE);
	if (p_iioModule == nullptr) return;

	if (nrf_gpio_pin_read(pin) == 0){
		   p_iioModule->pir = PIRState::PIR_OFF;
    }
    else {
	   p_iioModule->pir = PIRState::PIR_ON;
    }

	if (p_iioModule->pirCB != nullptr ) {
		p_iioModule->pirCB ( p_iioModule->pir );
	} else {
		logs("no pir callback was set" );
	}
}

/**
 * Note it is important that InukIOModule was created before the module that implements the
 * pirCallback method, otherwise it is not able to set a callback 
 */ 
void InukIOModule::setPIRCallback (pirCallbackType cb) {
	this->pirCB = cb;
}


// PWM driver
static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);

static uint16_t const              	pwm_top_value  = PWM_MAX_VALUE;
static nrf_pwm_values_individual_t 	pwm_seq_values;
static nrf_pwm_sequence_t      	   	pwm_seuqnce;
static uint16_t						phase_channel = 0;


void InukIOModule::setLIOManual (u8 level) {
	this->lioState = LIOState::LIO_ON_MANUAL;
	this->pwmLightState = MANUAL_START;
	nrf_drv_pwm_simple_playback(&m_pwm0, &pwm_seuqnce, 1,  NRF_DRV_PWM_FLAG_LOOP);
	dynamicLevel = level;
}

void InukIOModule::setLIO (LIOState state) {

	this->lioState = state;

	if (state == LIOState::LIO_ON || state == LIOState::LIO_GLOW) {
		this->pwmLightState = IS_OFF;
		nrf_drv_pwm_simple_playback(&m_pwm0, &pwm_seuqnce, 1,  NRF_DRV_PWM_FLAG_LOOP);
	} 
	else {
		nrf_drv_pwm_stop (&m_pwm0, true);
	}
 }

LIOState InukIOModule::getLIO ( void ) {
	return lioState;
}

void InukIOModule::lioGlowStateMachine ( void ) {
	this->pwm_counter++;

	if (this->pwm_counter % this->pwmLightTimeSettings.refreshInterval == 0) {

		uint16_t * p_channels = (uint16_t *)&pwm_seq_values;
		uint16_t value = p_channels[phase_channel];
		uint16_t maxValue = this->pwmLightTimeSettings.maxGlowValue;
		u32 step = maxValue * 10 / this->pwmLightTimeSettings.fadeOnTime;

		if (this->pwmLightState == IS_OFF) {
			this->pwm_counter = 0;
			this->pwmLightState = GLOW_STARTED;
			logs("change state to GLOW_STARTED");
			value = pwm_top_value;
		} else if (this->pwmLightState == GLOW_STARTED && this->pwm_counter % 6 == 0) {
			uint16_t rand =  Utility::GetRandomInteger();

			value = pwm_top_value - (rand % maxValue + maxValue / 2) ;
			
			phase_channel = (phase_channel+1) % 4;
			p_channels[phase_channel] = value;
			//logs("glow - value %u for chanel %u",value, phase_channel);

			if (this->pwm_counter * 10 >= this->pwmLightTimeSettings.glowTime) {
				this->pwmLightState = SEQUENCE_FINISHED;
				logs("change state to SEQUENCE_FINISHED %u", this->pwm_counter * 10);
				this->pwm_counter = 0;
			}
		} else if (this->pwmLightState == SEQUENCE_FINISHED) {
			value = pwm_top_value;	
			p_channels[0] = value;
			p_channels[1] = value;
			p_channels[2] = value;
			p_channels[3] = value;
		}
	}
}

void InukIOModule::lioManualStateMachine ( void ) {
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

void InukIOModule::lioStateMachine ( void ) {
	
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
		}
		else {
			this->pwmLightState = IS_OFF;
		}

		p_channels[channel] = value;
	}
}

void InukIOModule::pwm_handler(nrf_drv_pwm_evt_type_t event_type) {
	InukIOModule * p_iio = (InukIOModule *)GS->node.GetModuleById(ModuleId::INUKIO_MODULE);
	if (p_iio == nullptr) return;

    if (event_type == NRF_DRV_PWM_EVT_FINISHED)
    {
		if (p_iio->lioState == LIO_ON) {
			p_iio->lioStateMachine( );
		} else if (p_iio->lioState == LIO_ON_MANUAL) {
			p_iio->lioManualStateMachine();
		} 
		else if (p_iio->lioState == LIO_GLOW) {
			p_iio->lioGlowStateMachine( );
		}		 
    }
}

void InukIOModule::initPWM (i16 p1, i16 p2, i16 p3, i16 p4, i16 p5) {

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
	pwmLightTimeSettings.stayOnTime	 	= 2000;
	pwmLightTimeSettings.fadeOffTime 	= 1000;
	pwmLightTimeSettings.glowTime 		= 3000;
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

