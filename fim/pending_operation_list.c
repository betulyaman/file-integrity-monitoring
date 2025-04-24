#include "pending_operation_list.h"
#include "minifilter.h"


FAST_MUTEX g_lock;
LIST_ENTRY g_pending_operation_list;
LONG g_operation_id;

VOID pending_operation_list_initialize() {
	DbgPrint("FIM: pending_operation_list_initialize START\n");

	InitializeListHead(&g_pending_operation_list);
	ExInitializeFastMutex(&g_lock);
	DbgPrint("FIM: pending_operation_list_initialize END\n");

}

VOID pending_operation_list_append(_In_ PENDING_OPERATION* operation)
{
	DbgPrint("FIM: pending_operation_list_append START\n");

	ExAcquireFastMutex(&g_lock);
	InsertTailList(&g_pending_operation_list, &(operation->list_entry));
	ExReleaseFastMutex(&g_lock);
	DbgPrint("FIM: pending_operation_list_append END\n");

}

PENDING_OPERATION* pending_operation_list_remove_by_id(_In_ CONST ULONG operation_id)
{
	DbgPrint("FIM: pending_operation_list_remove_by_id START\n");

	ExAcquireFastMutex(&g_lock);
	PLIST_ENTRY entry = g_pending_operation_list.Flink;
	while (entry != &g_pending_operation_list) {
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

	DbgPrint("FIM: pending_operation_list_remove_by_id END\n");

	return NULL;
}

NTSTATUS add_operation_to_pending_list(_In_ PFLT_CALLBACK_DATA data, _In_ ULONG operation_id)
{
	DbgPrint("FIM: add_operation_to_pending_list START\n");

	// Set up pending op context, store calllback data and wait event
	PENDING_OPERATION* pending = ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDING_OPERATION), TAG);
	if (!pending) {
		DbgPrint("FIM: ExAllocatePoolWithTag Pending: STATUS_INSUFFICIENT_RESOURCES\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(pending, sizeof(PENDING_OPERATION));
	pending->operation_id = operation_id;
	pending->data = data;
	pending_operation_list_append(pending);

	DbgPrint("FIM: add_operation_to_pending_list END\n");

	return STATUS_SUCCESS;

}

VOID pending_operation_list_clear() {
	DbgPrint("FIM: pending_operation_list_clear START\n");

	ExAcquireFastMutex(&g_lock);
	PLIST_ENTRY head = RemoveHeadList(&g_pending_operation_list);
	while (head != &g_pending_operation_list) {
		ExFreePool(head);
		head = RemoveHeadList(&g_pending_operation_list);
	}
	ExReleaseFastMutex(&g_lock);
	DbgPrint("FIM: pending_operation_list_clear END\n");

}

// TODO deinitialize entry_list, lock