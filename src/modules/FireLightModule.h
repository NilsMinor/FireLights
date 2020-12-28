////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// ** FireLight main module
// **
// ** How to flash Falsh device
// ** nrfjprog --family nrf52 --program "github_dev_nrf52_merged.hex" --verify --chiperase --reset
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Module.h>
#include <FireLightIOModule.h>

constexpr VendorModuleId FireLight_MODULE_ID = GET_VENDOR_MODULE_ID(0xAAAA, 2);
constexpr u8 FireLight_MODULE_CONFIG_VERSION = 1;

class FireLightModule: public Module
{
	private:

		//Module configuration that is saved persistently (size must be multiple of 4)
		struct FireLightModuleConfiguration : ModuleConfiguration{
			//Insert more persistent config values here
			u16 previousLightId;
			u16 followingLightId;
		};

		FireLightModuleConfiguration configuration;

		enum FireLightModuleTriggerActionMessages{
			MESSAGE_TEST = 0,
			MESSAGE_SET_LIGHT_LEVEL = 1,
			MESSAGE_SET_PARTNER = 2,
			MESSAGE_PARTNER_NOTIFICATION = 3,
			MESSAGE_PING_LIGHT = 4,
			MESSAGE_GET_DEVICE_INFO = 10,
		};

		enum FireLightModuleActionResponseMessages{
			MESSAGE_TEST_RESPONSE = 0,
			MESSAGE_DEVICE_INFO_RESPONSE = 20,
		};

		enum FireLightLightMessages {
			PIR_TRIGGER = 0
		};

		enum FireLightDeviceInfoTypes {
			GET_DEVICE_INFO = 0,
			GET_PARTNER_IDS = 1
		};

		enum PartnerNotificationEventType {
			INDIVIDUAL_RECOGNIZED = 1
		};
		

		
		//####### Module messages (these need to be packed)
		#pragma pack(push)
		#pragma pack(1)

			// SET LIGHT LEVEL MESSAGE
			#define SIZEOF_FireLight_SET_LIGHT_LEVEL_MESSAGE 2
			typedef struct
			{
				u16 level;
			}FireLightSetLightLevelMessage;
			// SET LIGHT LEVEL MESSAGE

			// SET PARTNER IDS MESSAGE
			#define SIZEOF_FireLight_SET_PARTNER_MESSAGE 6
			typedef struct
			{
				u16 nodeId;
				u16 previousLightId;
				u16 followingLightId;
			}FireLightSetPartnerIDsMessage;
			// SET PARTNER IDS MESSAGE

			// GET DEVICE INFO
			#define SIZEOF_FireLight_GET_DEVICE_INFO_MESSAGE 4
			typedef struct
			{
				u16 nodeId;
				u16 deviceInfoType;
			} FireLightGetDeviceInfoMessage;
			// GET DEVICE INFO

			// PARTNER NOTIFICATION MESSAGE
			#define SIZEOF_PARTNER_NOTIFICATION_MESSAGE 7
			typedef struct
			{
				u8 eventType;
				u32 timeStamp;
				u16 pace;
			}FireLightPartnerNotificationMessage;
			// PARTNER NOTIFICATION MESSAGE

			#define SIZEOF_FireLight_DEVICE_INFO_MESSAGE 7
			typedef struct
			{
				u16 nodeId;
				u16 vsolar;
				u16 vbat;
				u8 pirState;

			}FireLightDeviceInfoMessage;

			#define SIZEOF_PING_MESSAGE 2
			typedef struct
			{
				u16 timeoutInMs;

			}FireLightPingLightMessage;
			 

		#pragma pack(pop)
		//####### Module messages end

		FireLightLightModes mode;
		FireLightStates currentState; 
		FireLightIOModule * p_iioModule;
		NodeId notifiedPartnerId;
		
		void handleSM ( void );
		static void pirCallback (u16 state);

		void setLighLeveltManual (u8 level);
		void setPartnerLights (u16 previousLightId, u16 followingLightId);
		void saveModuleConfiguration( void );
		void sendDeviceInfoPacket ( NodeId senderNode, NodeId targetNode, u8 requestHandle );
		void sendPartnerIdsPacket ( NodeId senderNode, NodeId targetNode, u8 requestHandle ); 
		void sendGlowNotificationToPartner ( );
		void handlePartnerNotificationMessage (NodeId senderNode, const FireLightPartnerNotificationMessage *packetData);
		void handlePIRCallback(u16 pirState);

	public:
		FireLightModule();

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
