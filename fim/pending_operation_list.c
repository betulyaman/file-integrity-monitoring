#include "pending_operation_list.h"
#include "minifilter.h"


FAST_MUTEX g_lock;
LIST_ENTRY g_pending_operation_list;
LONG g_operation_id;

VOID pending_operation_list_initiliaze() {
	InitializeListHead(&g_pending_operation_list);
	ExInitializeFastMutex(&g_lock);
}

VOID pending_operation_list_append(_In_ PENDING_OPERATION* operation)
{
	ExAcquireFastMutex(&g_lock);
	InsertTailList(&g_pending_operation_list, &(operation->list_entry));
	ExReleaseFastMutex(&g_lock);
}

PENDING_OPERATION* pending_operation_list_remove_by_id(_In_ CONST ULONG operation_id)
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

NTSTATUS add_operation_to_pending_list(_In_ PFLT_CALLBACK_DATA data, _In_ ULONG operation_id)
{
	// Set up pending op context, store calllback data and wait event
	PENDING_OPERATION* pending = ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDING_OPERATION), TAG);
	if (!pending) {
		DbgPrint("ExAllocatePoolWithTag Pending: STATUS_INSUFFICIENT_RESOURCES\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(pending, sizeof(PENDING_OPERATION));
	pending->operation_id = operation_id;
	pending->data = data;
	pending_operation_list_append(pending);

	return STATUS_SUCCESS;

}

VOID pending_operation_list_clear() {
	ExAcquireFastMutex(&g_lock);
	PLIST_ENTRY head = RemoveHeadList(&g_pending_operation_list);
	while (head != &g_pending_operation_list) {
		ExFreePool(head);
		head = RemoveHeadList(&g_pending_operation_list);
	}
	ExReleaseFastMutex(&g_lock);
}

// TODO deinitialize entry_list, lock