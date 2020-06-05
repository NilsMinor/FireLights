////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// ** Copyright (C) 2015-2020 M-Way Solutions GmbH
// ** Contact: https://www.blureange.io/licensing
// **
// ** This file is part of the Bluerange/FruityMesh implementation
// **
// ** $BR_BEGIN_LICENSE:GPL-EXCEPT$
// ** Commercial License Usage
// ** Licensees holding valid commercial Bluerange licenses may use this file in
// ** accordance with the commercial license agreement provided with the
// ** Software or, alternatively, in accordance with the terms contained in
// ** a written agreement between them and M-Way Solutions GmbH. 
// ** For licensing terms and conditions see https://www.bluerange.io/terms-conditions. For further
// ** information use the contact form at https://www.bluerange.io/contact.
// **
// ** GNU General Public License Usage
// ** Alternatively, this file may be used under the terms of the GNU
// ** General Public License version 3 as published by the Free Software
// ** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
// ** included in the packaging of this file. Please review the following
// ** information to ensure the GNU General Public License requirements will
// ** be met: https://www.gnu.org/licenses/gpl-3.0.html.
// **
// ** $BR_END_LICENSE$
// **
// ****************************************************************************/
////////////////////////////////////////////////////////////////////////////////

#include <Logger.h>
#include "GlobalState.h"

#include <Terminal.h>
#include <Utility.h>
#include <mini-printf.h>

#ifdef SIM_ENABLED
#include "json.hpp"
#endif

// Size for tracing messages to the log transport, if it is too short, messages will get truncated
constexpr size_t TRACE_BUFFER_SIZE = 500;

Logger::Logger()
{
	CheckedMemset(errorLog, 0, sizeof(errorLog));
}

Logger & Logger::getInstance()
{
	return GS->logger;
}

void Logger::log_f(bool printLine, bool isJson, bool isEndOfMessage, bool skipJsonEvent, const char* file, i32 line, const char* message, ...)
{
	char mhTraceBuffer[TRACE_BUFFER_SIZE] = {};

	//Variable argument list must be passed to vnsprintf
	va_list aptr;
	va_start(aptr, message);
	vsnprintf(mhTraceBuffer, TRACE_BUFFER_SIZE, message, aptr);
	va_end(aptr);

	if (printLine)
	{
		char tmp[40];
		snprintf(tmp, 40, "[%s@%d]: ", file, line);
		log_transport_putstring(tmp);
		log_transport_putstring(mhTraceBuffer);
		log_transport_putstring(EOL);
	}
	else if(!isJson)
	{
		log_transport_putstring(mhTraceBuffer);
	}

	if (isJson)
	{
		if (GS->terminal.IsCrcChecksEnabled())
		{
			if (isEndOfMessage)
			{
				if (strchr(mhTraceBuffer, '}') == nullptr && strstr(mhTraceBuffer, "CRC:") == nullptr)
				{
					//Found end of json message that didn't contain a "}", which is
					//probably a  bug. Did you mean to split a json message accross
					//several lines? If so, use logjson_partial for all of the logs
					//except the last one (that should still be logjson!)
					SIMEXCEPTION(IllegalArgumentException);
				}
				char* sepLoc = strstr(mhTraceBuffer, SEP);
				if (sepLoc != nullptr)
				{
					sepLoc[0] = '\0';
				}
			}
			currentJsonCrc = Utility::CalculateCrc32String(mhTraceBuffer, currentJsonCrc);
		}

#ifdef SIM_ENABLED
		currentString += mhTraceBuffer;
#endif
		log_transport_putstring(mhTraceBuffer);

		if (isEndOfMessage)
		{
			if (GS->terminal.IsCrcChecksEnabled())
			{
				Logger::getInstance().log_f(false, false, false, false, "", 0, " CRC: %u" SEP, currentJsonCrc);
				currentJsonCrc = 0;
			}
#ifdef SIM_ENABLED
			nlohmann::json j = nlohmann::json::parse(currentString);
			currentString = "";
#endif
		}

		if (!skipJsonEvent) {
			GS->terminal.OnJsonLogged(mhTraceBuffer);
		}
	}
}

void Logger::logTag_f(LogType logType, const char* file, i32 line, const char* tag, const char* message, ...) const
{
#if IS_ACTIVE(LOGGING) && defined(TERMINAL_ENABLED)
#ifdef SIM_ENABLED
	if (strcmp(tag, "ERROR") == 0)
	{
		//ERRORs are classified as severe enough that they should not happend
		//during normal execution. If they are logged, something went wrong
		//and must be analyzed.
		SIMEXCEPTION(ErrorLoggedException);
	}
#endif

	if (
			//UART communication (json mode)
			(
				Conf::getInstance().terminalMode != TerminalMode::PROMPT
				&& (logEverything || logType == LogType::UART_COMMUNICATION || IsTagEnabled(tag))
			)
			//User interaction (prompt mode)
			|| (Conf::getInstance().terminalMode == TerminalMode::PROMPT
				&& (logEverything || logType == LogType::TRACE || IsTagEnabled(tag))
			)
		)
	{
		char mhTraceBuffer[TRACE_BUFFER_SIZE] = {};

		//Variable argument list must be passed to vsnprintf
		va_list aptr;
		va_start(aptr, message);
		vsnprintf(mhTraceBuffer, TRACE_BUFFER_SIZE, message, aptr);
		va_end(aptr);

		if(Conf::getInstance().terminalMode == TerminalMode::PROMPT){
			if (logType == LogType::LOG_LINE)
			{
				char tmp[50];
#ifndef SIM_ENABLED
				snprintf(tmp, 50, "[%s@%d %s]: ", file, line, tag);
#else
				snprintf(tmp, 50, "%07u:%u:[%s@%d %s]: ", GS->node.IsInit() ? GS->appTimerDs : 0, RamConfig->defaultNodeId, file, line, tag);
#endif
				log_transport_putstring(tmp);
				log_transport_putstring(mhTraceBuffer);
				log_transport_putstring(EOL);
			}
			else if (logType == LogType::LOG_MESSAGE_ONLY || logType == LogType::TRACE)
			{
				log_transport_putstring(mhTraceBuffer);
			}
		} else {
			logjson_partial_skip_event("LOG", "{\"type\":\"log\",\"tag\":\"%s\",\"file\":\"%s\",\"line\":%d,\"message\":\"", tag, file, line);
			logjson_skip_event("LOG", "%s\"}" SEP, mhTraceBuffer);
		}
	}
#endif
}

