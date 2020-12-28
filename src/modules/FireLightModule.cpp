////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// **
// ****************************************************************************/
////////////////////////////////////////////////////////////////////////////////

#include <FireLightModule.h>
#include <Logger.h>
#include <Utility.h>
#include <Node.h>
#include <GlobalState.h>
#include <FireLightTypes.h>
#include <stdlib.h>
#include <Config.h>


FireLightModule::FireLightModule()
	: Module(ModuleId::FireLight_MODULE, "FireLight")
{
	//Register callbacks n' stuff

	//Save configuration to base class variables
	//sizeof configuration must be a multiple of 4 bytes
	configurationPointer = &configuration;
	configurationLength = sizeof(FireLightModuleConfiguration);
	notifiedPartnerId = 0;

	//Set defaults
	ResetToDefaultConfiguration();

}

void FireLightModule::ResetToDefaultConfiguration()
{
	//Set default configuration values
	configuration.moduleId = moduleId;
	configuration.moduleActive = true;
	configuration.moduleVersion = FireLight_MODULE_CONFIG_VERSION;

	mode = FireLightLightModes::AUTOMATIC;

	//Set additional config values...
	this->p_iioModule = (FireLightIOModule *)GS->node.GetModuleById(ModuleId::FireLightIO_MODULE);
	if (this->p_iioModule != nullptr) {
		this->p_iioModule->setPIRCallback (this->pirCallback); 
	}
}

void FireLightModule::ConfigurationLoadedHandler(u8* migratableConfig, u16 migratableConfigLength) 
{
	//Version migration can be added here, e.g. if module has version 2 and config is version 1
	//if(migratableConfig->moduleVersion == 1){/* ... */};

	//Do additional initialization upon loading the config

	logs("Partner IDs are previousLightId :[ %u ] followingLightId : [ %u ]" ,
		(u16) configuration.previousLightId, 
		(u16) configuration.followingLightId);

}

void FireLightModule::TimerEventHandler(u16 passedTimeDs)
{
	//Do stuff on timer...
	// logs("FireLight timer %u", passedTimeDs);
	
	//this->handleSM();
}

#ifdef TERMINAL_ENABLED
	TerminalCommandHandlerReturnType FireLightModule::TerminalCommandHandler(const char* commandArgs[], u8 commandArgsSize)
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

void FireLightModule::pirCallback (u16 state) {
	

	FireLightModule * p_mod = (FireLightModule *)GS->node.GetModuleById(ModuleId::FireLight_MODULE);

	if (p_mod) {
		p_mod->handlePIRCallback((u16 )state);
	}

	 /*FireLightIOModule * p_iio = (FireLightIOModule *)GS->node.GetModuleById(ModuleId::FireLightIO_MODULE);
	 if (p_iio != nullptr) {
		 if (p_iio->getPirSensorState() == PIR_ON) {
			 p_iio->setLIOLightMode(true);
			
		 }  
	}*/
}

void FireLightModule::handlePIRCallback(u16 pirState) {
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

void FireLightModule::ButtonHandler(u8 buttonId, u32 holdTimeDs) {
	logs("ButtonHandler %d", holdTimeDs);
	this->p_iioModule->triggerPIRManual();
	//this->p_iioModule->lioPing(1000);

	
	// create data
	// FireLightModuleMessage data;
	// data.vsolar = 1111;
	// data.vbat   = 1234;
	// data.lightCommandMessage   = FireLightLightMessages::PIR_TRIGGER;

	// u8 targetNodeId = NODE_ID_BROADCAST;

	// //Send ping packet to that node
	// SendModuleActionMessage(
	// 		MessageType::MODULE_TRIGGER_ACTION,
	// 		targetNodeId,
	// 		FireLightModuleTriggerActionMessages::MESSAGE_TEST,
	// 		0,
	// 		(u8*)&data,
	// 		SIZEOF_FireLight_MODULE_MESSAGE,
	// 		false
	// );
}

void FireLightModule::handleSM ( void ) {

	if (mode == FireLightLightModes::AUTOMATIC) {
		switch (currentState)
		{
			case FireLightStates::ERROR :
				logs("FireLight state ERROR");
				break;
			case FireLightStates::STARTED :
				logs("FireLight state STARTED");
			
				break;
			case FireLightStates::WAITING_FOR_TRIGGER :
				logs("FireLight state WAITING_FOR_TRIGGER");
			
				break;
			case FireLightStates::TRIGGER_OCCURED :
				logs("FireLight state TRIGGER_OCCURED");
			
				break;
		
			default:
				break;
		}
	} else if (mode == FireLightLightModes::MANUAL) {

	}
	
}

void FireLightModule::setLighLeveltManual (u8 level) {
	// change mode to manual
	mode = FireLightLightModes::MANUAL;

	logs("setLightManual :  %u", level);
	this->p_iioModule->setLIOManual(level);
}

void FireLightModule::saveModuleConfiguration (void) {
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

void FireLightModule::sendDeviceInfoPacket (  NodeId senderNode, NodeId targetNode, u8 requestHandle ) {

	FireLightDeviceInfoMessage data;
	data.nodeId = targetNode;
	data.vsolar = p_iioModule->getSolarVoltage();
	data.vbat   = p_iioModule->getBatteryVoltage();
	data.pirState = p_iioModule->getPirSensorState();

	logs("sendDeviceInfoPacket [ vsolar : %u ] [ vbat : %u ] [ pirState : %u ]", data.vsolar, data.vbat, data.pirState);

	 SendModuleActionMessage(
        MessageType::MODULE_ACTION_RESPONSE,
        senderNode,
        (u8)FireLightModuleActionResponseMessages::MESSAGE_DEVICE_INFO_RESPONSE,
        requestHandle,
        (u8*)&data,
        SIZEOF_FireLight_DEVICE_INFO_MESSAGE,
        false
    );
}

void FireLightModule::sendPartnerIdsPacket (  NodeId senderNode, NodeId targetNode, u8 requestHandle ) {

	FireLightSetPartnerIDsMessage data;
	data.nodeId = targetNode;
	data.previousLightId = (u16) configuration.previousLightId;
	data.followingLightId = (u16) configuration.followingLightId;

	logs("sendPartnerIdsPacket [ nodeId : %u ] [ previousLightId : %u ] [ followingLightId : %u ]", 
			data.nodeId, data.previousLightId, data.followingLightId);

	 SendModuleActionMessage(
        MessageType::MODULE_ACTION_RESPONSE,
        senderNode,
        (u8)FireLightModuleActionResponseMessages::MESSAGE_DEVICE_INFO_RESPONSE,
        requestHandle,
        (u8*)&data,
        SIZEOF_FireLight_SET_PARTNER_MESSAGE,
        false
    );
}

void FireLightModule::sendGlowNotificationToPartner ( ) {

	// get partner IDs
	uint16_t previousLightId = configuration.previousLightId;
	uint16_t followingLightId = configuration.followingLightId;
	uint16_t targetNodeId = 0;

	logs("Partner IDs are previousLightId : [ %u ] followingLightId : [ %u ] notifiedPartnerId : [ %u ]" ,
		(u16) configuration.previousLightId, (u16) configuration.followingLightId, (u16) notifiedPartnerId);

	// direction --> [previous light] - [this light] - [following light]

	FireLightPartnerNotificationMessage data;
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
			(u8)FireLightModuleTriggerActionMessages::MESSAGE_PARTNER_NOTIFICATION,
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
			(u8)FireLightModuleTriggerActionMessages::MESSAGE_PARTNER_NOTIFICATION,
			0,
			(u8*)&data,
			SIZEOF_PARTNER_NOTIFICATION_MESSAGE,
			false);
		
	} else {
		logs("no partner ids were set");
	}		

	notifiedPartnerId = 0;
	
}

