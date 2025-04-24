#ifndef FILE_INTEGRITY_MONITORING_FILTER_H
#define FILE_INTEGRITY_MONITORING_FILTER_H

#include "communication.h"
#include "event_handler.h"
#include "pending_operation_list.h"

#include <fltKernel.h>

#define TAG 'fltr'

typedef struct {
	PFLT_FILTER registered_filter;
	PFLT_PORT server_port;
	PFLT_PORT client_port;
} FIM_CONTEXT;

extern FIM_CONTEXT g_context;

NTSTATUS register_filter(_In_ PDRIVER_OBJECT driver_object);

NTSTATUS filter_unload_callback(FLT_FILTER_UNLOAD_FLAGS flags);


BOOLEAN is_agent_connected();

#endif // FILE_INTEGRITY_MONITORING_FILTER_H