#include "minifilter.h"

#include <fltKernel.h>

NTSTATUS register_filter(_In_ PDRIVER_OBJECT driver_object)
{
	DbgPrint("FIM: register_filter START\n");

	CONST FLT_OPERATION_REGISTRATION callbacks[] = {
		{IRP_MJ_CREATE,
		 0,
		 NULL,
		 post_operation_callback},

		{IRP_MJ_CLOSE,
		 0,
		 NULL,
		 post_operation_callback},

		{IRP_MJ_WRITE,
		 0,
		 NULL,
		 post_operation_callback},

		{IRP_MJ_CLEANUP,
		 0,
		 NULL,
		 post_operation_callback},

		{IRP_MJ_SET_INFORMATION,
		 0,
		 pre_operation_callback,
		 NULL},

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
	DbgPrint("FIM: register_filter END\n");

	return FltRegisterFilter(driver_object, &registration_data, &g_context.registered_filter);
}
