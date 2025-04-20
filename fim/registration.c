#include "minifilter.h"

#include <fltKernel.h>

NTSTATUS register_filter(_In_ PDRIVER_OBJECT driver_object)
{
	CONST FLT_OPERATION_REGISTRATION callbacks[] = {
		//{IRP_MJ_CREATE, 0, PreOperationCallback, NULL},
		//{IRP_MJ_CLOSE,  0, PreOperationCallback, NULL},
		//{IRP_MJ_WRITE,  0, PreOperationCallback, NULL},
		//{IRP_MJ_CLEANUP,0, PreOperationCallback, NULL},
		{IRP_MJ_SET_INFORMATION, 0, preoperation_callback_for_delete, NULL},
		{IRP_MJ_OPERATION_END}
	};

	CONST FLT_REGISTRATION registration_data = {
		.Size = sizeof(FLT_REGISTRATION),
		.Version = FLT_REGISTRATION_VERSION,
		.Flags = 0,
		.ContextRegistration = NULL,
		.OperationRegistration = callbacks,
		.FilterUnloadCallback = filter_unload_callback,
		.InstanceSetupCallback = NULL,
		.InstanceQueryTeardownCallback = NULL,
		.InstanceTeardownStartCallback = NULL,
		.InstanceTeardownCompleteCallback = NULL,
		.GenerateFileNameCallback = NULL,
		.NormalizeNameComponentCallback = NULL,
		.NormalizeContextCleanupCallback = NULL,
		.TransactionNotificationCallback = NULL,
		.NormalizeNameComponentExCallback = NULL,
		.SectionNotificationCallback = NULL,
	};

	return FltRegisterFilter(driver_object, &registration_data, &g_context.registered_filter);
}
