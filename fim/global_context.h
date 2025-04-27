#ifndef FILE_INTEGRITY_MONITORING_FILTER_GLOBAL_CONTEXT_H
#define FILE_INTEGRITY_MONITORING_FILTER_GLOBAL_CONTEXT_H

#include <fltKernel.h>

typedef struct {
	PFLT_FILTER registered_filter;
	PFLT_PORT server_port;
	PFLT_PORT client_port;
	PVOID thread_object;
	BOOLEAN keep_running;
} FIM_CONTEXT;

extern FIM_CONTEXT g_context;

#endif //FILE_INTEGRITY_MONITORING_FILTER_GLOBAL_CONTEXT_H