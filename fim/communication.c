#include "communication.h"
#include "event_handler.h"
#include "global_context.h"
#include "log.h"
#include "pending_operation_list.h"

#include <ntstrsafe.h>

NTSTATUS connect_notify_callback(
	_In_ PFLT_PORT client_port,
	_In_ PVOID server_port_cookie,
	_In_reads_bytes_(size_of_context) PVOID connection_context,
	_In_ ULONG size_of_context,
	_Outptr_result_maybenull_ PVOID* connection_cookie)
{
	UNREFERENCED_PARAMETER(server_port_cookie);
	UNREFERENCED_PARAMETER(connection_context);
	UNREFERENCED_PARAMETER(size_of_context);
	connection_cookie = NULL;

	FLT_ASSERT(g_context.client_port == NULL);
	g_context.client_port = client_port;
	return STATUS_SUCCESS;
}

VOID disconnect_notify_callback(
	_In_opt_ PVOID connection_cookie)
{
	UNREFERENCED_PARAMETER(connection_cookie);

	if (g_context.client_port != NULL)
	{
		FltCloseClientPort(g_context.registered_filter, &g_context.client_port);
		g_context.client_port = NULL;
	}
	LOG_MSG("disconnect_notify_callback");
}

NTSTATUS create_communication_port()
{
	LOG_MSG("create_communication_port START");

	PSECURITY_DESCRIPTOR security_descriptor = NULL;
	NTSTATUS status = FltBuildDefaultSecurityDescriptor(&security_descriptor, FLT_PORT_ALL_ACCESS);
	if (NT_ERROR(status)) {
		LOG_MSG("FltBuildDefaultSecurityDescriptor failed. status 0x%x\n", status);
		return status;
	}

	UNICODE_STRING portName;
	RtlInitUnicodeString(&portName, COMMUNICATION_PORT_NAME);

	OBJECT_ATTRIBUTES object_attributes;
	InitializeObjectAttributes(&object_attributes,
		&portName,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		security_descriptor);

	status = FltCreateCommunicationPort(g_context.registered_filter,
		&g_context.server_port,
		&object_attributes,
		NULL,
		connect_notify_callback,
		disconnect_notify_callback,
		user_reply_notify_callback, // triggered by FilterSendMessage 
		1);

	FltFreeSecurityDescriptor(security_descriptor);

	if (!NT_SUCCESS(status)) {
		FltUnregisterFilter(g_context.registered_filter);
	}

	LOG_MSG("create_communication_port END");

	return status;
}

NTSTATUS send_message_to_user(_In_ FIM_MESSAGE* message)
{
	LOG_MSG("send_message_to_user START");

	if (g_context.client_port == NULL) {
		LOG_MSG("g_context.client_port == NULL");
		return STATUS_UNSUCCESSFUL;
	}

	if (message == NULL) {
		LOG_MSG("message == NULL");
		return STATUS_UNSUCCESSFUL;
	}

	NTSTATUS status = FltSendMessage(
		g_context.registered_filter,
		&g_context.client_port,
		message,
		sizeof(FIM_MESSAGE),
		NULL,
		NULL,
		NULL
	);
	LOG_MSG("FltSendMessage");

	if (!NT_SUCCESS(status)) {
		return STATUS_UNSUCCESSFUL;
	}

	LOG_MSG("send_message_to_user END");

	return STATUS_SUCCESS;
}

NTSTATUS create_confirmation_message(_In_ PFLT_CALLBACK_DATA data, _In_ ULONG operation_id, _Out_ FIM_MESSAGE* message) {
	LOG_MSG("create_confirmation_message START");
	if (message == NULL) {
		LOG_MSG("message == NULL");
		return STATUS_INVALID_PARAMETER;
	}

	message->message_type = MESSAGE_TYPE_CONFIRMATION;
	message->operation_type = get_operation_type(data);;
	message->confirmation_message.operation_id = operation_id;
	RtlSecureZeroMemory(&(message->log_message), sizeof(LOG_MESSAGE));

	WCHAR buffer[MAX_FILE_NAME_LENGTH];
	UNICODE_STRING file_name = { .Length = 0, .MaximumLength = MAX_FILE_NAME_LENGTH, .Buffer = buffer };
	NTSTATUS status = get_file_name(data, &file_name);
	if (!NT_SUCCESS(status)) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	status = RtlStringCchCopyW(message->file_name, 260, file_name.Buffer);
	if (!NT_SUCCESS(status)) {
		LOG_MSG("RtlStringCchCopyW failed. status 0x%x\n", status);
		return status;
	}

	LOG_MSG("create_confirmation_message END");
	return STATUS_SUCCESS;
}

