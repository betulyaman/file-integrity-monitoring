#ifndef FILE_INTEGRITY_MONITORING_COMMUNICATION_INFO_H
#define FILE_INTEGRITY_MONITORING_COMMUNICATION_INFO_H

#define COMMUNICATION_PORT_NAME L"\\CommunicationPort"

typedef struct {
	ULONG operation_id;
	//UINT8 operation_type;
	WCHAR file_name[260];
} FIM_MESSAGE;

typedef struct {
	ULONG operation_id;
	BOOLEAN allow;
} USER_REPLY;

#endif // FILE_INTEGRITY_MONITORING_COMMUNICATION_INFO_H