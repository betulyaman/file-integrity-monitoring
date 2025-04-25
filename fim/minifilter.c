#include "minifilter.h"

#include <wdm.h>

FIM_CONTEXT g_context;

NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT driver_object,
	_In_ PUNICODE_STRING registry_path)
{
	UNREFERENCED_PARAMETER(registry_path);

	LOG_MSG("DriverEntry START");

	NTSTATUS status = STATUS_SUCCESS;

	try {

		pending_operation_list_initialize();

		status = register_filter(driver_object);
		if (!NT_SUCCESS(status)) {
			LOG_MSG("FltRegisterFilter failed. status 0x%x", status);
			return status;
		}

		status = create_communication_port();
		if (!NT_SUCCESS(status)) {
			LOG_MSG("create_communication_port failed, status: 0x%x", status);
			leave;
		}

		status = FltStartFiltering(g_context.registered_filter);
		if (!NT_SUCCESS(status)) {
			LOG_MSG("FltStartFiltering failed, status: 0x%x", status);
			leave;
		}
	}
	finally {

		if (!NT_SUCCESS(status)) {

			if (NULL != g_context.server_port) {
				FltCloseCommunicationPort(g_context.server_port);
			}

			if (NULL != g_context.registered_filter) {
				FltUnregisterFilter(g_context.registered_filter);
			}
		}
	}
	LOG_MSG("DriverEntry END");
	return status;
}

NTSTATUS filter_unload_callback(FLT_FILTER_UNLOAD_FLAGS flags)
{
	UNREFERENCED_PARAMETER(flags);
	LOG_MSG("filter_unload_callback START");

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

	LOG_MSG("filter_unload_callback END");
	return STATUS_SUCCESS;
}

// allows our filter to be manually detached from a volume.
NTSTATUS filter_tear_down_callback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	PAGED_CODE();
	return STATUS_SUCCESS;
}

BOOLEAN is_agent_connected() {
	return (g_context.client_port != NULL);
}