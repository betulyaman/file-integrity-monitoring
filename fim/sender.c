#include "sender.h"

#include "communication.h"
#include "event_handler.h"
#include "file_operation_list.h"
#include "global_context.h"
#include "log.h"
#include "pending_operation_list.h"

VOID complete_failed_file_operation(FILE_OPERATION* file_operation);

LONG g_operation_id;

void run_sender(_In_ PVOID start_context) {
	UNREFERENCED_PARAMETER(start_context);

	while (g_context.keep_running) {
		FILE_OPERATION* file_operation = file_operation_list_remove();
		if (file_operation == NULL) {
			KeDelayExecutionThread(KernelMode, FALSE, &((LARGE_INTEGER) { .QuadPart = -10 * 1000 })); // 10 microseconds (relatives)
			continue;
		}

		LONG operation_id = g_operation_id;
		g_operation_id += 1;

		FIM_MESSAGE message;
		NTSTATUS status = create_confirmation_message(file_operation->data, operation_id, &message);
		if (!NT_SUCCESS(status)) {
			complete_failed_file_operation(file_operation);
		}

		status = send_message_to_user(&message);
		if (!NT_SUCCESS(status)) {
			complete_failed_file_operation(file_operation);
		}

		status = add_operation_to_pending_list(file_operation->data, operation_id);
		if (!NT_SUCCESS(status)) {
			complete_failed_file_operation(file_operation);
		}

		ExFreePoolWithTag(file_operation, FILE_OPERATION_TAG);

	}
}


VOID complete_failed_file_operation(FILE_OPERATION* file_operation) {
	LOG_MSG("complete_operation START");

	file_operation->data->IoStatus.Status = STATUS_ACCESS_DENIED;
	NTSTATUS status = FLT_PREOP_COMPLETE;
	
	FltCompletePendedPreOperation(file_operation->data, status, NULL);
	LOG_MSG("FltCompletePendedPreOperation");

	ExFreePoolWithTag(file_operation, FILE_OPERATION_TAG);

	LOG_MSG("complete_operation END");
}