#include "pending_operation_list.h"

FAST_MUTEX g_lock;
LIST_ENTRY g_pending_operation_list;
LONG g_operation_id;

VOID pending_operation_list_initiliaze() {
    InitializeListHead(&g_pending_operation_list);
    ExInitializeFastMutex(&g_lock);
}

VOID pending_operation_list_append(PENDING_OPERATION* operation)
{
    ExAcquireFastMutex(&g_lock);
    InsertTailList(&g_pending_operation_list, &(operation->list_entry));
    ExReleaseFastMutex(&g_lock);
}

VOID pending_operation_list_remove(PENDING_OPERATION* pending)
{
    ExAcquireFastMutex(&g_lock);
    RemoveEntryList(&pending->list_entry);
    ExReleaseFastMutex(&g_lock);
}

PENDING_OPERATION* pending_operation_list_remove_by_id(CONST ULONG operation_id)
{
    ExAcquireFastMutex(&g_lock);
    PLIST_ENTRY entry = &g_pending_operation_list;
    while (entry != NULL) {
        // CONTAINING_RECORD: get a pointer to a parent structure when you have a pointer to one of its fields
        PENDING_OPERATION* pending_operation = CONTAINING_RECORD(entry, PENDING_OPERATION, list_entry);
        if (pending_operation->operation_id == operation_id) {
            RemoveEntryList(entry);
            ExReleaseFastMutex(&g_lock);
            return pending_operation;
        }

        entry = entry->Flink;
    }
    ExReleaseFastMutex(&g_lock);

    return NULL;
}

BOOLEAN pending_operation_list_is_empty()
{
	ExAcquireFastMutex(&g_lock);
    BOOLEAN is_empty = (IsListEmpty(&g_pending_operation_list)) ? TRUE : FALSE;
	ExReleaseFastMutex(&g_lock);
    return is_empty;
}

// TODO deinitialize entry_list, lock