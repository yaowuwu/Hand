#ifndef __SYLAR_LOG_H_
#define __SYLAR_LOG_H_

#include <string>
#include <stdint.h> //for int32_t
#include <memory> //for LogAppender::ptr
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h> //for va args 
#include <map>
#include "util.h"
#include "singleton.h"

// #define SYLAR_LOG_LEVEL(logger, level)
//     if(logger->getLevel() <= level) 
//         LogEvent::ptr(new LogEvent(__FILE__, __LINE__, 0, sylar::GetThreadId(), 
//             sylar::GetFiberId(),time(0)))   ->getSS()
//问题: 此处仅LogEvent::ptr 无法获得stream流 getSS()方法 
//解决: 引用Wrap , 返回LogEvent, 获得getSS方法

//宏 把 命名空间加上 因为已经在空间外了, 不信自己往下看
#define SYLAR_LOG_LEVEL(logger, level)\
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), \
            sylar::GetFiberId(),time(0)))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...)\
    if(logger->getLevel() <= level)\
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
        __FILE__, __LINE__,0,sylar::GetThreadId(), \
        sylar::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()  //返回 logger ptr

namespace sylar{

class Logger;

//日志级别
class LogLevel{
public:
    enum Level{
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    static const char* ToString(LogLevel::Level level);
};


//日志事件
//每个日志生成的时候定义为一个logevent.它把所有的属性字段放在一起
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    //LogEvent();
    LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level, const char* file,int32_t line, 
    uint32_t elapse,uint32_t thread_id, uint32_t fiber_id , uint64_t time);

    const char* getFile() const {return m_file;}
    int32_t getLine() const {return m_line;}
    uint32_t getElapse() const {return m_elapse;}
    uint32_t getThreadId() const {return m_threadId;}
    uint32_t getFiberId() const {return m_fiberId;}
    uint64_t getTime() const {return m_time;}
    //const std::string& getContent() const {return m_content;} //这里前后两个const ?  
    std::string getContent() const {return m_ss.str();}
    std::shared_ptr<Logger> getLogger() const {return m_logger;}
    LogLevel::Level getLevel() const {return m_level;}

    std::stringstream& getSS() {return m_ss;}

    void format(const char* fmt, ...);  //输出日志方法2: 1是宏定义 流式方法, 2是format方法
    void format(const char* fmt, va_list al);

private:
    const char* m_file = nullptr;//文件名 c++11后可以这样定义, 直接在类里面初始化
    int32_t m_line = 0;          //行号
    uint32_t m_elapse = 0;       //程序启动到现在的毫秒数
    int32_t m_threadId = 0;      //线程id
    uint32_t m_fiberId = 0;      //协程id
    uint64_t m_time ;            //时间戳
    //std::string m_content;       //消息体
    std::stringstream m_ss;

    std::shared_ptr<Logger> m_logger; //LogEvent要写入的Logger对象智能指针
    LogLevel::Level m_level;
};

class LogEventWrap{//引用该类是因为LogEvent类能通过(写入目标对象logger), Wrap在析构函数中再将logEvent写入logger里面去
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    LogEvent::ptr getEvent() const {return m_event;}

    std::stringstream& getSS();  
private:
    LogEvent::ptr m_event;
};



//日志格式器 
//每个日志输出地的日志格式不一样, 所有不同的日志输出地定义不同的格式器
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);
    // %m小写%  %t时间
    std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event);  //给每个format 传event 进行格式化

public:
    class FormatItem{ //子模块 子类: 解析成固定格式
        public:
            typedef std::shared_ptr<FormatItem> ptr;
           // FormatItem(const std::string& fmt = "") {};
            virtual ~FormatItem(){}
            //virtual std::string format(LogEvent::ptr event) = 0;
            virtual void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) = 0;
            //因为有多个, 输出到流里比输出到string 性能更好一点
            // 虚类 虚函数, 这样基于这个父类, 就可以定义许多个不同的实际子类
            //不同的子类负责输出各自的一部分
    };

    void init();
private:
        std::string m_pattern;//当pattern被初始化(实例化)赋值的时候,根据pattern解析出它里面的item的信息
        std::vector<FormatItem::ptr> m_items; //列表 == 输出多少不同的项
};

//日志输出地
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {}
    //定义成虚析构函数 因为日志输出地很多 所以不同输出地继承于该父类,
    //父类定义成虚析构函数,防止子类析构时资源释放不完全
    virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level  level, LogEvent::ptr event) = 0;//此处Logger在后面,所以在最头文件上头定义了Logger
    //此处在appender的时候将 logger传给 formatter
    //基类纯虚函数 ,这样子类必须实现该方法

    void setFormatter(LogFormatter::ptr val){ m_formatter = val;} //既然定义了m_formatter就有get和set方法
    LogFormatter::ptr getFormatter() const { return m_formatter;}

    LogLevel::Level getLevel() const {return m_level;}
    void setLevel(LogLevel::Level val){ m_level = val;}
//private:
protected: //子类可能用到level, 所以此处定义成protected
    //LogLevel::Level m_level;
    LogLevel::Level m_level = LogLevel::DEBUG;//不初始化就会随机
    LogFormatter::ptr m_formatter; //不同地方的日志格式不一样, 比如有的地方需要线程日志,所以appender加上m_formatter类
};



//日志器
class Logger : public std::enable_shared_from_this<Logger> {//只有继承于此,才能在该类的成员函数中使用自身职能指针 shared_from_this
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& name = "root");
    //void log(LogLevel::Level level, const LogEvent& event);
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    LogLevel::Level getLevel() const {return m_level;}
    void setLevel(LogLevel::Level val){ m_level = val;}

    const std::string& getName() const {return m_name;}
private:
    std::string m_name; //日志一般都有一个名字 ,构造函数里面默认root
    LogLevel::Level m_level;                    //日志级别
    std::list<LogAppender::ptr> m_appenders;   //Appender集合
    LogFormatter::ptr m_formatter; //此处添加formate ,因为在初始化时, 可能appender不需要formatter,直接用logger这里的formatter做就行了
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender{
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override; //override 描述从父类中继承重载的实现
    private:
        
};

//输出到文件的Appender
class FileLogAppender : public LogAppender{
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string& filename);
        void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override;

        bool reopen();
    private:
        std::string m_filename;
        std::ofstream m_filestream; //#include <stringstreams> fstream
};

class LoggerManager{
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();//logger的初始化可以从配置中读出来
    Logger::ptr getRoot() const {return m_root;} //此处用在了宏 SYLAR_LOG_ROOT 
private:
    std::map<std::string , Logger::ptr>m_loggers;
    Logger::ptr m_root;
};

typedef sylar::Singleton<LoggerManager> LoggerMgr;

}

#endif