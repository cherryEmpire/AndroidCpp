#ifndef HARMONYOS_ICLOG_H
#define HARMONYOS_ICLOG_H

#include <stdio.h>
#include <string.h>

#define IC_LOG_ERROR(format, ...)    printf("[E][IC_SRV][%s:%d] " format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define IC_LOG_WARNING(format, ...)  printf("[W][IC_SRV][%s:%d] " format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define IC_LOG_INFO(format, ...)     printf("[I][IC_SRV][%s:%d] " format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define IC_LOG_DEBUG(format, ...)    printf("[D][IC_SRV][%s:%d] " format "\n", __func__, __LINE__, ##__VA_ARGS__)

#endif