void Logger::uart_error_f(UartErrorType type) const
{
	switch (type)
	{
		case UartErrorType::SUCCESS:
			logjson("ERROR", "{\"type\":\"error\",\"code\":0,\"text\":\"OK\"}" SEP);
			break;
		case UartErrorType::COMMAND_NOT_FOUND:
			logjson("ERROR", "{\"type\":\"error\",\"code\":1,\"text\":\"Command not found\"}" SEP);
			break;
		case UartErrorType::ARGUMENTS_WRONG:
			logjson("ERROR", "{\"type\":\"error\",\"code\":2,\"text\":\"Wrong Arguments\"}" SEP);
			break;
		case UartErrorType::TOO_MANY_ARGUMENTS:
			logjson("ERROR", "{\"type\":\"error\",\"code\":3,\"text\":\"Too many arguments\"}" SEP);
			break;
		case UartErrorType::TOO_FEW_ARGUMENTS:
			logjson("ERROR", "{\"type\":\"error\",\"code\":4,\"text\":\"Too few arguments\"}" SEP);
			break;
#if IS_INACTIVE(SAVE_SPACE)
		case UartErrorType::WARN_DEPRECATED:
			logjson("ERROR", "{\"type\":\"error\",\"code\":5,\"text\":\"Warning: Command is marked deprecated!\"}" SEP);
			break;
#endif
		case UartErrorType::CRC_INVALID:
			logjson("ERROR", "{\"type\":\"error\",\"code\":6,\"text\":\"crc invalid\"}" SEP);
			break;
		case UartErrorType::CRC_MISSING:
			logjson("ERROR", "{\"type\":\"error\",\"code\":7,\"text\":\"crc missing\"}" SEP);
			break;
		case UartErrorType::INTERNAL_ERROR:
			logjson("ERROR", "{\"type\":\"error\",\"code\":8,\"text\":\"internal error\"}" SEP);
			break;
		default:
			logjson("ERROR", "{\"type\":\"error\",\"code\":%u,\"text\":\"Unknown Error\"}" SEP, (u32)type);
			break;
	}
}

void Logger::enableTag(const char* tag)
{
#if IS_ACTIVE(LOGGING) && defined(TERMINAL_ENABLED)

	if (strlen(tag) + 1 > MAX_LOG_TAG_LENGTH) {
		logt("ERROR", "Too long");				//LCOV_EXCL_LINE assertion
		SIMEXCEPTION(IllegalArgumentException);	//LCOV_EXCL_LINE assertion
		return;									//LCOV_EXCL_LINE assertion
	}

	char tagUpper[MAX_LOG_TAG_LENGTH];
	strcpy(tagUpper, tag);
	Utility::ToUpperCase(tagUpper);

	i32 emptySpot = -1;
	bool found = false;

	for (i32 i = 0; i < MAX_ACTIVATE_LOG_TAG_NUM; i++) {
		if (activeLogTags[i * MAX_LOG_TAG_LENGTH] == '\0' && emptySpot < 0) emptySpot = i;
		if (strcmp(&activeLogTags[i * MAX_LOG_TAG_LENGTH], tagUpper) == 0) found = true;
	}

	if (!found && emptySpot >= 0) {
		strcpy(&activeLogTags[emptySpot * MAX_LOG_TAG_LENGTH], tagUpper);
	}
	else if (!found && emptySpot < 0)
	{
		logt("ERROR", "Too many tags");
		SIMEXCEPTION(IllegalStateException);
	}

	return;

#endif
}

bool Logger::IsTagEnabled(const char* tag) const
{
#if IS_ACTIVE(LOGGING) && defined(TERMINAL_ENABLED)

	if (strcmp(tag, "ERROR") == 0 || strcmp(tag, "WARNING") == 0) {
		return true;
	}
	for (u32 i = 0; i < MAX_ACTIVATE_LOG_TAG_NUM; i++)
	{
		if (strcmp(&activeLogTags[i * MAX_LOG_TAG_LENGTH], tag) == 0) return true;
	}
#endif

	return false;
}

void Logger::disableTag(const char* tag)
{
#if IS_ACTIVE(LOGGING) && defined(TERMINAL_ENABLED)

	char tagUpper[MAX_LOG_TAG_LENGTH];
	strcpy(tagUpper, tag);
	Utility::ToUpperCase(tagUpper);

	for (u32 i = 0; i < MAX_ACTIVATE_LOG_TAG_NUM; i++) {
		if (strcmp(&activeLogTags[i * MAX_LOG_TAG_LENGTH], tagUpper) == 0) {
			activeLogTags[i * MAX_LOG_TAG_LENGTH] = '\0';
			return;
		}
	}

#endif
}

void Logger::toggleTag(const char* tag)
{
#if IS_ACTIVE(LOGGING) && defined(TERMINAL_ENABLED)
	
	if (strlen(tag) + 1 > MAX_LOG_TAG_LENGTH) {
		logt("ERROR", "Too long");				//LCOV_EXCL_LINE assertion
		SIMEXCEPTION(IllegalArgumentException);	//LCOV_EXCL_LINE assertion
		return;									//LCOV_EXCL_LINE assertion
	}

	char tagUpper[MAX_LOG_TAG_LENGTH];
	strcpy(tagUpper, tag);
	Utility::ToUpperCase(tagUpper);

	//First, check if it is enabled and disable it after it was found
	bool found = false;
	i32 emptySpot = -1;
	for (u32 i = 0; i < MAX_ACTIVATE_LOG_TAG_NUM; i++) {
		if (activeLogTags[i * MAX_LOG_TAG_LENGTH] == '\0' && emptySpot < 0) emptySpot = i;
		if (strcmp(&activeLogTags[i * MAX_LOG_TAG_LENGTH], tagUpper) == 0) {
			activeLogTags[i * MAX_LOG_TAG_LENGTH] = '\0';
			found = true;
			// => Do not return or break as we are still looking for an empty spot
		}
	}

	//If we haven't found it, we enable it by using the previously found empty spot
	if (!found && emptySpot >= 0) {
		strcpy(&activeLogTags[emptySpot * MAX_LOG_TAG_LENGTH], tagUpper);
		logt("WARNING", "Tag enabled");
	}
	else if (!found && emptySpot < 0) {
		logt("ERROR", "Too many tags");
		SIMEXCEPTION(IllegalStateException);
	}
	else if (found)
	{
		logt("WARNING", "Tag disabled");
	}

#endif
}

