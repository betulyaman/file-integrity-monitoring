#include <Windows.h>
#include <fltUser.h>
#include <stdio.h>

#include "communication_info.h"

typedef struct {
	FILTER_MESSAGE_HEADER header;
	FIM_MESSAGE kernel_message;
} GET_MESSAGE;

int main() {
	printf("Agent\n");

	HANDLE port = INVALID_HANDLE_VALUE;
	HRESULT result = FilterConnectCommunicationPort(COMMUNICATION_PORT_NAME, 0, NULL, 0, NULL, &port);
	if (FAILED(result)) {
		printf("Connect error: 0x%08x\n", result);
		return 1;
	}
	
	GET_MESSAGE message;
	HRESULT hresult;
	while (TRUE) {
		printf("START\n");
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
		printf("ID: %u  -  Message: %ls -  Opeation: %d\n", message.kernel_message.operation_id, message.kernel_message.file_name, message.kernel_message.operation_type);

		LONG operation_id = message.kernel_message.operation_id;
		BOOLEAN allow = ((operation_id % 2) == 0);
		USER_REPLY reply = { .operation_id = operation_id, .allow = allow};
		DWORD bytes_returned;
		hresult = FilterSendMessage(port, &reply, sizeof(USER_REPLY), NULL, NULL, &bytes_returned);
		printf("ID: %d  -  Allow: %u\n", message.kernel_message.operation_id, allow);
		if (hresult != S_OK) {
			printf("FilterSendMessage FAILED hresult: %d\n", hresult);
			continue;
		}

		printf("END\n\n");
	}

}