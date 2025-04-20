//#include "minifilter.h"
//
//FAST_MUTEX g_lock;
//LIST_ENTRY g_pending_ops_list;
//
//// Check for queued ops
//// Wait on the event for that op
//// Once event is set by handle_user_reply, decide to allow/cancel
//VOID listen_for_reply(PVOID start_context) {
//
//	UNREFERENCED_PARAMETER(start_context);
//
//	// Wait for user's reply
//	while (stop_thread) {
//		ExAcquireFastMutex(&g_lock);
//		if (IsListEmpty(&g_pending_ops_list)) {
//			ExReleaseFastMutex(&g_lock);
//			// Suspends the calling thread.
//			KeDelayExecutionThread(KernelMode, FALSE, &((LARGE_INTEGER) { .QuadPart = -10 * 1000 * 1000 })); // nanoseconds (relative times)
//			continue;
//		}
//		//TODO waker, listener
//		PLIST_ENTRY entry = RemoveHeadList(&g_pending_ops_list);
//		ExReleaseFastMutex(&g_lock);
//
//		PPENDING_OPERATION operation = CONTAINING_RECORD(entry, PENDING_OPERATION, list_entry);
//		// the routine waits indefinitely until the dispatcher object is set to the signaled state.
//		// Puts the current thread to sleep until the event is signaled
//		KeWaitForSingleObject(&operation->reply_event, Executive, KernelMode, FALSE, NULL);
//
//
//
//		FltCompletePendedPreOperation(
//			operation->data,
//			operation->allow ? FLT_PREOP_SUCCESS_NO_CALLBACK : FLT_PREOP_COMPLETE,
//			NULL
//		);
//
//		ExFreePool(operation);
//	}
//}