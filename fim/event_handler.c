#include "event_handler.h"

#include "communication.h"
#include "file_operation_list.h"
#include "global_context.h"
#include "log.h"

#include <ntstrsafe.h>

BOOLEAN is_agent_connected();

LIST_ENTRY g_file_operation_list;
FAST_MUTEX g_file_operation_list_lock;

FLT_PREOP_CALLBACK_STATUS pre_operation_callback(
	_Inout_ PFLT_CALLBACK_DATA data,
	_In_ PCFLT_RELATED_OBJECTS filter_objects,
	_Flt_CompletionContext_Outptr_ PVOID* completion_callback
) {
	LOG_MSG("pre_operation_callback START");

	if (!is_agent_connected()) {
		LOG_MSG("Agent not connected");
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	UNREFERENCED_PARAMETER(filter_objects);
	UNREFERENCED_PARAMETER(completion_callback);

	OPERATION_TYPE operation_type = get_operation_type(data);
	if (operation_type != OPERATION_TYPE_DELETE &&
		operation_type != OPERATION_TYPE_MOVE &&
		operation_type != OPERATION_TYPE_RENAME) {
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	add_operation_to_file_list(data);

	LOG_MSG("pre_operation_callback END");

	return FLT_PREOP_PENDING;
}

FLT_POSTOP_CALLBACK_STATUS post_operation_callback(
	_Inout_ PFLT_CALLBACK_DATA data,
	_In_ PCFLT_RELATED_OBJECTS filter_objects,
	_In_opt_ PVOID completion_context,
	_In_ FLT_POST_OPERATION_FLAGS flags
) {
	UNREFERENCED_PARAMETER(filter_objects);
	UNREFERENCED_PARAMETER(completion_context);
	UNREFERENCED_PARAMETER(flags);
	LOG_MSG("post_operation_callback START");

	FIM_MESSAGE message;
	NTSTATUS status = create_log_message(data, &message);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = send_message_to_user(&message);
	if (!NT_SUCCESS(status)) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	LOG_MSG("post_operation_callback END");
	return FLT_POSTOP_FINISHED_PROCESSING;
}


OPERATION_TYPE get_operation_type(_Inout_ PFLT_CALLBACK_DATA data) {
	LOG_MSG("get_operation_type START");

	OPERATION_TYPE operation_type = OPERATION_TYPE_INVALID;

	switch (data->Iopb->MajorFunction) {
		case IRP_MJ_CREATE:
		{
			LOG_MSG("OPERATION_TYPE_CREATE");
			operation_type = OPERATION_TYPE_CREATE;
			break;
		}
		case IRP_MJ_WRITE:
		{
			LOG_MSG("OPERATION_TYPE_WRITE");
			operation_type = OPERATION_TYPE_WRITE;
			break;
		}
		case IRP_MJ_SET_INFORMATION:
		{
			LOG_MSG("IRP_MJ_SET_INFORMATION");

			FILE_INFORMATION_CLASS file_information_class = data->Iopb->Parameters.SetFileInformation.FileInformationClass;
			if (file_information_class == FileDispositionInformation ||
				file_information_class == FileDispositionInformationEx) {

				PFILE_DISPOSITION_INFORMATION file_information = (PFILE_DISPOSITION_INFORMATION)data->Iopb->Parameters.SetFileInformation.InfoBuffer;
				if (file_information->DeleteFile) {
					operation_type = OPERATION_TYPE_DELETE;
					LOG_MSG("OPERATION_TYPE_DELETE");

				}
			}
			else if (file_information_class == FileRenameInformation ||
				file_information_class == FileRenameInformationEx) {
				PFILE_RENAME_INFORMATION file_information = (PFILE_RENAME_INFORMATION)data->Iopb->Parameters.SetFileInformation.InfoBuffer;
				if (file_information->RootDirectory == NULL) {
					operation_type = OPERATION_TYPE_RENAME;
					LOG_MSG("OPERATION_TYPE_RENAME");

				}
				else {
					operation_type = OPERATION_TYPE_MOVE;
					LOG_MSG("OPERATION_TYPE_MOVE");

				}
			}
			break;
		}
		default:
		{
			LOG_MSG("OPERATION_TYPE_INVALID");

			operation_type = OPERATION_TYPE_INVALID;
		}
	}
	LOG_MSG("get_operation_type END");
	return operation_type;
}

NTSTATUS get_file_name(_Inout_ PFLT_CALLBACK_DATA data, _Out_ PUNICODE_STRING file_name) {
	LOG_MSG("get_file_name START");

	if (file_name == NULL) {
		return STATUS_INVALID_PARAMETER;
	}

	PFLT_FILE_NAME_INFORMATION name_info;
	NTSTATUS status = FltGetFileNameInformation(data, FLT_FILE_NAME_NORMALIZED, &name_info);
	if (NT_SUCCESS(status)) {
		status = FltParseFileNameInformation(name_info);
		if (NT_SUCCESS(status)) {
			status = RtlUnicodeStringCopy(file_name, &name_info->Name);
		}

		FltReleaseFileNameInformation(name_info);
	}

	LOG_MSG("get_file_name END");

	return status;
}

BOOLEAN is_agent_connected() {
	return (g_context.client_port != NULL);
}