#ifndef FILE_INTEGRITY_MONITORING_COMMUNICATION_H
#define FILE_INTEGRITY_MONITORING_COMMUNICATION_H

#include <fltKernel.h>

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

NTSTATUS connect_notify_callback(
	_In_ PFLT_PORT client_port,
	_In_ PVOID server_port_cookie,
	_In_reads_bytes_(size_of_context) PVOID conneciton_context,
	_In_ ULONG size_of_context,
	_Outptr_result_maybenull_ PVOID* connection_cookie);

VOID disconnect_notify_callback(
	_In_opt_ PVOID connection_cookie);

NTSTATUS user_reply_notify_callback(
	_In_opt_ PVOID port_cookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID input_buffer,
	_In_ ULONG input_buffer_length,
	_Out_writes_bytes_to_opt_(output_buffer_length, *return_output_buffer_length) PVOID output_buffer,
	_In_ ULONG output_buffer_length,
	_Out_ PULONG return_output_buffer_length
);

NTSTATUS create_communication_port();

NTSTATUS send_message_to_user(
	_In_ PFLT_CALLBACK_DATA data,
	_In_ PCUNICODE_STRING file_name);

LONG exception_handler(
	_In_ PEXCEPTION_POINTERS ExceptionPointer,
	_In_ BOOLEAN AccessingUserBuffer);

#endif //FILE_INTEGRITY_MONITORING_COMMUNICATION_H