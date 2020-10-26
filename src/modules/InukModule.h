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

constexpr VendorModuleId INUK_MODULE_ID = GET_VENDOR_MODULE_ID(0xAAAA, 2);
constexpr u8 INUK_MODULE_CONFIG_VERSION = 1;

class InukModule: public Module
{
	private:

		//Module configuration that is saved persistently (size must be multiple of 4)
		struct InukModuleConfiguration : ModuleConfiguration{
			//Insert more persistent config values here
			u16 previousLightId;
			u16 followingLightId;
		};

		InukModuleConfiguration configuration;

		enum InukModuleTriggerActionMessages{
			MESSAGE_TEST = 0,
			MESSAGE_SET_LIGHT_LEVEL = 1,
			MESSAGE_SET_PARTNER = 2,

			MESSAGE_GET_DEVICE_INFO = 10,
		};

		enum InukModuleActionResponseMessages{
			MESSAGE_TEST_RESPONSE = 0,
			MESSAGE_DEVICE_INFO_RESPONSE = 20,
		};

		enum InukLightMessages {
			PIR_TRIGGER = 0
		};
		

		
		//####### Module messages (these need to be packed)
		#pragma pack(push)
		#pragma pack(1)

			// SET LIGHT LEVEL MESSAGE
			#define SIZEOF_INUK_SET_LIGHT_LEVEL_MESSAGE 2
			typedef struct
			{
				u16 level;

			}InukSetLightLevelMessage;
			// SET LIGHT LEVEL MESSAGE

			// SET PARTNER IDS MESSAGE
			#define SIZEOF_INUK_SET_PARTNER_MESSAGE 4
			typedef struct
			{
				u16 previousLightId;
				u16 followingLightId;

			}InukSetPartnerIDsMessage;
			// SET PARTNER IDS MESSAGE


			// GET DEVICE INFO
			#define SIZEOF_INUK_GET_DEVICE_INFO_MESSAGE 2
			typedef struct
			{
				u16 nodeId;
			}InukGetDeviceInfoMessage;

			#define SIZEOF_INUK_DEVICE_INFO_MESSAGE 7
			typedef struct
			{
				u16 nodeId;
				u16 vsolar;
				u16 vbat;
				u8 pirState;

			}InukDeviceInfoMessage;
			// GET DEVICE INFO

			// #define SIZEOF_INUK_MODULE_MESSAGE 10
			// typedef struct
			// {
			// 	//Insert values here
			// 	u16 vsolar;
			// 	u16 vbat;
			// 	u16 lightCommandMessage;

			// }InukModuleMessage;

		#pragma pack(pop)
		//####### Module messages end

		InukLightModes mode;
		InukStates currentState; 
		InukIOModule * p_iioModule;
		
		void handleSM ( void );
		static void pirCallback (u16 state);

		void setLighLeveltManual (u8 level);
		void setPartnerLights (u16 previousLightId, u16 followingLightId);
		void saveModuleConfiguration( void );
		void sendDeviceInfoPacket ( NodeId toNode, NodeId targetNode, u8 requestHandle );

	public:
		InukModule();

		void ConfigurationLoadedHandler(u8* migratableConfig, u16 migratableConfigLength) override final;

		void ResetToDefaultConfiguration() override;

		void TimerEventHandler(u16 passedTimeDs) override;

		void MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader) override;

		#ifdef TERMINAL_ENABLED
			TerminalCommandHandlerReturnType TerminalCommandHandler(const char* commandArgs[], u8 commandArgsSize) override;
		#endif

		#if IS_ACTIVE(BUTTONS)
			void ButtonHandler(u8 buttonId, u32 holdTimeDs) override;
		#endif
};
