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

VOID pending_operation_list_initiliaze();
VOID pending_operation_list_append(PENDING_OPERATION* operation);
PENDING_OPERATION* pending_operation_list_remove_by_id(CONST ULONG operation_id);
BOOLEAN pending_operation_list_is_empty();
VOID pending_operation_list_remove(PENDING_OPERATION* pending);


#endif // FILE_INTEGRITY_MONITORING_PENDING_OPERATION_LIST_H

