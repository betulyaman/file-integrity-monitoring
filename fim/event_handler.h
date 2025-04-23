#ifndef FILE_INTEGRITY_MONITORING_EVENT_LISTENER_H
#define FILE_INTEGRITY_MONITORING_EVENT_LISTENER_H

#include <fltKernel.h>

// 0x00	IRP_MJ_CREATE
// 0x01	IRP_MJ_CREATE_NAMED_PIPE
// 0x02	IRP_MJ_CLOSE
// 0x03	IRP_MJ_READ
// 0x04	IRP_MJ_WRITE
// 0x05	IRP_MJ_QUERY_INFORMATION
// 0x06	IRP_MJ_SET_INFORMATION

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
