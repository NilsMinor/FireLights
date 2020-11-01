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
#include <Config.h>


InukModule::InukModule()
	: Module(ModuleId::INUK_MODULE, "inuk")
{
	//Register callbacks n' stuff

	//Save configuration to base class variables
	//sizeof configuration must be a multiple of 4 bytes
	configurationPointer = &configuration;
	configurationLength = sizeof(InukModuleConfiguration);
	notifiedPartnerId = 0;

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

void InukModule::ConfigurationLoadedHandler(u8* migratableConfig, u16 migratableConfigLength) 
{
	//Version migration can be added here, e.g. if module has version 2 and config is version 1
	//if(migratableConfig->moduleVersion == 1){/* ... */};

	//Do additional initialization upon loading the config

	logs("Partner IDs are previousLightId :[ %u ] followingLightId : [ %u ]" ,
		(u16) configuration.previousLightId, 
		(u16) configuration.followingLightId);

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
	

	InukModule * p_mod = (InukModule *)GS->node.GetModuleById(ModuleId::INUK_MODULE);

	if (p_mod) {
		p_mod->handlePIRCallback((u16 )state);
	}

	 /*InukIOModule * p_iio = (InukIOModule *)GS->node.GetModuleById(ModuleId::INUKIO_MODULE);
	 if (p_iio != nullptr) {
		 if (p_iio->getPirSensorState() == PIR_ON) {
			 p_iio->setLIOLightMode(true);
			
		 }  
	}*/
}

void InukModule::handlePIRCallback(u16 pirState) {
	logs("handlePIRCallback %u", (u16 )pirState);
	if (pirState == PIR_ON) {
		bool animationIsRunning = p_iioModule->getAnimationIsRunning();
		if (animationIsRunning) {
			p_iioModule->setLIOGlow(false);// stop animation
		}

		p_iioModule->setLIOLightMode(true);
		this->sendGlowNotificationToPartner();
	}
	 
}

void InukModule::ButtonHandler(u8 buttonId, u32 holdTimeDs) {
	logs("ButtonHandler %d", holdTimeDs);
	this->p_iioModule->triggerPIRManual();
	//this->p_iioModule->lioPing(1000);

	
	// create data
	// InukModuleMessage data;
	// data.vsolar = 1111;
	// data.vbat   = 1234;
	// data.lightCommandMessage   = InukLightMessages::PIR_TRIGGER;

	// u8 targetNodeId = NODE_ID_BROADCAST;

	// //Send ping packet to that node
	// SendModuleActionMessage(
	// 		MessageType::MODULE_TRIGGER_ACTION,
	// 		targetNodeId,
	// 		InukModuleTriggerActionMessages::MESSAGE_TEST,
	// 		0,
	// 		(u8*)&data,
	// 		SIZEOF_INUK_MODULE_MESSAGE,
	// 		false
	// );
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

void InukModule::saveModuleConfiguration (void) {
	 //Save the module config to flash

	const RecordStorageResultCode err = Utility::SaveModuleSettingsToFlash(
                        this,
                        this->configurationPointer,
                        this->configurationLength,
                        nullptr,
                        0,
                        nullptr,
                        0);

	logs("saveModuleConfiguration : %u" , (u8) err);
}

void InukModule::sendDeviceInfoPacket (  NodeId senderNode, NodeId targetNode, u8 requestHandle ) {

	InukDeviceInfoMessage data;
	data.nodeId = targetNode;
	data.vsolar = p_iioModule->getSolarVoltage();
	data.vbat   = p_iioModule->getBatteryVoltage();
	data.pirState = p_iioModule->getPirSensorState();

	logs("sendDeviceInfoPacket [ vsolar : %u ] [ vbat : %u ] [ pirState : %u ]", data.vsolar, data.vbat, data.pirState);

	 SendModuleActionMessage(
        MessageType::MODULE_ACTION_RESPONSE,
        senderNode,
        (u8)InukModuleActionResponseMessages::MESSAGE_DEVICE_INFO_RESPONSE,
        requestHandle,
        (u8*)&data,
        SIZEOF_INUK_DEVICE_INFO_MESSAGE,
        false
    );
}

void InukModule::sendPartnerIdsPacket (  NodeId senderNode, NodeId targetNode, u8 requestHandle ) {

	InukSetPartnerIDsMessage data;
	data.nodeId = targetNode;
	data.previousLightId = (u16) configuration.previousLightId;
	data.followingLightId = (u16) configuration.followingLightId;

	logs("sendPartnerIdsPacket [ nodeId : %u ] [ previousLightId : %u ] [ followingLightId : %u ]", 
			data.nodeId, data.previousLightId, data.followingLightId);

	 SendModuleActionMessage(
        MessageType::MODULE_ACTION_RESPONSE,
        senderNode,
        (u8)InukModuleActionResponseMessages::MESSAGE_DEVICE_INFO_RESPONSE,
        requestHandle,
        (u8*)&data,
        SIZEOF_INUK_DEVICE_INFO_MESSAGE,
        false
    );
}

void InukModule::sendGlowNotificationToPartner ( ) {

	// get partner IDs
	uint16_t previousLightId = configuration.previousLightId;
	uint16_t followingLightId = configuration.followingLightId;
	uint16_t targetNodeId = 0;

	logs("Partner IDs are previousLightId : [ %u ] followingLightId : [ %u ] notifiedPartnerId : [ %u ]" ,
		(u16) configuration.previousLightId, (u16) configuration.followingLightId, (u16) notifiedPartnerId);

	// direction --> [previous light] - [this light] - [following light]

	InukPartnerNotificationMessage data;
	data.eventType = (u8) PartnerNotificationEventType::INDIVIDUAL_RECOGNIZED;
	data.timeStamp = (u32) 123456;
	data.pace = (u16) 5;

	if ((followingLightId != 0 && notifiedPartnerId == followingLightId) || (notifiedPartnerId == 0 && previousLightId != 0)) {	
		// notifier was the previous light
		logs("send notification packet to previousLightId : [ %u ] eventType : [ %u ] timeStamp : [ %u ] pace : [ %u ]" ,
			(u16) targetNodeId, (u8) data.eventType, (u32) data.timeStamp, (u16) data.pace);
		// send message
		SendModuleActionMessage(
			MessageType::MODULE_TRIGGER_ACTION,
			previousLightId,
			(u8)InukModuleTriggerActionMessages::MESSAGE_PARTNER_NOTIFICATION,
			0,
			(u8*)&data,
			SIZEOF_PARTNER_NOTIFICATION_MESSAGE,
			false);
	}
	if ((previousLightId != 0 && notifiedPartnerId == previousLightId) || (notifiedPartnerId == 0 && followingLightId != 0 )) {
		// notifier was the following light

		logs("send notification packet to followingLightId : [ %u ] eventType : [ %u ] timeStamp : [ %u ] pace : [ %u ]" ,
			(u16) targetNodeId, (u8) data.eventType, (u32) data.timeStamp, (u16) data.pace);
		// send message
		SendModuleActionMessage(
			MessageType::MODULE_TRIGGER_ACTION,
			followingLightId,
			(u8)InukModuleTriggerActionMessages::MESSAGE_PARTNER_NOTIFICATION,
			0,
			(u8*)&data,
			SIZEOF_PARTNER_NOTIFICATION_MESSAGE,
			false);
		
	} else {
		logs("no partner ids were set");
	}		

	notifiedPartnerId = 0;
	
}

void InukModule::setPartnerLights (u16 previousLightId, u16 followingLightId) {
	if (previousLightId) {
		configuration.previousLightId = previousLightId;
	}
	if (followingLightId) {
		configuration.followingLightId = followingLightId;
	}

 	saveModuleConfiguration ();

	logs("Partner IDs are previousLightId : [ %u ] followingLightId : [ %u ]" ,
		(u16) configuration.previousLightId, 
		(u16) configuration.followingLightId);
}

void InukModule::handlePartnerNotificationMessage (NodeId senderNode, const InukPartnerNotificationMessage *packetData) {
	
	// remember which partner send the notification 
	notifiedPartnerId = senderNode;

	uint16_t pace = packetData->pace;
	uint8_t eventType = packetData->eventType;
	uint32_t timeStamp = packetData->timeStamp;

	logs("Received partner notification senderNode: [%u] pace : [ %u ] eventType : [ %u ] timeStamp : [ %u ]" ,
		(u16) senderNode, (u16) pace, (u8) eventType, (u32) timeStamp);

	if (eventType == PartnerNotificationEventType::INDIVIDUAL_RECOGNIZED) {
		p_iioModule->setLIOGlow(true);
	}

}

void InukModule::MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader)
{
	//Must call superclass for handling
	Module::MeshMessageReceivedHandler(connection, sendData, packetHeader);

	if(packetHeader->messageType == MessageType::MODULE_TRIGGER_ACTION){
		ConnPacketModule const * packet = (ConnPacketModule const *)packetHeader;

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
			else if(packet->actionType == InukModuleTriggerActionMessages::MESSAGE_PARTNER_NOTIFICATION && sendData->dataLength >= SIZEOF_PARTNER_NOTIFICATION_MESSAGE){
				InukPartnerNotificationMessage const * packetData = (InukPartnerNotificationMessage const *) (packet->data);
				this->handlePartnerNotificationMessage(packet->header.sender, packetData);
			}
			else if(packet->actionType == InukModuleTriggerActionMessages::MESSAGE_PING_LIGHT && sendData->dataLength >= SIZEOF_PING_MESSAGE){
				InukPingLightMessage const * packetData = (InukPingLightMessage const *) (packet->data);
				uint16_t pingTimeInMs = packetData->timeoutInMs;
				logs("ping light pingTimeInMs : [ %u ]", pingTimeInMs);
				this->p_iioModule->lioPing(pingTimeInMs);
			}
			else if(packet->actionType == InukModuleTriggerActionMessages::MESSAGE_GET_DEVICE_INFO && sendData->dataLength >= SIZEOF_INUK_GET_DEVICE_INFO_MESSAGE){
				InukGetDeviceInfoMessage const * packetData = (InukGetDeviceInfoMessage const *) (packet->data);
				logs("messgatype MESSAGE_GET_DEVICE_INFO [ sender : %u ] [ deviceInfoType : %u ] ", packet->header.sender, packetData->deviceInfoType);
				switch ((u16)packetData->deviceInfoType) {
					case GET_DEVICE_INFO:
						this->sendDeviceInfoPacket(packet->header.sender, packetData->nodeId, packet->requestHandle);
					break;

					case GET_PARTNER_IDS:
						this->sendPartnerIdsPacket(packet->header.sender, packetData->nodeId, packet->requestHandle);
					break;
				}
				
			}
			else {
				logs("No message handler for type %u", (u8) packet->actionType);
			}
		}
	}

	//Parse Module responses
	if(packetHeader->messageType == MessageType::MODULE_ACTION_RESPONSE){
		ConnPacketModule const * packet = (ConnPacketModule const *)packetHeader;

		//Check if our module is meant and we should trigger an action
		if(packet->moduleId == moduleId)
		{
			if(packet->actionType == InukModuleActionResponseMessages::MESSAGE_TEST_RESPONSE)
			{

			}
		}
	}
}