u32 Logger::getAmountOfEnabledTags()
{
#if IS_ACTIVE(LOGGING) && defined(TERMINAL_ENABLED)
	u32 amount = 0;
	for (i32 i = 0; i < MAX_ACTIVATE_LOG_TAG_NUM; i++) {
		if (activeLogTags[i * MAX_LOG_TAG_LENGTH] != '\0') amount++;
	}
	return amount;
#else
	return 0;
#endif // IS_ACTIVE(LOGGING) && defined(TERMINAL_ENABLED)
}

void Logger::printEnabledTags() const
{
#if IS_ACTIVE(LOGGING) && defined(TERMINAL_ENABLED)
	
	if (logEverything) trace("LOG ALL IS ACTIVE" EOL);
	for (u32 i = 0; i < MAX_ACTIVATE_LOG_TAG_NUM; i++) {
		if (activeLogTags[i * MAX_LOG_TAG_LENGTH] != '\0') {
			trace("%s" EOL, &activeLogTags[i * MAX_LOG_TAG_LENGTH]);
		}
	}

#endif
}

#ifdef TERMINAL_ENABLED
TerminalCommandHandlerReturnType Logger::TerminalCommandHandler(const char* commandArgs[], u8 commandArgsSize)
{
#if IS_ACTIVE(LOGGING) && defined(TERMINAL_ENABLED)
	if (TERMARGS(0, "debug") && commandArgsSize >= 2)
	{
		if (TERMARGS(1, "all"))
		{
			logEverything = !logEverything;
		}
		else if (TERMARGS(1, "none"))
		{
			disableAll();
		}
		else
		{
			toggleTag(commandArgs[1]);
		}

		return TerminalCommandHandlerReturnType::SUCCESS;
	}
	else if (TERMARGS(0, "debugtags"))
	{
		printEnabledTags();

		return TerminalCommandHandlerReturnType::SUCCESS;
	}
	else if (TERMARGS(0, "errors"))
	{
		for(int i=0; i<errorLogPosition; i++){
			if(errorLog[i].errorType == LoggingError::HCI_ERROR)
			{
				trace("HCI %u %s @%u" EOL, errorLog[i].errorCode, Logger::getHciErrorString((FruityHal::BleHciError)errorLog[i].errorCode), errorLog[i].timestamp);
			}
			else if(errorLog[i].errorType == LoggingError::GENERAL_ERROR)
			{
				trace("GENERAL %u %s @%u" EOL, errorLog[i].errorCode, Logger::getGeneralErrorString((ErrorType)errorLog[i].errorCode), errorLog[i].timestamp);
			} else {
				trace("CUSTOM %u %u @%u" EOL, (u32)errorLog[i].errorType, errorLog[i].errorCode, errorLog[i].timestamp);
			}
		}

		return TerminalCommandHandlerReturnType::SUCCESS;
	}

#endif
	return TerminalCommandHandlerReturnType::UNKNOWN;
}
#endif

const char* Logger::getErrorLogErrorType(LoggingError type)
{
#if defined(TERMINAL_ENABLED)
	switch (type)
	{
	case LoggingError::GENERAL_ERROR:
		return "GENERAL_ERROR";
	case LoggingError::HCI_ERROR:
		return "HCI_ERROR";
	case LoggingError::CUSTOM:
		return "CUSTOM";
	case LoggingError::GATT_STATUS:
		return "GATT_STATUS";
	case LoggingError::REBOOT:
		return "REBOOT";
	default:
		SIMEXCEPTION(ErrorCodeUnknownException); //Could be an error or should be added to the list
		return "UNKNOWN_ERROR";
	}
#else
	return nullptr;
#endif
}

