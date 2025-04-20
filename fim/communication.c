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
}

NTSTATUS create_communication_port()
{
    PSECURITY_DESCRIPTOR security_descriptor = NULL;
    NTSTATUS status = FltBuildDefaultSecurityDescriptor(&security_descriptor, FLT_PORT_ALL_ACCESS);
    if (NT_ERROR(status)) {
        DbgPrint("FltBuildDefaultSecurityDescriptor failed. status 0x%x\n", status);
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
        user_reply_notify_callback,
        1);

    if (!NT_SUCCESS(status)) {
        FltUnregisterFilter(g_context.registered_filter);
    }

    return status;
}

NTSTATUS send_message_to_user(
    _In_ PFLT_CALLBACK_DATA data, 
    _In_ PCUNICODE_STRING file_name) 
{
    
    if (g_context.client_port == NULL) {
        DbgPrint("Client Port is NULL\n");
        return STATUS_UNSUCCESSFUL;
    }

    FIM_MESSAGE* message = ExAllocatePoolWithTag(NonPagedPool, sizeof(FIM_MESSAGE), TAG);
    if (message == NULL) {
        DbgPrint("ExAllocatePoolWithTag: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LONG operation_id = InterlockedIncrement(&g_operation_id);
    message->operation_id = operation_id;
    NTSTATUS status = RtlStringCchCopyW(message->file_name, 260, file_name->Buffer);
    if (status) {
        DbgPrint("RtlStringCchCopyW failed. status 0x%x\n", status);
    }

    // Send to user
    status = FltSendMessage(
        g_context.registered_filter,
        &g_context.client_port,
        message, 
        sizeof(FIM_MESSAGE),
        NULL, 
        NULL,
        NULL
    );
    if (!NT_SUCCESS(status)) {
        ExFreePool(message);
        DbgPrint("FltSendMessage failed. status 0x%x\n", status);
        return status;
    }

    ExFreePool(message);

    // Set up pending op context, store calllback data and wait event
    PENDING_OPERATION* pending = ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDING_OPERATION), TAG);
    if (!pending) {
        DbgPrint("ExAllocatePoolWithTag Pending: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pending, sizeof(PENDING_OPERATION));
    pending->operation_id = operation_id;
    pending->data = data;
    pending_operation_list_append(pending);

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

    } except( exception_handler(GetExceptionInformation(), TRUE)) {

        return GetExceptionCode();
    }

    PENDING_OPERATION* replied_operation = pending_operation_list_remove_by_id(reply.operation_id);
    if (replied_operation == NULL) {
        // TODO replied operation doesnt exist in the pending list
        return STATUS_UNSUCCESSFUL;
    }

    FLT_PREOP_CALLBACK_STATUS status;
    if (reply.allow) {
        status = FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
    else {
        replied_operation->data->IoStatus.Status = STATUS_ACCESS_DENIED;
        status = FLT_PREOP_COMPLETE;
    }

    FltCompletePendedPreOperation(replied_operation->data, status, NULL);

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
