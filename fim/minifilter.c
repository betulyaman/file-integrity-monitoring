#include "minifilter.h"

#include <wdm.h>

FIM_CONTEXT g_context;

NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT driver_object,
	_In_ PUNICODE_STRING registry_path)
{
	UNREFERENCED_PARAMETER(registry_path);

	DbgPrint("FIM: DriverEntry START\n");
	pending_operation_list_initialize();

	NTSTATUS status = register_filter(driver_object);
	if (!NT_SUCCESS(status)) {
		DbgPrint("FIM: FltRegisterFilter failed. status 0x%x\n", status);
		return status;
	}

	status = create_communication_port();
	if (!NT_SUCCESS(status)) {
		DbgPrint("FIM: create_communication_port failed, status: 0x%x\n", status);
		return status;
	}

	status = FltStartFiltering(g_context.registered_filter);
	if (!NT_SUCCESS(status)) {
		DbgPrint("FIM: FltStartFiltering failed, status: 0x%x\n", status);
		FltUnregisterFilter(g_context.registered_filter);
		return status;
	}
	DbgPrint("FIM: FltStartFiltering\n");

	DbgPrint("FIM: DriverEntry END\n");
	return status;
}

NTSTATUS filter_unload_callback(FLT_FILTER_UNLOAD_FLAGS flags)
{
	UNREFERENCED_PARAMETER(flags);
	DbgPrint("FIM: filter_unload_callback START\n");

	if (g_context.server_port != NULL) {
		FltCloseCommunicationPort(g_context.server_port);
		g_context.server_port = NULL;
	}

	if (g_context.client_port != NULL) {
		FltCloseClientPort(g_context.registered_filter, &g_context.client_port);
		g_context.client_port = NULL;
	}

	if (g_context.registered_filter != NULL) {
		FltUnregisterFilter(g_context.registered_filter);
		g_context.registered_filter = NULL;
	}

	pending_operation_list_clear();

	DbgPrint("FIM: filter_unload_callback END\n");

	return STATUS_SUCCESS;
}

BOOLEAN is_agent_connected() {
	return (g_context.client_port != NULL);
}