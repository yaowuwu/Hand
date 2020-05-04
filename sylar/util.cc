#include "util.h"

namespace sylar{

pid_t GetThreadId(){
    return  syscall(SYS_gettid);
}

uint32_t GetFiberId(){
    return 0; //TODO 暂时还没有协程库
}

}