NTSTATUS create_log_message(_In_ PFLT_CALLBACK_DATA data, _Out_ FIM_MESSAGE* message) {
	LOG_MSG("create_log_message START");
	if (message == NULL) {
		LOG_MSG("message == NULL");
		return STATUS_INVALID_PARAMETER;
	}

	message->message_type = MESSAGE_TYPE_LOG;
	message->operation_type = get_operation_type(data);
	RtlSecureZeroMemory(&(message->confirmation_message), sizeof(CONFIRMATION_MESSAGE));

	KeQuerySystemTime(&message->log_message.completion_time);
	message->log_message.process_id = (ULONG_PTR)PsGetProcessId(IoThreadToProcess(data->Thread));
	message->log_message.thread_id = (ULONG_PTR)PsGetThreadId(data->Thread);

	WCHAR buffer[MAX_FILE_NAME_LENGTH];
	UNICODE_STRING file_name = { .Length = 0, .MaximumLength = MAX_FILE_NAME_LENGTH, .Buffer = buffer };
	NTSTATUS status = get_file_name(data, &file_name);
	if (!NT_SUCCESS(status)) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	status = RtlStringCchCopyW(message->file_name, 260, file_name.Buffer);
	if (!NT_SUCCESS(status)) {
		LOG_MSG("RtlStringCchCopyW failed. status 0x%x\n", status);
		return status;
	}

	LOG_MSG("create_log_message END");
	return STATUS_SUCCESS;
}

// called whenever a user mode application wishes to communicate with the minifilter.
NTSTATUS user_reply_notify_callback(
	_In_opt_ PVOID port_cookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID input_buffer,
	_In_ ULONG input_buffer_length,
	_Out_writes_bytes_to_opt_(output_buffer_length, *return_output_buffer_length) PVOID output_buffer,
	_In_ ULONG output_buffer_length,
	_Out_ PULONG return_output_buffer_length
) {
	LOG_MSG("user_reply_notify_callback START");
	UNREFERENCED_PARAMETER(port_cookie);
	UNREFERENCED_PARAMETER(output_buffer);
	UNREFERENCED_PARAMETER(output_buffer_length);
	*return_output_buffer_length = 0;

	if ((input_buffer == NULL) ||
		(input_buffer_length < (FIELD_OFFSET(USER_REPLY, operation_id) +
			sizeof(USER_REPLY)))) {
		return STATUS_INVALID_PARAMETER;
	}

	USER_REPLY reply;
	try {
		reply.operation_id = ((USER_REPLY*)input_buffer)->operation_id;
		reply.allow = ((USER_REPLY*)input_buffer)->allow;
		LOG_MSG("try");

	} except(exception_handler(GetExceptionInformation(), TRUE)) {

		return GetExceptionCode();
	}

	PENDING_OPERATION* replied_operation = pending_operation_list_remove_by_id(reply.operation_id);
	if (replied_operation == NULL) {
		// TODO replied operation doesnt exist in the pending list
		return STATUS_UNSUCCESSFUL;
	}

	FLT_PREOP_CALLBACK_STATUS status;
	if (reply.allow) {
		status = FLT_PREOP_SUCCESS_WITH_CALLBACK; // call postoperation for logging
		LOG_MSG("ALLOWED");
	}
	else {
		// TODO: add log
		replied_operation->data->IoStatus.Status = STATUS_ACCESS_DENIED;
		status = FLT_PREOP_COMPLETE;
		LOG_MSG("DENIED");
	}

	FltCompletePendedPreOperation(replied_operation->data, status, NULL);
	LOG_MSG("FltCompletePendedPreOperation");

	ExFreePoolWithTag(replied_operation, PENDING_OPERATION_TAG);
	LOG_MSG("user_reply_notify_callback END");

	return STATUS_SUCCESS;
}

LONG exception_handler(
	_In_ PEXCEPTION_POINTERS ExceptionPointer,
	_In_ BOOLEAN AccessingUserBuffer)
{
	NTSTATUS Status;

	Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

	//  Certain exceptions shouldn't be dismissed within the filter
	//  unless we're touching user memory.

	if (!FsRtlIsNtstatusExpected(Status) &&
		!AccessingUserBuffer) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}