const char* Logger::getErrorLogCustomError(CustomErrorTypes type)
{
#if defined(TERMINAL_ENABLED)
	switch (type)
	{
	case CustomErrorTypes::FATAL_BLE_GATTC_EVT_TIMEOUT_FORCED_US:
		return "FATAL_BLE_GATTC_EVT_TIMEOUT_FORCED_US";
	case CustomErrorTypes::INFO_TRYING_CONNECTION_SUSTAIN:
		return "INFO_TRYING_CONNECTION_SUSTAIN";
	case CustomErrorTypes::WARN_CONNECTION_SUSTAIN_FAILED_TO_ESTABLISH:
		return "WARN_CONNECTION_SUSTAIN_FAILED_TO_ESTABLISH";
	case CustomErrorTypes::COUNT_CONNECTION_SUCCESS:
		return "COUNT_CONNECTION_SUCCESS";
	case CustomErrorTypes::COUNT_HANDSHAKE_DONE:
		return "COUNT_HANDSHAKE_DONE";
	case CustomErrorTypes::WARN_HANDSHAKE_TIMEOUT:
		return "WARN_HANDSHAKE_TIMEOUT";
	case CustomErrorTypes::WARN_CM_FAIL_NO_SPOT:
		return "WARN_CM_FAIL_NO_SPOT";
	case CustomErrorTypes::FATAL_QUEUE_NUM_MISMATCH:
		return "FATAL_QUEUE_NUM_MISMATCH";
	case CustomErrorTypes::WARN_GATT_WRITE_ERROR:
		return "WARN_GATT_WRITE_ERROR";
	case CustomErrorTypes::WARN_TX_WRONG_DATA:
		return "WARN_TX_WRONG_DATA";
	case CustomErrorTypes::WARN_RX_WRONG_DATA:
		return "WARN_RX_WRONG_DATA";
	case CustomErrorTypes::FATAL_CLUSTER_UPDATE_FLOW_MISMATCH:
		return "FATAL_CLUSTER_UPDATE_FLOW_MISMATCH";
	case CustomErrorTypes::WARN_HIGH_PRIO_QUEUE_FULL:
		return "WARN_HIGH_PRIO_QUEUE_FULL";
	case CustomErrorTypes::COUNT_NO_PENDING_CONNECTION:
		return "COUNT_NO_PENDING_CONNECTION";
	case CustomErrorTypes::FATAL_HANDLE_PACKET_SENT_ERROR:
		return "FATAL_HANDLE_PACKET_SENT_ERROR";
	case CustomErrorTypes::COUNT_DROPPED_PACKETS:
		return "COUNT_DROPPED_PACKETS";
	case CustomErrorTypes::COUNT_SENT_PACKETS_RELIABLE:
		return "COUNT_SENT_PACKETS_RELIABLE";
	case CustomErrorTypes::COUNT_SENT_PACKETS_UNRELIABLE:
		return "COUNT_SENT_PACKETS_UNRELIABLE";
	case CustomErrorTypes::INFO_ERRORS_REQUESTED:
		return "INFO_ERRORS_REQUESTED";
	case CustomErrorTypes::INFO_CONNECTION_SUSTAIN_SUCCESS:
		return "INFO_CONNECTION_SUSTAIN_SUCCESS";
	case CustomErrorTypes::COUNT_JOIN_ME_RECEIVED:
		return "COUNT_JOIN_ME_RECEIVED";
	case CustomErrorTypes::WARN_CONNECT_AS_MASTER_NOT_POSSIBLE:
		return "WARN_CONNECT_AS_MASTER_NOT_POSSIBLE";
	case CustomErrorTypes::FATAL_PENDING_NOT_CLEARED:
		return "FATAL_PENDING_NOT_CLEARED";
	case CustomErrorTypes::FATAL_PROTECTED_PAGE_ERASE:
		return "FATAL_PROTECTED_PAGE_ERASE";
	case CustomErrorTypes::INFO_IGNORING_CONNECTION_SUSTAIN:
		return "INFO_IGNORING_CONNECTION_SUSTAIN";
	case CustomErrorTypes::INFO_IGNORING_CONNECTION_SUSTAIN_LEAF:
		return "INFO_IGNORING_CONNECTION_SUSTAIN_LEAF";
	case CustomErrorTypes::COUNT_GATT_CONNECT_FAILED:
		return "COUNT_GATT_CONNECT_FAILED";
	case CustomErrorTypes::FATAL_PACKET_PROCESSING_FAILED:
		return "FATAL_PACKET_PROCESSING_FAILED";
	case CustomErrorTypes::FATAL_PACKET_TOO_BIG:
		return "FATAL_PACKET_TOO_BIG";
	case CustomErrorTypes::COUNT_HANDSHAKE_ACK1_DUPLICATE:
		return "COUNT_HANDSHAKE_ACK1_DUPLICATE";
	case CustomErrorTypes::COUNT_HANDSHAKE_ACK2_DUPLICATE:
		return "COUNT_HANDSHAKE_ACK2_DUPLICATE";
	case CustomErrorTypes::COUNT_ENROLLMENT_NOT_SAVED:
		return "COUNT_ENROLLMENT_NOT_SAVED";
	case CustomErrorTypes::COUNT_FLASH_OPERATION_ERROR:
		return "COUNT_FLASH_OPERATION_ERROR";
	case CustomErrorTypes::FATAL_WRONG_FLASH_STORAGE_COMMAND:
		return "FATAL_WRONG_FLASH_STORAGE_COMMAND";
	case CustomErrorTypes::FATAL_ABORTED_FLASH_TRANSACTION:
		return "FATAL_ABORTED_FLASH_TRANSACTION";
	case CustomErrorTypes::FATAL_PACKETQUEUE_PACKET_TOO_BIG:
		return "FATAL_PACKETQUEUE_PACKET_TOO_BIG";
	case CustomErrorTypes::FATAL_NO_RECORDSTORAGE_SPACE_LEFT:
		return "FATAL_NO_RECORDSTORAGE_SPACE_LEFT";
	case CustomErrorTypes::FATAL_RECORD_CRC_WRONG:
		return "FATAL_RECORD_CRC_WRONG";
	case CustomErrorTypes::COUNT_3RD_PARTY_TIMEOUT:
		return "COUNT_3RD_PARTY_TIMEOUT";
	case CustomErrorTypes::FATAL_CONNECTION_ALLOCATOR_OUT_OF_MEMORY:
		return "FATAL_CONNECTION_ALLOCATOR_OUT_OF_MEMORY";
	case CustomErrorTypes::FATAL_CONNECTION_REMOVED_WHILE_TIME_SYNC:
		return "FATAL_CONNECTION_REMOVED_WHILE_TIME_SYNC";
	case CustomErrorTypes::FATAL_COULD_NOT_RETRIEVE_CAPABILITIES:
		return "FATAL_COULD_NOT_RETRIEVE_CAPABILITIES";
	case CustomErrorTypes::FATAL_INCORRECT_HOPS_TO_SINK:
		return "FATAL_INCORRECT_HOPS_TO_SINK";
	case CustomErrorTypes::WARN_SPLIT_PACKET_MISSING:
		return "WARN_SPLIT_PACKET_MISSING";
	case CustomErrorTypes::WARN_SPLIT_PACKET_NOT_IN_MTU:
		return "WARN_SPLIT_PACKET_NOT_IN_MTU";
	case CustomErrorTypes::FATAL_DFU_FLASH_OPERATION_FAILED:
		return "FATAL_DFU_FLASH_OPERATION_FAILED";
	case CustomErrorTypes::WARN_RECORD_STORAGE_ERASE_CYCLES_HIGH:
		return "WARN_RECORD_STORAGE_ERASE_CYCLES_HIGH";
	case CustomErrorTypes::FATAL_RECORD_STORAGE_ERASE_CYCLES_HIGH:
		return "FATAL_RECORD_STORAGE_ERASE_CYCLES_HIGH";
	case CustomErrorTypes::FATAL_RECORD_STORAGE_COULD_NOT_FIND_SWAP_PAGE:
		return "FATAL_RECORD_STORAGE_COULD_NOT_FIND_SWAP_PAGE";
	case CustomErrorTypes::WARN_MTU_UPGRADE_FAILED:
		return "WARN_MTU_UPGRADE_FAILED";
	case CustomErrorTypes::WARN_ENROLLMENT_ERASE_FAILED:
		return "WARN_ENROLLMENT_ERASE_FAILED";
	case CustomErrorTypes::FATAL_RECORD_STORAGE_UNLOCK_FAILED:
		return "FATAL_RECORD_STORAGE_UNLOCK_FAILED";
	case CustomErrorTypes::WARN_ENROLLMENT_LOCK_DOWN_FAILED:
		return "WARN_ENROLLMENT_LOCK_DOWN_FAILED";
	case CustomErrorTypes::WARN_CONNECTION_SUSTAIN_FAILED:
		return "WARN_CONNECTION_SUSTAIN_FAILED";
	case CustomErrorTypes::FATAL_SENSOR_PINS_NOT_DEFINED_IN_BOARD_ID:
		return "FATAL_SENSOR_PINS_NOT_DEFINED_IN_BOARD_ID";
	default:
		SIMEXCEPTION(ErrorCodeUnknownException); //Could be an error or should be added to the list
		return "UNKNOWN_ERROR";
	}
#else
	return nullptr;
#endif
}

