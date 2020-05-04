#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

namespace sylar{


//单例模式封装类  T类型 X 为了创造多个实例对应的Tag N 同一个Tag创造多个实例索引
template<class T, class x = void, int N = 0>
class Singleton{
public:
    static T* GetInstance(){
        static T v;
        return &v;//返回单例裸指针
        //return &GetInstanceX(T,X,N)();
    }
};

template<class T, class x = void , int N = 0>
class SingletonPtr{
public:
    static std::shared_ptr<T>GetInstance(){
        static std::shared_ptr<T> v(new T);
        return v;//返回单例智能指针
    }

};


}

#endif
//日志类 一个应用往往对应一个日志实例
//配置类 应用的配置集中管理, 并提供全局访问
//管理类,
//共享资源类

