////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// **
// ****************************************************************************/
////////////////////////////////////////////////////////////////////////////////

#include <InukModule.h>
#include <Logger.h>
#include <Utility.h>
#include <Node.h>
#include <GlobalState.h>
#include <InukTypes.h>
#include <stdlib.h>

constexpr u8 INUK_MODULE_CONFIG_VERSION = 1;

InukModule::InukModule()
	: Module(ModuleId::INUK_MODULE, "inuk")
{
	//Register callbacks n' stuff

	//Save configuration to base class variables
	//sizeof configuration must be a multiple of 4 bytes
	configurationPointer = &configuration;
	configurationLength = sizeof(InukModuleConfiguration);

	logs("Partner IDs are previousLightId :[ %u ] followingLightId : [ %u ]" ,
		(u16) configuration.previousLightId, 
		(u16) configuration.followingLightId);

	//Set defaults
	ResetToDefaultConfiguration();

}

void InukModule::ResetToDefaultConfiguration()
{
	//Set default configuration values
	configuration.moduleId = moduleId;
	configuration.moduleActive = true;
	configuration.moduleVersion = INUK_MODULE_CONFIG_VERSION;

	mode = InukLightModes::AUTOMATIC;

	//Set additional config values...
	this->p_iioModule = (InukIOModule *)GS->node.GetModuleById(ModuleId::INUKIO_MODULE);
	if (this->p_iioModule != nullptr) {
		this->p_iioModule->setPIRCallback (this->pirCallback); 
	}
}

void InukModule::ConfigurationLoadedHandler(ModuleConfiguration* migratableConfig, u16 migratableConfigLength)
{
	//Version migration can be added here, e.g. if module has version 2 and config is version 1
	if(migratableConfig->moduleVersion == 1){/* ... */};

	//Do additional initialization upon loading the config
}

void InukModule::TimerEventHandler(u16 passedTimeDs)
{
	//Do stuff on timer...
	// logs("Inuk timer %u", passedTimeDs);
	
	//this->handleSM();
}

#ifdef TERMINAL_ENABLED
	TerminalCommandHandlerReturnType InukModule::TerminalCommandHandler(const char* commandArgs[], u8 commandArgsSize)
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
#endif

void InukModule::pirCallback (u16 state) {
	logs("pirCallback %u", (u16 )state);
	// InukIOModule * p_iio = (InukIOModule *)GS->node.GetModuleById(ModuleId::INUKIO_MODULE);
	// if (p_iio != nullptr) {
	// 	//this->p_iioModule->setLIO(LIOState::LIO_ON);
	// 	p_iio->setLIO(LIOState::LIO_ON);
	// }
}

void InukModule::ButtonHandler(u8 buttonId, u32 holdTimeDs) {
	logs("ButtonHandler %d", holdTimeDs);

	// create data
	InukModuleMessage data;
	data.vsolar = 1111;
	data.vbat   = 1234;
	data.lightCommandMessage   = InukLightMessages::PIR_TRIGGER;

	u8 targetNodeId = NODE_ID_BROADCAST;

	//Send ping packet to that node
	SendModuleActionMessage(
			MessageType::MODULE_TRIGGER_ACTION,
			targetNodeId,
			InukModuleTriggerActionMessages::MESSAGE_TEST,
			0,
			(u8*)&data,
			SIZEOF_INUK_MODULE_MESSAGE,
			false
	);
}

void InukModule::handleSM ( void ) {

	if (mode == InukLightModes::AUTOMATIC) {
		switch (currentState)
		{
			case InukStates::ERROR :
				logs("Inuk state ERROR");
				break;
			case InukStates::STARTED :
				logs("Inuk state STARTED");
			
				break;
			case InukStates::WAITING_FOR_TRIGGER :
				logs("Inuk state WAITING_FOR_TRIGGER");
			
				break;
			case InukStates::TRIGGER_OCCURED :
				logs("Inuk state TRIGGER_OCCURED");
			
				break;
		
			default:
				break;
		}
	} else if (mode == InukLightModes::MANUAL) {

	}
	
}

