#ifndef FILE_INTEGRITY_MONITORING_FILE_OPERATION_LIST_H
#define FILE_INTEGRITY_MONITORING_FILE_OPERATION_LIST_H

#include <fltKernel.h>

#define FILE_OPERATION_TAG 'fopt'

typedef struct {
	LIST_ENTRY list_entry;
	PFLT_CALLBACK_DATA data;
} FILE_OPERATION;

extern LIST_ENTRY g_file_operation_list;
extern KSPIN_LOCK g_file_operation_lock;

VOID file_operation_list_initialize();
VOID file_operation_list_append(_In_ FILE_OPERATION* operation);
FILE_OPERATION* file_operation_list_remove();
VOID file_operation_list_clear();
BOOLEAN file_operation_list_is_empty();

NTSTATUS add_operation_to_file_list(_In_ PFLT_CALLBACK_DATA data);
#endif // FILE_INTEGRITY_MONITORING_FILE_OPERATION_LIST_H

