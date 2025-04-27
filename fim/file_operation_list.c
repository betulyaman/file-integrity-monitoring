#include "file_operation_list.h"

#include "log.h"

FAST_MUTEX g_file_operation_list_lock;
LIST_ENTRY g_file_operation_list;

VOID file_operation_list_initialize() {
	LOG_MSG("file_operation_list_initialize START");

	InitializeListHead(&g_file_operation_list);
	ExInitializeFastMutex(&g_file_operation_list_lock);

	LOG_MSG("file_operation_list_initialize END");
}

VOID file_operation_list_append(_In_ FILE_OPERATION* operation)
{
	LOG_MSG("file_operation_list_append START");

	ExAcquireFastMutex(&g_file_operation_list_lock);
	InsertTailList(&g_file_operation_list, &(operation->list_entry));
	ExReleaseFastMutex(&g_file_operation_list_lock);

	LOG_MSG("file_operation_list_append END");
}

FILE_OPERATION* file_operation_list_remove()
{
	LOG_MSG("file_operation_list_remove_by_id START");

	if (file_operation_list_is_empty()) {
		return NULL;
	}

	ExAcquireFastMutex(&g_file_operation_list_lock);
	PLIST_ENTRY removed_entry = RemoveHeadList(&g_file_operation_list);
	FILE_OPERATION* file_operation = CONTAINING_RECORD(removed_entry, FILE_OPERATION, list_entry);
	ExReleaseFastMutex(&g_file_operation_list_lock);

	LOG_MSG("file_operation_list_remove_by_id END");

	return file_operation;
}

NTSTATUS add_operation_to_file_list(_In_ PFLT_CALLBACK_DATA data)
{
	LOG_MSG("add_operation_to_file_list START");

	// Set up file op context, store calllback data and wait event
	FILE_OPERATION* file = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILE_OPERATION), FILE_OPERATION_TAG);
	if (!file) {
		LOG_MSG("ExAllocatePoolWithTag Pending: STATUS_INSUFFICIENT_RESOURCES");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(file, sizeof(FILE_OPERATION));
	file->data = data;
	file_operation_list_append(file);

	LOG_MSG("add_operation_to_file_list END");

	return STATUS_SUCCESS;

}

VOID file_operation_list_clear() {
	LOG_MSG("file_operation_list_clear START");

	ExAcquireFastMutex(&g_file_operation_list_lock);
	PLIST_ENTRY head = RemoveHeadList(&g_file_operation_list);
	while (head != &g_file_operation_list) {
		ExFreePoolWithTag(head, FILE_OPERATION_TAG);
		head = RemoveHeadList(&g_file_operation_list);
	}
	ExReleaseFastMutex(&g_file_operation_list_lock);
	LOG_MSG("file_operation_list_clear END");

}

BOOLEAN file_operation_list_is_empty() {
	ExAcquireFastMutex(&g_file_operation_list_lock);
	BOOLEAN is_empty = IsListEmpty(&g_file_operation_list);
	ExReleaseFastMutex(&g_file_operation_list_lock);

	return is_empty;
}


// TODO deinitialize entry_list, lock