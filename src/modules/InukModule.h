////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// ** Inuk main module
// **
// ** nrfjprog --family nrf52 --program "github_nrf52_merged.hex" --verify --chiperase --reset
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Module.h>
#include <InukIOModule.h>

class InukModule: public Module
{
	private:

		//Module configuration that is saved persistently (size must be multiple of 4)
		struct InukModuleConfiguration : ModuleConfiguration{
			//Insert more persistent config values here
		};

		InukModuleConfiguration configuration;

		enum InukModuleTriggerActionMessages{
			MESSAGE_TEST = 0
		};

		enum InukModuleActionResponseMessages{
			MESSAGE_TEST_RESPONSE = 0
		};

		enum InukLightMessages {
			PIR_TRIGGER = 0
		};
		

		
		//####### Module messages (these need to be packed)
		#pragma pack(push)
		#pragma pack(1)

			#define SIZEOF_INUK_MODULE_MESSAGE 10
			typedef struct
			{
				//Insert values here
				u16 vsolar;
				u16 vbat;
				u16 lightCommandMessage;

			}InukModuleMessage;

		#pragma pack(pop)
		//####### Module messages end

		InukStates currentState; 
		InukIOModule * p_iioModule;

		void handleSM ( void );

		static void pirCallback (u16 state);

	public:
		InukModule();

		void ConfigurationLoadedHandler(ModuleConfiguration* migratableConfig, u16 migratableConfigLength) override;

		void ResetToDefaultConfiguration() override;

		void TimerEventHandler(u16 passedTimeDs) override;

		void MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, connPacketHeader const * packetHeader) override;

		#ifdef TERMINAL_ENABLED
			TerminalCommandHandlerReturnType TerminalCommandHandler(const char* commandArgs[], u8 commandArgsSize) override;
		#endif

		#if IS_ACTIVE(BUTTONS)
			void ButtonHandler(u8 buttonId, u32 holdTimeDs) override;
		#endif
};