const char* Logger::getGattStatusErrorString(FruityHal::BleGattEror gattStatusCode)
{
#if defined(TERMINAL_ENABLED)
	switch (gattStatusCode)
	{
	case FruityHal::BleGattEror::SUCCESS:
		return "Success";
	case FruityHal::BleGattEror::UNKNOWN:
		return "Unknown or not applicable status";
	case FruityHal::BleGattEror::READ_NOT_PERMITTED:
		return "ATT Error: Read not permitted";
	case FruityHal::BleGattEror::WRITE_NOT_PERMITTED:
		return "ATT Error: Write not permitted";
	case FruityHal::BleGattEror::INVALID_PDU:
		return "ATT Error: Used in ATT as Invalid PDU";
	case FruityHal::BleGattEror::INSUF_AUTHENTICATION:
		return "ATT Error: Authenticated link required";
	case FruityHal::BleGattEror::REQUEST_NOT_SUPPORTED:
		return "ATT Error: Used in ATT as Request Not Supported";
	case FruityHal::BleGattEror::INVALID_OFFSET:
		return "ATT Error: Offset specified was past the end of the attribute";
	case FruityHal::BleGattEror::INSUF_AUTHORIZATION:
		return "ATT Error: Used in ATT as Insufficient Authorisation";
	case FruityHal::BleGattEror::PREPARE_QUEUE_FULL:
		return "ATT Error: Used in ATT as Prepare Queue Full";
	case FruityHal::BleGattEror::ATTRIBUTE_NOT_FOUND:
		return "ATT Error: Used in ATT as Attribute not found";
	case FruityHal::BleGattEror::ATTRIBUTE_NOT_LONG:
		return "ATT Error: Attribute cannot be read or written using read/write blob requests";
	case FruityHal::BleGattEror::INSUF_ENC_KEY_SIZE:
		return "ATT Error: Encryption key size used is insufficient";
	case FruityHal::BleGattEror::INVALID_ATT_VAL_LENGTH:
		return "ATT Error: Invalid value size";
	case FruityHal::BleGattEror::UNLIKELY_ERROR:
		return "ATT Error: Very unlikely error";
	case FruityHal::BleGattEror::INSUF_ENCRYPTION:
		return "ATT Error: Encrypted link required";
	case FruityHal::BleGattEror::UNSUPPORTED_GROUP_TYPE:
		return "ATT Error: Attribute type is not a supported grouping attribute";
	case FruityHal::BleGattEror::INSUF_RESOURCES:
		return "ATT Error: Encrypted link required";
	case FruityHal::BleGattEror::RFU_RANGE1_BEGIN:
		return "ATT Error: Reserved for Future Use range #1 begin";
	case FruityHal::BleGattEror::RFU_RANGE1_END:
		return "ATT Error: Reserved for Future Use range #1 end";
	case FruityHal::BleGattEror::APP_BEGIN:
		return "ATT Error: Application range begin";
	case FruityHal::BleGattEror::APP_END:
		return "ATT Error: Application range end";
	case FruityHal::BleGattEror::RFU_RANGE2_BEGIN:
		return "ATT Error: Reserved for Future Use range #2 begin";
	case FruityHal::BleGattEror::RFU_RANGE2_END:
		return "ATT Error: Reserved for Future Use range #2 end";
	case FruityHal::BleGattEror::RFU_RANGE3_BEGIN:
		return "ATT Error: Reserved for Future Use range #3 begin";
	case FruityHal::BleGattEror::RFU_RANGE3_END:
		return "ATT Error: Reserved for Future Use range #3 end";
	case FruityHal::BleGattEror::CPS_CCCD_CONFIG_ERROR:
		return "ATT Common Profile and Service Error: Client Characteristic Configuration Descriptor improperly configured";
	case FruityHal::BleGattEror::CPS_PROC_ALR_IN_PROG:
		return "ATT Common Profile and Service Error: Procedure Already in Progress";
	case FruityHal::BleGattEror::CPS_OUT_OF_RANGE:
		return "ATT Common Profile and Service Error: Out Of Range";
	default:
		SIMEXCEPTION(ErrorCodeUnknownException); //Could be an error or should be added to the list
		return "Unknown GATT status";
	}
#else
	return nullptr;
#endif
}

