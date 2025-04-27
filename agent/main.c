#include <Windows.h>
#include <fltUser.h>
#include <stdio.h>

#include "communication_info.h"

#define TIME_BUFFER_LENGTH 20
#define TIME_ERROR "time error"

typedef struct {
	FILTER_MESSAGE_HEADER header;
	FIM_MESSAGE body;
} GET_MESSAGE;

ULONG
FormatSystemTime(
	_In_ SYSTEMTIME* SystemTime,
	_Out_writes_bytes_(BufferLength) CHAR* Buffer,
	_In_ ULONG BufferLength
);

int main() {
	printf("Agent");

	HANDLE port = INVALID_HANDLE_VALUE;
	HRESULT result;
	boolean is_connected = 0;
	while (!is_connected) {
		
		result = FilterConnectCommunicationPort(COMMUNICATION_PORT_NAME, 0, NULL, 0, NULL, &port);
		if (FAILED(result)) {
			printf("Connect error: 0x%08x\n", result);
			continue;
		}
		else {
			is_connected = 1;
		}
		Sleep(1000);
	}
	
	GET_MESSAGE message;
	HRESULT hresult;
	while (TRUE) {
		printf("START");
		hresult = FilterGetMessage(
			port,
			&(message.header),
			sizeof(GET_MESSAGE),
			NULL
		);

		if (hresult != S_OK) {
			printf("FilterGetMessage FAILED hresult: %d\n", hresult);
			continue;
		}

		if (message.body.message_type == MESSAGE_TYPE_CONFIRMATION) {
			printf("ID       : %u \nOperation: %d \nFile Name: %ls\n\r",
				message.body.confirmation_message.operation_id,
				message.body.operation_type,
				message.body.file_name);

			LONG operation_id = message.body.confirmation_message.operation_id;
			BOOLEAN allow = ((operation_id % 2) == 0);
			USER_REPLY reply = { .operation_id = operation_id, .allow = allow };
			
			DWORD bytes_returned;
			hresult = FilterSendMessage(port, &reply, sizeof(USER_REPLY), NULL, 0, &bytes_returned);
			
			printf("ID    : %u \nAllow : %u\n\r", message.body.confirmation_message.operation_id, allow);
			if (hresult != S_OK) {
				printf("FilterSendMessage FAILED hresult: %d\n", hresult);
				continue;
			}
		}
		else if(message.body.message_type == MESSAGE_TYPE_LOG) {

			// Convert completion time
			FILETIME localTime;
			SYSTEMTIME systemTime;
			CHAR time[TIME_BUFFER_LENGTH];
			FileTimeToLocalFileTime((FILETIME*)&(message.body.log_message.completion_time),&localTime);
			FileTimeToSystemTime(&localTime, &systemTime);
			if (FormatSystemTime(&systemTime, time, TIME_BUFFER_LENGTH)) {
				printf("%-12s\n\r", time);
			}
			else {
				printf("%-12s ", TIME_ERROR);
			}

			printf("Process ID: %8Ix \nThread ID : %-4Ix \n\r", 
				message.body.log_message.process_id, 
				message.body.log_message.thread_id);
			
			printf("ID       : %u \nFile Name: %ls\n\r",
				message.body.operation_type,
				message.body.file_name);
		}
		else {
			printf("INVALID MESSAGE TYPE\n\r");
		}


		printf("END\n");
	}

}

ULONG
FormatSystemTime(
	_In_ SYSTEMTIME* SystemTime,
	_Out_writes_bytes_(BufferLength) CHAR* Buffer,
	_In_ ULONG BufferLength
)
{
	ULONG returnLength = 0;

	if (BufferLength < 20) {
		return 0;
	}

	returnLength = sprintf_s(Buffer,
		BufferLength,
		"%02d:%02d:%02d:%03d",
		SystemTime->wHour,
		SystemTime->wMinute,
		SystemTime->wSecond,
		SystemTime->wMilliseconds);

	return returnLength;
}
