#ifndef FILE_INTEGRITY_MONITORING_FILTER_LOG_H
#define FILE_INTEGRITY_MONITORING_FILTER_LOG_H

#if 0
#define LOG_MSG(format, ...) DbgPrint("FIM : " format "\n\r" __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_MSG(format, ...) 
#endif

#endif //FILE_INTEGRITY_MONITORING_FILTER_LOG_H
