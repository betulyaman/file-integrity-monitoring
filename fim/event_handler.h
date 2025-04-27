#ifndef FILE_INTEGRITY_MONITORING_EVENT_LISTENER_H
#define FILE_INTEGRITY_MONITORING_EVENT_LISTENER_H

#include "kernel_communication_info.h"

#include <fltKernel.h>

FLT_PREOP_CALLBACK_STATUS pre_operation_callback(
	_Inout_ PFLT_CALLBACK_DATA data,
	_In_ PCFLT_RELATED_OBJECTS filter_objects,
	_Flt_CompletionContext_Outptr_ PVOID* completion_context
);

FLT_POSTOP_CALLBACK_STATUS post_operation_callback(
	_Inout_ PFLT_CALLBACK_DATA data,
	_In_ PCFLT_RELATED_OBJECTS filter_objects,
	_In_opt_ PVOID completion_context,
	_In_ FLT_POST_OPERATION_FLAGS flags
);

OPERATION_TYPE get_operation_type(_Inout_ PFLT_CALLBACK_DATA data);
NTSTATUS get_file_name(_Inout_ PFLT_CALLBACK_DATA data, _Out_ PUNICODE_STRING file_name);

#endif // FILE_INTEGRITY_MONITORING_EVENT_LISTENER_H