const char* Logger::getGeneralErrorString(ErrorType ErrorCode)
{
#if defined(TERMINAL_ENABLED)
	switch ((u32)ErrorCode)
	{
	case (u32)ErrorType::SUCCESS:
		return "SUCCESS";
	case (u32)ErrorType::SVC_HANDLER_MISSING:
		return "ERROR_SVC_HANDLER_MISSING";
	case (u32)ErrorType::BLE_STACK_NOT_ENABLED:
		return "ERROR_SOFTDEVICE_NOT_ENABLED";
	case (u32)ErrorType::INTERNAL:
		return "ERROR_INTERNAL";
	case (u32)ErrorType::NO_MEM:
		return "ERROR_NO_MEM";
	case (u32)ErrorType::NOT_FOUND:
		return "ERROR_NOT_FOUND";
	case (u32)ErrorType::NOT_SUPPORTED:
		return "ERROR_NOT_SUPPORTED";
	case (u32)ErrorType::INVALID_PARAM:
		return "ERROR_INVALID_PARAM";
	case (u32)ErrorType::INVALID_STATE:
		return "ERROR_INVALID_STATE";
	case (u32)ErrorType::INVALID_LENGTH:
		return "ERROR_INVALID_LENGTH";
	case (u32)ErrorType::INVALID_FLAGS:
		return "ERROR_INVALID_FLAGS";
	case (u32)ErrorType::INVALID_DATA:
		return "ERROR_INVALID_DATA";
	case (u32)ErrorType::DATA_SIZE:
		return "ERROR_DATA_SIZE";
	case (u32)ErrorType::TIMEOUT:
		return "ERROR_TIMEOUT";
	case (u32)ErrorType::NULL_ERROR:
		return "ERROR_NULL";
	case (u32)ErrorType::FORBIDDEN:
		return "ERROR_FORBIDDEN";
	case (u32)ErrorType::INVALID_ADDR:
		return "ERROR_INVALID_ADDR";
	case (u32)ErrorType::BUSY:
		return "ERROR_BUSY";
	case (u32)ErrorType::CONN_COUNT:
		return "CONN_COUNT";
	case (u32)ErrorType::BLE_INVALID_CONN_HANDLE:
		return "BLE_ERROR_INVALID_CONN_HANDLE";
	case (u32)ErrorType::BLE_INVALID_ATTR_HANDLE:
		return "BLE_ERROR_INVALID_ATTR_HANDLE";
	case (u32)ErrorType::BLE_NO_TX_PACKETS:
		return "BLE_ERROR_NO_TX_PACKETS";
	case 0xDEADBEEF:
		return "DEADBEEF";
	default:
		SIMEXCEPTION(ErrorCodeUnknownException); //Could be an error or should be added to the list
		return "UNKNOWN_ERROR";
	}
#else
	return nullptr;
#endif
}

const char* Logger::getHciErrorString(FruityHal::BleHciError hciErrorCode)
{
#if defined(TERMINAL_ENABLED)
	switch (hciErrorCode)
	{
	case FruityHal::BleHciError::SUCCESS:
		return "Success";

	case FruityHal::BleHciError::UNKNOWN_BLE_COMMAND:
		return "Unknown BLE Command";

	case FruityHal::BleHciError::UNKNOWN_CONNECTION_IDENTIFIER:
		return "Unknown Connection Identifier";

	case FruityHal::BleHciError::AUTHENTICATION_FAILURE:
		return "Authentication Failure";

	case FruityHal::BleHciError::CONN_FAILED_TO_BE_ESTABLISHED:
		return "Connection Failed to be Established";

	case FruityHal::BleHciError::CONN_INTERVAL_UNACCEPTABLE:
		return "Connection Interval Unacceptable";

	case FruityHal::BleHciError::CONN_TERMINATED_DUE_TO_MIC_FAILURE:
		return "Connection Terminated due to MIC Failure";

	case FruityHal::BleHciError::CONNECTION_TIMEOUT:
		return "Connection Timeout";

	case FruityHal::BleHciError::CONTROLLER_BUSY:
		return "Controller Busy";

	case FruityHal::BleHciError::DIFFERENT_TRANSACTION_COLLISION:
		return "Different Transaction Collision";

	case FruityHal::BleHciError::DIRECTED_ADVERTISER_TIMEOUT:
		return "Directed Adverisement Timeout";

	case FruityHal::BleHciError::INSTANT_PASSED:
		return "Instant Passed";

	case FruityHal::BleHciError::LOCAL_HOST_TERMINATED_CONNECTION:
		return "Local Host Terminated Connection";

	case FruityHal::BleHciError::MEMORY_CAPACITY_EXCEEDED:
		return "Memory Capacity Exceeded";

	case FruityHal::BleHciError::PAIRING_WITH_UNIT_KEY_UNSUPPORTED:
		return "Pairing with Unit Key Unsupported";

	case FruityHal::BleHciError::REMOTE_DEV_TERMINATION_DUE_TO_LOW_RESOURCES:
		return "Remote Device Terminated Connection due to low resources";

	case FruityHal::BleHciError::REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF:
		return "Remote Device Terminated Connection due to power off";

	case FruityHal::BleHciError::REMOTE_USER_TERMINATED_CONNECTION:
		return "Remote User Terminated Connection";

	case FruityHal::BleHciError::COMMAND_DISALLOWED:
		return "Command Disallowed";

	case FruityHal::BleHciError::INVALID_BLE_COMMAND_PARAMETERS:
		return "Invalid BLE Command Parameters";

	case FruityHal::BleHciError::INVALID_LMP_PARAMETERS:
		return "Invalid LMP Parameters";

	case FruityHal::BleHciError::LMP_PDU_NOT_ALLOWED:
		return "LMP PDU Not Allowed";

	case FruityHal::BleHciError::LMP_RESPONSE_TIMEOUT:
		return "LMP Response Timeout";

	case FruityHal::BleHciError::PIN_OR_KEY_MISSING:
		return "Pin or Key missing";

	case FruityHal::BleHciError::UNSPECIFIED_ERROR:
		return "Unspecified Error";

	case FruityHal::BleHciError::UNSUPPORTED_REMOTE_FEATURE:
		return "Unsupported Remote Feature";
	default:
		SIMEXCEPTION(ErrorCodeUnknownException); //Could be an error or should be added to the list
		return "Unknown HCI error";
	}
#else
	return nullptr;
#endif
}