void FireLightModule::setPartnerLights (u16 previousLightId, u16 followingLightId) {
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

void FireLightModule::handlePartnerNotificationMessage (NodeId senderNode, const FireLightPartnerNotificationMessage *packetData) {
	
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

void FireLightModule::MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader)
{
	//Must call superclass for handling
	Module::MeshMessageReceivedHandler(connection, sendData, packetHeader);

	if(packetHeader->messageType == MessageType::MODULE_TRIGGER_ACTION){
		ConnPacketModule const * packet = (ConnPacketModule const *)packetHeader;

		//Check if our module is meant and we should trigger an action
		if(packet->moduleId == moduleId ){

			if(packet->actionType == FireLightModuleTriggerActionMessages::MESSAGE_SET_PARTNER && sendData->dataLength >= SIZEOF_FireLight_SET_PARTNER_MESSAGE){
		 		FireLightSetPartnerIDsMessage const * packetData = (FireLightSetPartnerIDsMessage const *) (packet->data);
				this->setPartnerLights ((u16) packetData->previousLightId, (u16) packetData->followingLightId);
			}
			else if(packet->actionType == FireLightModuleTriggerActionMessages::MESSAGE_SET_LIGHT_LEVEL && sendData->dataLength >= SIZEOF_FireLight_SET_LIGHT_LEVEL_MESSAGE){
				FireLightSetLightLevelMessage const * packetData = (FireLightSetLightLevelMessage const *) (packet->data);
				this->setLighLeveltManual ((u8) packetData->level);
			}
			else if(packet->actionType == FireLightModuleTriggerActionMessages::MESSAGE_PARTNER_NOTIFICATION && sendData->dataLength >= SIZEOF_PARTNER_NOTIFICATION_MESSAGE){
				FireLightPartnerNotificationMessage const * packetData = (FireLightPartnerNotificationMessage const *) (packet->data);
				this->handlePartnerNotificationMessage(packet->header.sender, packetData);
			}
			else if(packet->actionType == FireLightModuleTriggerActionMessages::MESSAGE_PING_LIGHT && sendData->dataLength >= SIZEOF_PING_MESSAGE){
				FireLightPingLightMessage const * packetData = (FireLightPingLightMessage const *) (packet->data);
				uint16_t pingTimeInMs = packetData->timeoutInMs;
				logs("ping light pingTimeInMs : [ %u ]", pingTimeInMs);
				this->p_iioModule->lioPing(pingTimeInMs);
			}
			else if(packet->actionType == FireLightModuleTriggerActionMessages::MESSAGE_GET_DEVICE_INFO && sendData->dataLength >= SIZEOF_FireLight_GET_DEVICE_INFO_MESSAGE){
				FireLightGetDeviceInfoMessage const * packetData = (FireLightGetDeviceInfoMessage const *) (packet->data);
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
			if(packet->actionType == FireLightModuleActionResponseMessages::MESSAGE_TEST_RESPONSE)
			{

			}
		}
	}
}
