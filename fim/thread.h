#ifndef FILE_INTEGRITY_MONITORING_THREAD_H
#define FILE_INTEGRITY_MONITORING_THREAD_H

#include <fltKernel.h>

#define JOIN_RELATIVE_TIMEOUT -100000 // wait for 10 milisecond to complete the thread

NTSTATUS thread_create(PETHREAD thread_object, PKSTART_ROUTINE thread_routine);
VOID thread_join(PVOID thread_object);

#endif