const char* Logger::getErrorLogRebootReason(RebootReason type)
{
#if defined(TERMINAL_ENABLED)
	switch (type)
	{
	case RebootReason::UNKNOWN:
		return "UNKNOWN";
	case RebootReason::HARDFAULT:
		return "HARDFAULT";
	case RebootReason::APP_FAULT:
		return "APP_FAULT";
	case RebootReason::SD_FAULT:
		return "SD_FAULT";
	case RebootReason::PIN_RESET:
		return "PIN_RESET";
	case RebootReason::WATCHDOG:
		return "WATCHDOG";
	case RebootReason::FROM_OFF_STATE:
		return "FROM_OFF_STATE";
	case RebootReason::LOCAL_RESET:
		return "LOCAL_RESET";
	case RebootReason::REMOTE_RESET:
		return "REMOTE_RESET";
	case RebootReason::ENROLLMENT:
		return "ENROLLMENT";
	case RebootReason::PREFERRED_CONNECTIONS:
		return "PREFERRED_CONNECTIONS";
	case RebootReason::DFU:
		return "DFU";
	case RebootReason::MODULE_ALLOCATOR_OUT_OF_MEMORY:
		return "MODULE_ALLOCATOR_OUT_OF_MEMORY";
	case RebootReason::MEMORY_MANAGEMENT:
		return "MEMORY_MANAGEMENT";
	case RebootReason::BUS_FAULT:
		return "BUS_FAULT";
	case RebootReason::USAGE_FAULT:
		return "USAGE_FAULT";
	case RebootReason::ENROLLMENT_REMOVE:
		return "ENROLLMENT_REMOVE";
	case RebootReason::FACTORY_RESET_FAILED:
		return "FACTORY_RESET_FAILED";
	case RebootReason::FACTORY_RESET_SUCCEEDED_FAILSAFE:
		return "FACTORY_RESET_SUCCEEDED_FAILSAFE";
	default:
		SIMEXCEPTION(ErrorCodeUnknownException); //Could be an error or should be added to the list
		return "UNDEFINED";
	}
#else
	return nullptr;
#endif
}

const char * Logger::getErrorLogError(LoggingError type, u32 code)
{
#if defined(TERMINAL_ENABLED)
	switch (type)
	{
	case LoggingError::GENERAL_ERROR:
		return getGeneralErrorString((ErrorType)code);
	case LoggingError::HCI_ERROR:
		return getHciErrorString((FruityHal::BleHciError)code);
	case LoggingError::CUSTOM:
		return getErrorLogCustomError((CustomErrorTypes)code);
	case LoggingError::GATT_STATUS:
		return getGattStatusErrorString((FruityHal::BleGattEror)code);
	case LoggingError::REBOOT:
		return getErrorLogRebootReason((RebootReason)code);
	default:
		SIMEXCEPTION(ErrorCodeUnknownException); //Could be an error or should be added to the list
		return "UNKNOWN_TYPE";
	}
#else
	return nullptr;
#endif
}

void Logger::blePrettyPrintAdvData(SizedData advData) const
{

	trace("Rx Packet len %d: ", advData.length);

	u32 i = 0;
	SizedData fieldData;
	char hexString[100];
	//Loop through advertising data and parse it
	while (i < advData.length)
	{
		u8 fieldSize = advData.data[i];
		u8 fieldType = advData.data[i + 1];
		fieldData.data = advData.data + i + 2;
		fieldData.length = fieldSize - 1;

		//Print it
		Logger::convertBufferToHexString(fieldData.data, fieldData.length, hexString, sizeof(hexString));
		trace("Type %d, Data %s" EOL, fieldType, hexString);

		i += fieldSize + 1;
	}
}

void Logger::logError(LoggingError errorType, u32 errorCode, u32 extraInfo)
{
	errorLog[errorLogPosition].errorType = errorType;
	errorLog[errorLogPosition].errorCode = errorCode;
	errorLog[errorLogPosition].extraInfo = extraInfo;
	errorLog[errorLogPosition].timestamp = GS->node.IsInit() ? GS->timeManager.GetTime() : 0;

	//Will fill the error log until the last entry (last entry does get overwritten with latest value)
	if(errorLogPosition < NUM_ERROR_LOG_ENTRIES-1) errorLogPosition++;
}

void Logger::logCustomError(CustomErrorTypes customErrorType, u32 extraInfo)
{
	logError(LoggingError::CUSTOM, (u32)customErrorType, extraInfo);
}

//can be called multiple times and will increment the extra each time this happens
void Logger::logCount(LoggingError errorType, u32 errorCode)
{
	//Check if the erroLogEntry exists already and increment the extra if yes
	for (u32 i = 0; i < errorLogPosition; i++) {
		if (errorLog[i].errorType == errorType && errorLog[i].errorCode == errorCode) {
			errorLog[i].extraInfo++;
			return;
		}
	}

	//Create the entry
	errorLog[errorLogPosition].errorType = errorType;
	errorLog[errorLogPosition].errorCode = errorCode;
	errorLog[errorLogPosition].extraInfo = 1;
	errorLog[errorLogPosition].timestamp = GS->timeManager.GetTime();

	//Will fill the error log until the last entry (last entry does get overwritten with latest value)
	if (errorLogPosition < NUM_ERROR_LOG_ENTRIES - 1) errorLogPosition++;
}

void Logger::logCustomCount(CustomErrorTypes customErrorType)
{
	logCount(LoggingError::CUSTOM, (u32)customErrorType);
}

const char* base64Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void convertBase64Block(const u8 * srcBuffer, u32 blockLength, char* dstBuffer)
{
	if (blockLength == 0 || blockLength > 3) {
		//blockLength must be 1, 2 or 3
		SIMEXCEPTION(IllegalArgumentException); //LCOV_EXCL_LINE assertion
	}

	                    dstBuffer[0] = base64Alphabet[((srcBuffer[0] & 0b11111100) >> 2)                                                             ];
	                    dstBuffer[1] = base64Alphabet[((srcBuffer[0] & 0b00000011) << 4) + (((blockLength > 1 ? srcBuffer[1] : 0) & 0b11110000) >> 4)];
	if(blockLength > 1) dstBuffer[2] = base64Alphabet[((srcBuffer[1] & 0b00001111) << 2) + (((blockLength > 2 ? srcBuffer[2] : 0) & 0b11000000) >> 6)];
	else                dstBuffer[2] = '=';
	if(blockLength > 2) dstBuffer[3] = base64Alphabet[ (srcBuffer[2] & 0b00111111)                                                                   ];
	else                dstBuffer[3] = '=';
}

