#include "minifilter.h"

#include <ntstrsafe.h>

FLT_PREOP_CALLBACK_STATUS pre_operation_callback(
	_Inout_ PFLT_CALLBACK_DATA data,
	_In_ PCFLT_RELATED_OBJECTS filter_objects,
	_Flt_CompletionContext_Outptr_ PVOID* completion_callback
) {
	DbgPrint("FIM: pre_operation_callback START\n");

	UNREFERENCED_PARAMETER(filter_objects);
	UNREFERENCED_PARAMETER(completion_callback);

	OPERATION_TYPE operation_type = get_operation_type(data);
	if (operation_type != OPERATION_TYPE_DELETE &&
		operation_type != OPERATION_TYPE_MOVE &&
		operation_type != OPERATION_TYPE_RENAME) {
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	LONG operation_id = InterlockedIncrement(&g_operation_id);

	FIM_MESSAGE message;
	NTSTATUS status = create_confirmation_message(data, operation_id, &message);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = send_message_to_user(&message);
	if (!NT_SUCCESS(status)) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	status = add_operation_to_pending_list(data, operation_id);
	if (!NT_SUCCESS(status)) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	DbgPrint("FIM: pre_operation_callback END\n");

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
	DbgPrint("FIM: post_operation_callback START\n");

	FIM_MESSAGE message;
	NTSTATUS status = create_log_message(data, &message);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = send_message_to_user(&message);
	if (!NT_SUCCESS(status)) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	DbgPrint("FIM: post_operation_callback END\n");
	return FLT_POSTOP_FINISHED_PROCESSING;
}


OPERATION_TYPE get_operation_type(_Inout_ PFLT_CALLBACK_DATA data) {
	DbgPrint("FIM: get_operation_type START\n");

	OPERATION_TYPE operation_type = OPERATION_TYPE_INVALID;
	switch (data->Iopb->MajorFunction) {
	case IRP_MJ_CREATE:
	{
		DbgPrint("FIM: OPERATION_TYPE_CREATE\n");
		operation_type = OPERATION_TYPE_CREATE;
		break;
	}
	case IRP_MJ_WRITE:
	{
		DbgPrint("FIM: OPERATION_TYPE_WRITE\n");
		operation_type = OPERATION_TYPE_WRITE;
		break;
	}
	case IRP_MJ_CLOSE:
	{
		DbgPrint("FIM: OPERATION_TYPE_CLOSE\n");
		operation_type = OPERATION_TYPE_CLOSE;
		break;
	}
	case IRP_MJ_SET_INFORMATION:
	{
		DbgPrint("FIM: IRP_MJ_SET_INFORMATION\n");

		FILE_INFORMATION_CLASS file_information_class = data->Iopb->Parameters.SetFileInformation.FileInformationClass;
		if (file_information_class == FileDispositionInformation ||
			file_information_class == FileDispositionInformationEx) {

			PFILE_DISPOSITION_INFORMATION file_information = (PFILE_DISPOSITION_INFORMATION)data->Iopb->Parameters.SetFileInformation.InfoBuffer;
			if (file_information->DeleteFile) {
				operation_type = OPERATION_TYPE_DELETE;
				DbgPrint("FIM: OPERATION_TYPE_DELETE\n");

			}
		}
		else if (file_information_class == FileRenameInformation ||
			file_information_class == FileRenameInformationEx) {
			PFILE_RENAME_INFORMATION file_information = (PFILE_RENAME_INFORMATION)data->Iopb->Parameters.SetFileInformation.InfoBuffer;
			if (file_information->RootDirectory == NULL) {
				operation_type = OPERATION_TYPE_RENAME;
				DbgPrint("FIM: OPERATION_TYPE_RENAME\n");

			}
			else {
				operation_type = OPERATION_TYPE_MOVE;
				DbgPrint("FIM: OPERATION_TYPE_MOVE\n");

			}
		}
		break;
	}
	case IRP_MJ_CLEANUP:
	{
		DbgPrint("FIM: OPERATION_TYPE_CLEANUP\n");

		operation_type = OPERATION_TYPE_CLEANUP;
		break;
	}
	default:
	{
		DbgPrint("FIM: OPERATION_TYPE_INVALID\n");

		operation_type = OPERATION_TYPE_INVALID;
	}
	}
	DbgPrint("FIM: get_operation_type END\n");
	return operation_type;
}

NTSTATUS get_file_name(_Inout_ PFLT_CALLBACK_DATA data, _Out_ PUNICODE_STRING file_name) {
	DbgPrint("FIM: get_file_name START\n");

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

	DbgPrint("FIM: get_file_name END\n");

	return status;
}