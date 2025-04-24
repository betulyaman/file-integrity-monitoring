#include "communication.h"
#include "minifilter.h"

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
	g_context.client_port = client_port;
	DbgPrint("FIM: connect_notify_callback\n");
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
	DbgPrint("FIM: disconnect_notify_callback\n");
}

NTSTATUS create_communication_port()
{
	DbgPrint("FIM: create_communication_port START\n");

	PSECURITY_DESCRIPTOR security_descriptor = NULL;
	NTSTATUS status = FltBuildDefaultSecurityDescriptor(&security_descriptor, FLT_PORT_ALL_ACCESS);
	if (NT_ERROR(status)) {
		DbgPrint("FIM: FltBuildDefaultSecurityDescriptor failed. status 0x%x\n", status);
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

	if (!NT_SUCCESS(status)) {
		FltUnregisterFilter(g_context.registered_filter);
	}

	DbgPrint("FIM: create_communication_port END\n");

	return status;
}

NTSTATUS send_message_to_user(_In_ FIM_MESSAGE* message)
{
	DbgPrint("FIM: pre_operation_callback START\n");

	if (g_context.client_port == NULL) {
		DbgPrint("FIM: g_context.client_port == NULL\n");
		return STATUS_UNSUCCESSFUL;
	}

	if (message == NULL) {
		DbgPrint("FIM: message == NULL\n");
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
	DbgPrint("FIM: FltSendMessage\n");

	if (!NT_SUCCESS(status)) {
		return STATUS_UNSUCCESSFUL;
	}

	DbgPrint("FIM: pre_operation_callback END\n");

	return STATUS_SUCCESS;
}

NTSTATUS create_message(_Out_ FIM_MESSAGE** message, _In_ PCUNICODE_STRING file_name, _In_ OPERATION_TYPE operation_type, _In_ ULONG operation_id) {
	DbgPrint("FIM: create_message START\n");
	if (message == NULL) {
		DbgPrint("FIM: message == NULL\n");
		*message = NULL;
		return STATUS_INVALID_PARAMETER;
	}

	FIM_MESSAGE* message_new = ExAllocatePoolWithTag(NonPagedPool, sizeof(FIM_MESSAGE), TAG);
	if (message_new == NULL) {
		DbgPrint("FIM: ExAllocatePoolWithTag: STATUS_INSUFFICIENT_RESOURCES\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	DbgPrint("FIM: Allocate message");
	message_new->operation_id = operation_id;
	message_new->operation_type = operation_type;
	NTSTATUS status = RtlStringCchCopyW(message_new->file_name, 260, file_name->Buffer);
	if (status) {
		DbgPrint("FIM: RtlStringCchCopyW failed. status 0x%x\n", status);
		*message = NULL;

		return status;
	}

	*message = message_new;
	DbgPrint("FIM: create_message END\n");
	return STATUS_SUCCESS;
}

NTSTATUS user_reply_notify_callback(
	_In_opt_ PVOID port_cookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID input_buffer,
	_In_ ULONG input_buffer_length,
	_Out_writes_bytes_to_opt_(output_buffer_length, *return_output_buffer_length) PVOID output_buffer,
	_In_ ULONG output_buffer_length,
	_Out_ PULONG return_output_buffer_length
) {
	DbgPrint("FIM: user_reply_notify_callback START\n");
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
		DbgPrint("FIM: try\n");

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
		DbgPrint("FIM: ALLOWED\n");
	}
	else {
		// TODO: add log
		replied_operation->data->IoStatus.Status = STATUS_ACCESS_DENIED;
		status = FLT_PREOP_COMPLETE;
		DbgPrint("FIM: DENIED\n");
	}

	FltCompletePendedPreOperation(replied_operation->data, status, NULL);
	DbgPrint("FIM: FltCompletePendedPreOperation\n");

	DbgPrint("FIM: user_reply_notify_callback END\n");

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