void Logger::convertBufferToBase64String(const u8 * srcBuffer, u32 srcLength, char * dstBuffer, u16 bufferLength)
{
	if (bufferLength == 0) {
		SIMEXCEPTION(BufferTooSmallException); //LCOV_EXCL_LINE assertion
		return;
	}
	bufferLength--; //Reserve one byte for zero termination
	u32 requiredDstBufferLength = ((srcLength + 2) / 3) * 4;
	if (bufferLength < requiredDstBufferLength) {
		SIMEXCEPTION(BufferTooSmallException);
		srcLength = bufferLength / 4 * 3;
	}

	for (u32 i = 0; i < srcLength; i += 3) {
		u32 srcLengthLeft = srcLength - i;
		convertBase64Block(srcBuffer + i, srcLengthLeft > 3 ? 3 : srcLengthLeft, dstBuffer + i / 3 * 4);
	}

	dstBuffer[((srcLength + 2) / 3) * 4] = 0;
}

void Logger::convertBufferToHexString(const u8 * srcBuffer, u32 srcLength, char * dstBuffer, u16 bufferLength)
{
	CheckedMemset(dstBuffer, 0x00, bufferLength);

	char* dstBufferStart = dstBuffer;
	for (u32 i = 0; i < srcLength; i++)
	{
		//We need to have at least 3 chars to place our .. if the string is too long
		if (dstBuffer - dstBufferStart + 3 < bufferLength) {
			dstBuffer += snprintf(dstBuffer, bufferLength, i < srcLength - 1 ? "%02X:" : "%02X\0", srcBuffer[i]);
		}
		else {
			dstBuffer[-3] = '.';
			dstBuffer[-2] = '.';
			dstBuffer[-1] = '\0';
			break;
		}

	};
}

u32 Logger::parseEncodedStringToBuffer(const char * encodedString, u8 * dstBuffer, u16 dstBufferSize, bool *didError)
{
	auto len = strlen(encodedString);
	if (len >= 4 && encodedString[2] != ':') {
		return parseBase64StringToBuffer(encodedString, len, dstBuffer, dstBufferSize, didError);
	}
	else {
		return parseHexStringToBuffer(encodedString, len, dstBuffer, dstBufferSize, didError);
	}
}

u32 Logger::parseHexStringToBuffer(const char* hexString, u32 hexStringLength, u8* dstBuffer, u16 dstBufferSize, bool *didError)
{
	u32 length = (hexStringLength + 1) / 3;
	if(length > dstBufferSize){
		if(didError != nullptr) *didError = true;
		logt("ERROR", "too long for dstBuffer"); //LCOV_EXCL_LINE assertion
		length = dstBufferSize;					 //LCOV_EXCL_LINE assertion
		SIMEXCEPTION(BufferTooSmallException);	 //LCOV_EXCL_LINE assertion
	}

	for(u32 i = 0; i<length; i++){
		dstBuffer[i] = (u8)strtoul(hexString + (i*3), nullptr, 16);
	}

	return length;
}

u32 parseBase64Block(const char* base64Block, u8 * dstBuffer, u16 dstBufferSize, bool *didError)
{
	const char* base64Ptr[4];
	u32 base64Index[4];
	for (int i = 0; i < 4; i++) 
	{
		base64Ptr[i] = strchr(base64Alphabet, base64Block[i]);
		base64Index[i] = (u32)(base64Ptr[i] - base64Alphabet);
	}

	u32 length = 3;
	//Strictly speaking we accept any none base64 char as padding.
	if (base64Ptr[3] == nullptr)
		length--;
	if (base64Ptr[2] == nullptr)
		length--;
	if (base64Ptr[1] == nullptr || base64Ptr[0] == nullptr)
	{
		if (didError != nullptr) *didError = true;
		SIMEXCEPTION(IllegalArgumentException);	 //LCOV_EXCL_LINE assertion
		return 0;								 //LCOV_EXCL_LINE assertion
	}

	if(               dstBufferSize >= 1) dstBuffer[0] = ((base64Index[0] & 0b111111) << 2) + ((base64Index[1] & 0b110000) >> 4);
	if(length >= 2 && dstBufferSize >= 2) dstBuffer[1] = ((base64Index[1] & 0b001111) << 4) + ((base64Index[2] & 0b111100) >> 2);
	if(length >= 3 && dstBufferSize >= 3) dstBuffer[2] = ((base64Index[2] & 0b000011) << 6) + ((base64Index[3] & 0b111111));

	if (length > dstBufferSize) 
	{
		if (didError != nullptr) *didError = true;
		SIMEXCEPTION(BufferTooSmallException);
	}

	return length;
}

u32 Logger::parseBase64StringToBuffer(const char * base64String, const u32 base64StringLength, u8 * dstBuffer, u16 dstBufferSize, bool *didError)
{
	if (base64StringLength % 4 != 0)
	{
		if (didError != nullptr) *didError = true;
		SIMEXCEPTION(IllegalArgumentException); //The base64 string was not padded correctly.
		return 0;
	}

	const u32 amountOfBlocks = base64StringLength / 4;

	u32 amountOfBytes = 0;
	for (u32 i = 0; i < amountOfBlocks; i++)
	{
		amountOfBytes += parseBase64Block(base64String, dstBuffer, (i32)dstBufferSize - (i32)amountOfBytes, didError);
		base64String += 4;
		dstBuffer += 3;
	}
	return amountOfBytes;
}

void Logger::disableAll()
{
	activeLogTags = {};
	logEverything = false;
}

void Logger::enableAll()
{
	logEverything = true;
}
