#include "thread.h"

#include <fltKernel.h>

NTSTATUS thread_create(PETHREAD thread_object, PKSTART_ROUTINE thread_routine) {
	OBJECT_ATTRIBUTES object_attributes;
	InitializeObjectAttributes(&object_attributes,
		NULL,
		OBJ_KERNEL_HANDLE,
		NULL,
		NULL);

	HANDLE thread_handle;
	NTSTATUS status = PsCreateSystemThread(&thread_handle,
		THREAD_ALL_ACCESS,
		&object_attributes,
		NULL,
		NULL,
		thread_routine,
		NULL);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = ObReferenceObjectByHandle(
		thread_handle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		(PVOID*)&thread_object,
		NULL
	);

	ZwClose(thread_handle);

	return status;
}

VOID thread_join(PVOID thread_object) {
	KeWaitForSingleObject(
		thread_object,
		Executive,
		KernelMode,
		FALSE,
		&((LARGE_INTEGER) { .QuadPart = JOIN_RELATIVE_TIMEOUT})
	);

	ObDereferenceObject(thread_object);
}