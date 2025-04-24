#ifndef FILE_INTEGRITY_MONITORING_PENDING_OPERATION_LIST_H
#define FILE_INTEGRITY_MONITORING_PENDING_OPERATION_LIST_H

#include <fltKernel.h>

typedef struct {
	LIST_ENTRY list_entry;
	ULONG operation_id;
	PFLT_CALLBACK_DATA data;
} PENDING_OPERATION;

extern FAST_MUTEX g_lock;
extern LIST_ENTRY g_pending_operation_list;
extern LONG g_operation_id;

VOID pending_operation_list_initialize();
VOID pending_operation_list_append(_In_ PENDING_OPERATION* operation);
PENDING_OPERATION* pending_operation_list_remove_by_id(_In_ CONST ULONG operation_id);
VOID pending_operation_list_clear();

NTSTATUS add_operation_to_pending_list(_In_ PFLT_CALLBACK_DATA data, _In_ ULONG operation_id);
#endif // FILE_INTEGRITY_MONITORING_PENDING_OPERATION_LIST_H

