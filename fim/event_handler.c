#include "minifilter.h"

FLT_PREOP_CALLBACK_STATUS preoperation_callback_for_delete(
    _Inout_ PFLT_CALLBACK_DATA data,
    _In_ PCFLT_RELATED_OBJECTS filter_objects,
    _Flt_CompletionContext_Outptr_ PVOID* completion_callback
) {
    UNREFERENCED_PARAMETER(filter_objects);
    UNREFERENCED_PARAMETER(completion_callback);

    if (data->Iopb->MajorFunction != IRP_MJ_SET_INFORMATION) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    FILE_INFORMATION_CLASS file_information_class = data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    if (file_information_class != FileDispositionInformation &&
        file_information_class != FileDispositionInformationEx) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    PFILE_DISPOSITION_INFORMATION file_information = (PFILE_DISPOSITION_INFORMATION)data->Iopb->Parameters.SetFileInformation.InfoBuffer;
    if (!file_information->DeleteFile) {
        DbgPrint("Operation is not DELETE");
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    PFLT_FILE_NAME_INFORMATION name_info;
    NTSTATUS status = FltGetFileNameInformation(data, FLT_FILE_NAME_NORMALIZED, &name_info);
    if (!NT_SUCCESS(status)) {
        DbgPrint("FltGetFileNameInformation failed. status 0x%x\n", status);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    status = FltParseFileNameInformation(name_info);
    if (!NT_SUCCESS(status)) {
        DbgPrint("FltParseFileNameInformation failed. status 0x%x\n", status);
        FltReleaseFileNameInformation(name_info);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
    // get name todo

    status = send_message_to_user(data, &name_info->Name);
    if (!NT_SUCCESS(status)) {
        DbgPrint("send_message_to_user failed. status 0x%x\n", status);
        FltReleaseFileNameInformation(name_info);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    FltReleaseFileNameInformation(name_info);

    return FLT_PREOP_PENDING;
}

