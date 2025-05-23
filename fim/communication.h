#ifndef FILE_INTEGRITY_MONITORING_COMMUNICATION_H
#define FILE_INTEGRITY_MONITORING_COMMUNICATION_H

#include "kernel_communication_info.h"

#include <fltKernel.h>

NTSTATUS connect_notify_callback(
	_In_ PFLT_PORT client_port,
	_In_ PVOID server_port_cookie,
	_In_reads_bytes_(size_of_context) PVOID conneciton_context,
	_In_ ULONG size_of_context,
	_Outptr_result_maybenull_ PVOID* connection_cookie);

VOID disconnect_notify_callback(_In_opt_ PVOID connection_cookie);

NTSTATUS user_reply_notify_callback(
	_In_ PVOID port_cookie,
	_In_reads_bytes_opt_(input_buffer_length) PVOID input_buffer,
	_In_ ULONG input_buffer_length,
	_Out_writes_bytes_to_opt_(output_buffer_length, *return_output_buffer_length) PVOID output_buffer,
	_In_ ULONG output_buffer_length,
	_Out_ PULONG return_output_buffer_length
);

NTSTATUS create_communication_port();

NTSTATUS create_confirmation_message(
	_In_ PFLT_CALLBACK_DATA data,
	_In_ ULONG operation_id,
	_Out_ FIM_MESSAGE* message);


NTSTATUS create_log_message(
	_In_ PFLT_CALLBACK_DATA data, 
	_Out_ FIM_MESSAGE* message);

NTSTATUS send_message_to_user(_In_ FIM_MESSAGE* message);

LONG exception_handler(
	_In_ PEXCEPTION_POINTERS ExceptionPointer,
	_In_ BOOLEAN AccessingUserBuffer);

#endif //FILE_INTEGRITY_MONITORING_COMMUNICATION_H