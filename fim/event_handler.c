#include "minifilter.h"

#include <ntstrsafe.h>

#define MAX_FILE_NAME_LENGTH 260

FLT_PREOP_CALLBACK_STATUS pre_operation_callback(
	_Inout_ PFLT_CALLBACK_DATA data,
	_In_ PCFLT_RELATED_OBJECTS filter_objects,
	_Flt_CompletionContext_Outptr_ PVOID* completion_callback
) {
	UNREFERENCED_PARAMETER(filter_objects);
	UNREFERENCED_PARAMETER(completion_callback);

	OPERATION_TYPE operation_type = get_operation_type(data);
	if (operation_type == OPERATION_TYPE_DELETE ||
		operation_type == OPERATION_TYPE_MOVE ||
		operation_type == OPERATION_TYPE_RENAME) {
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	WCHAR buffer[MAX_FILE_NAME_LENGTH];
	UNICODE_STRING file_name = { .Length = 0, .MaximumLength = MAX_FILE_NAME_LENGTH, .Buffer = buffer };
	NTSTATUS status = get_file_name(data, &file_name);
	if (!NT_SUCCESS(status)) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	LONG operation_id = InterlockedIncrement(&g_operation_id);

	FIM_MESSAGE* message;
	status = create_message(&message, &file_name, operation_type, operation_id);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = send_message_to_user(message);
	if (!NT_SUCCESS(status)) {
		ExFreePool(message);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	ExFreePool(message);

	status = add_operation_to_pending_list(data, operation_id);
	if (!NT_SUCCESS(status)) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	return FLT_PREOP_PENDING;
}

FLT_POSTOP_CALLBACK_STATUS post_operation_callback(
	_Inout_ PFLT_CALLBACK_DATA data,
	_In_ PCFLT_RELATED_OBJECTS filter_objects,
	_In_opt_ PVOID completion_context,
	_In_ FLT_POST_OPERATION_FLAGS flags
) {
	UNREFERENCED_PARAMETER(data);
	UNREFERENCED_PARAMETER(filter_objects);
	UNREFERENCED_PARAMETER(completion_context);
	UNREFERENCED_PARAMETER(flags);

	return FLT_POSTOP_FINISHED_PROCESSING;
}


OPERATION_TYPE get_operation_type(_Inout_ PFLT_CALLBACK_DATA data) {
	OPERATION_TYPE operation_type = OPERATION_TYPE_INVALID;
	switch (data->Iopb->MajorFunction) {
	case IRP_MJ_CREATE:
	{
		operation_type = OPERATION_TYPE_CREATE;
		break;
	}
	case IRP_MJ_WRITE:
	{
		operation_type = OPERATION_TYPE_WRITE;
		break;
	}
	case IRP_MJ_CLOSE:
	{
		operation_type = OPERATION_TYPE_CLOSE;
		break;
	}
	case IRP_MJ_SET_INFORMATION:
	{
		FILE_INFORMATION_CLASS file_information_class = data->Iopb->Parameters.SetFileInformation.FileInformationClass;
		if (file_information_class == FileDispositionInformation ||
			file_information_class == FileDispositionInformationEx) {

			PFILE_DISPOSITION_INFORMATION file_information = (PFILE_DISPOSITION_INFORMATION)data->Iopb->Parameters.SetFileInformation.InfoBuffer;
			if (file_information->DeleteFile) {
				operation_type = OPERATION_TYPE_DELETE;
			}
		}
		else if (file_information_class == FileRenameInformation ||
			file_information_class == FileRenameInformationEx) {
			PFILE_RENAME_INFORMATION file_information = (PFILE_RENAME_INFORMATION)data->Iopb->Parameters.SetFileInformation.InfoBuffer;
			if (file_information->RootDirectory == NULL) {
				operation_type = OPERATION_TYPE_RENAME;
			}
			else {
				operation_type = OPERATION_TYPE_MOVE;
			}
		}
		break;
	}
	case IRP_MJ_CLEANUP:
	{
		operation_type = OPERATION_TYPE_CLEANUP;
		break;
	}
	default:
	{
		operation_type = OPERATION_TYPE_INVALID;
	}
	}
	return operation_type;
}

NTSTATUS get_file_name(_Inout_ PFLT_CALLBACK_DATA data, _Out_ PUNICODE_STRING file_name) {
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

	return status;
}