void InukModule::setLighLeveltManual (u8 level) {
	// change mode to manual
	mode = InukLightModes::MANUAL;

	logs("setLightManual :  %u", level);
	this->p_iioModule->setLIOManual(level);
}

void InukModule::setPartnerLights (u16 previousLightId, u16 followingLightId) {
	if (previousLightId) {
		configuration.previousLightId = previousLightId;
	}
	if (followingLightId) {
		configuration.followingLightId = followingLightId;
	}

	GS->config.SaveConfigToFlash(nullptr, 0, nullptr, 0);

	logs("Partner IDs are previousLightId : [ %u ] followingLightId : [ %u ]" ,
		(u16) configuration.previousLightId, 
		(u16) configuration.followingLightId);
}

void InukModule::MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, connPacketHeader const * packetHeader)
{
	//Must call superclass for handling
	Module::MeshMessageReceivedHandler(connection, sendData, packetHeader);

	if(packetHeader->messageType == MessageType::MODULE_TRIGGER_ACTION){
		connPacketModule const * packet = (connPacketModule const *)packetHeader;

		//Check if our module is meant and we should trigger an action
		if(packet->moduleId == moduleId ){

			if(packet->actionType == InukModuleTriggerActionMessages::MESSAGE_SET_PARTNER && sendData->dataLength >= SIZEOF_INUK_SET_PARTNER_MESSAGE){
		 		InukSetPartnerIDsMessage const * packetData = (InukSetPartnerIDsMessage const *) (packet->data);
				 this->setPartnerLights ((u16) packetData->previousLightId, (u16) packetData->followingLightId);
			}
			else if(packet->actionType == InukModuleTriggerActionMessages::MESSAGE_SET_LIGHT_LEVEL && sendData->dataLength >= SIZEOF_INUK_SET_LIGHT_LEVEL_MESSAGE){
				InukSetLightLevelMessage const * packetData = (InukSetLightLevelMessage const *) (packet->data);
				this->setLighLeveltManual ((u8) packetData->level);
			}

			else if(packet->actionType == InukModuleTriggerActionMessages::MESSAGE_TEST){
				logs("Recived test message frome node %u",(u8) packet->moduleId);

				InukModuleTriggerActionMessages actionType = (InukModuleTriggerActionMessages)packet->actionType;
				u16 dataFieldLength = sendData->dataLength - SIZEOF_CONN_PACKET_MODULE;

				if (actionType == InukModuleTriggerActionMessages::MESSAGE_TEST) {

					//Parse the data and set the gpio ports to the requested
					InukModuleMessage const * packetData = (InukModuleMessage const *) (packet->data);
				
					logs("got test message with packetData | lightCommandMessage %u, vsolar %u, vbat %u",
						(u16) packetData->lightCommandMessage,
						(u16) packetData->vsolar,
						(u16) packetData->vbat
					);

					if (this->p_iioModule){
						LIOState s = this->p_iioModule->getLIO();
						if (s == LIOState::LIO_OFF) {
							this->p_iioModule->setLIO(LIOState::LIO_ON);
							//this->p_iioModule->setLIO(LIOState::LIO_GLOW);
						} else {
							this->p_iioModule->setLIO(LIOState::LIO_OFF);
						}
					}		
				}					
			}
		}
	}

	//Parse Module responses
	if(packetHeader->messageType == MessageType::MODULE_ACTION_RESPONSE){
		connPacketModule const * packet = (connPacketModule const *)packetHeader;

		//Check if our module is meant and we should trigger an action
		if(packet->moduleId == moduleId)
		{
			if(packet->actionType == InukModuleActionResponseMessages::MESSAGE_TEST_RESPONSE)
			{

			}
		}
	}
}
