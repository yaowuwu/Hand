#ifndef __SYLAR_UTIL__H__
#define __SYLAR_UTIL__H__

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>//将 0 转换成 uint32_t

namespace sylar{

pid_t GetThreadId();
uint32_t GetFiberId();


}

#endif






