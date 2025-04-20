#ifndef FILE_INTEGRITY_MONITORING_EVENT_LISTENER_H
#define FILE_INTEGRITY_MONITORING_EVENT_LISTENER_H

#include <fltKernel.h>

FLT_PREOP_CALLBACK_STATUS preoperation_callback_for_delete(
    _Inout_ PFLT_CALLBACK_DATA data,
    _In_ PCFLT_RELATED_OBJECTS filter_objects,
    _Flt_CompletionContext_Outptr_ PVOID* completion_callback
);

//FLT_PREOP_CALLBACK_STATUS preoperation_callback_for_create(
//    _Inout_ PFLT_CALLBACK_DATA data,
//    _In_ PCFLT_RELATED_OBJECTS filter_objects,
//    _Flt_CompletionContext_Outptr_ PVOID* completion_callback
//);
//
//FLT_PREOP_CALLBACK_STATUS preoperation_callback_for_write(
//    _Inout_ PFLT_CALLBACK_DATA data,
//    _In_ PCFLT_RELATED_OBJECTS filter_objects,
//    _Flt_CompletionContext_Outptr_ PVOID* completion_callback
//);
//
//FLT_PREOP_CALLBACK_STATUS preoperation_callback_for_rename(
//    _Inout_ PFLT_CALLBACK_DATA data,
//    _In_ PCFLT_RELATED_OBJECTS filter_objects,
//    _Flt_CompletionContext_Outptr_ PVOID* completion_callback
//);
//
//FLT_PREOP_CALLBACK_STATUS preoperation_callback_for_move(
//    _Inout_ PFLT_CALLBACK_DATA data,
//    _In_ PCFLT_RELATED_OBJECTS filter_objects,
//    _Flt_CompletionContext_Outptr_ PVOID* completion_callback
//);

#endif // FILE_INTEGRITY_MONITORING_EVENT_LISTENER_H
