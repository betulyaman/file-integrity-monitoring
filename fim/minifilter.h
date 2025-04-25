#ifndef FILE_INTEGRITY_MONITORING_FILTER_H
#define FILE_INTEGRITY_MONITORING_FILTER_H

#include "communication.h"
#include "event_handler.h"
#include "pending_operation_list.h"

#include <fltKernel.h>

#if 1
#define LOG_MSG(format, ...) DbgPrint("FIM : " format "\n\r" __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG(format, ...) do{}while(false)
#endif

#define TAG 'fltr'

typedef struct {
	PFLT_FILTER registered_filter;
	PFLT_PORT server_port;
	PFLT_PORT client_port;
} FIM_CONTEXT;

extern FIM_CONTEXT g_context;

NTSTATUS register_filter(_In_ PDRIVER_OBJECT driver_object);

NTSTATUS filter_unload_callback(FLT_FILTER_UNLOAD_FLAGS flags);

NTSTATUS filter_tear_down_callback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

BOOLEAN is_agent_connected();

#endif // FILE_INTEGRITY_MONITORING_FILTER_H