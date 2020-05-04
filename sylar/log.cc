#include "log.h"
#include <map>
#include <iostream>
#include <functional>
#include <time.h>

namespace sylar{

const char* LogLevel::ToString(LogLevel::Level level){
    switch(level){
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;
    
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
    
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
    :m_event(e){

}

LogEventWrap::~LogEventWrap(){
    m_event->getLogger()->log(m_event->getLevel(), m_event);//目的:把自己写进去
}

std::stringstream& LogEventWrap::getSS(){
    return m_event->getSS();
}

void LogEvent::format(const char* fmt, ...){
    va_list al;
    va_start(al, fmt);
    format(fmt,al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al){
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt , al);//
    if(len != -1){
        m_ss << std::string(buf,len);
        free(buf);
    }
}


class MessageFormatItem : public LogFormatter::FormatItem {  
public:
    MessageFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string& str = ""){}
    void format(std::ostream& os,Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {  
public:
    ElapseFormatItem(const std::string& str = ""){}
    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {  
public:
    NameFormatItem(const std::string& str = ""){}
    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << logger->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {  
public:
    ThreadIdFormatItem(const std::string& str = ""){}
    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {  
public:
    FiberIdFormatItem(const std::string& str = ""){}
    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {  
public:
    //DateTimeFormatItem(const std::string& format = "")
   // DateTimeFormatItem(const std::string& format  )
   DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H-%M-%S")//此处赋值 就是做个样子 毫无作用
        :m_format(format){
            if(m_format.empty()){// reset默认时  %d{%Y-%m-%d %H:%M:%S}后面如果没有, 只有%d才会走到这里empty
                m_format = "%Y-%m-%d %H:%M:%S";
               // std::cout << "m_format is empty" << std::endl;
            }//此处明明已经初始化了m_format, 但是m_format仍然为空, 走到循环里面,为什么?
            //此处如果 (d) - (%Y-%m-%d %H:%M:%S) - (1) 才是有参构造函数
            //否则就是走无参构造函数
        }

    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
           // os << event->getTime(); //先不格式化,先简单把时间打印出来,  
           
        //    time_t time = event->getTime();   //指针版
        //    struct tm*  tm = localtime(&time);
        //    localtime_r(&time, tm);
        //    char buf[64];
        //    strftime(buf, sizeof(buf), m_format.c_str(), tm);
        //    os << buf;

           struct tm tm;
           time_t time = event->getTime(); //得到的是logEvent初始化时传入的 time(0)
           //localtime_r(&time, &tm); //localtime_r线程安全  localtime_s传参相反win平台
           localtime_r(&time, &tm);
           char buf[64];
           strftime(buf, sizeof(buf), m_format.c_str(), &tm);
           os << buf;
    }
private:
    std::string m_format ;
};

class FilenameFormatItem : public LogFormatter::FormatItem {  
public:
    FilenameFormatItem(const std::string& str = ""){}
    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {  
public:
    LineFormatItem(const std::string& str = ""){}
    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {  
public:
    NewLineFormatItem(const std::string& str = ""){}
    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << std::endl;
    }
};

//给不带type的字符串 输出格式用 ()
class StringFormatItem : public LogFormatter::FormatItem {  
public:
    StringFormatItem(const std::string& str)
        : m_string(str){}
    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {  
public:
    TabFormatItem(const std::string& str = ""){}
    void format(std::ostream& os,Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << "\t";
    }
private:
    std::string m_string;
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level, const char* file,int32_t line, 
uint32_t elapse,uint32_t thread_id, uint32_t fiber_id , uint64_t time)
    :m_file(file),
     m_line(line),
     m_elapse(elapse),
     m_threadId(thread_id),
     m_fiberId(fiber_id),
     m_time(time) ,
     m_logger(logger),
     m_level(level) {

     }

    //%m --消息体
    //%p -- level
    //%r -- 启动后的时间
    //%c -- 日志名称
    //%t -- 线程id
    //%n -- 回车换行
    //%d -- 时间
    //%f -- 文件名
    //%l -- 行号
    ////str, format, type

Logger::Logger(const std::string& name)
:m_name(name)
,m_level(LogLevel::DEBUG) {
   // m_formatter.reset(new LogFormatter("%d [%p] %f %l %m %n"));//默认的format
   // m_formatter.reset(new LogFormatter("%d  [%p]  <%f:%l>  %m  %n"));//默认的format
     m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));//默认的pattern
    // m_formatter.reset(new LogFormatter("%d%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));//默认的pattern
}

void Logger::addAppender(LogAppender::ptr appender){
    if(!appender->getFormatter()){
        appender->setFormatter(m_formatter);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender){
    for(auto it = m_appenders.begin(); it != m_appenders.end(); it++){
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        auto self = shared_from_this();
        for(auto& i : m_appenders){
            i->log(self,level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event){
        log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event){
        log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event){
        log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string& filename)
    :m_filename(filename){
        reopen(); //此处漏掉reopen,导致文件日志没有输出
}

void FileLogAppender::log(std::shared_ptr<Logger>logger, LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        //m_filestream << m_formatter.format(logger, level,event);
        //LogFormatter::ptr 指针
        m_filestream << m_formatter->format(logger, level,event);
    }
}
//重新打开文件,文件打开成功返回true
bool FileLogAppender::reopen(){
    if(m_filestream){
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream; //返回值为什么要运算两次?   这样才是bool值
}

void StdoutLogAppender::log(std::shared_ptr<Logger>logger, LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){//此处漏掉= 要了老命了
        std::cout << m_formatter->format(logger, level,event);
        // std::string str = m_formatter->format(logger, level, event);
        // std::cout << str << "****" << std::endl;
    }
}

LogFormatter::LogFormatter(const std::string& pattern)
    :m_pattern(pattern){
        init();
}

std::string LogFormatter::format(std::shared_ptr<Logger>logger, LogLevel::Level level,LogEvent::ptr event){
    std::stringstream ss;
    for(auto& i : m_items){
        i->format(ss,logger, level, event);//循环将多个子类ostream 输出到stringstream ss
    }
    return ss.str(); //stringstream的str()方法
}
//仿照 loger4J日志格式定义
//%xxx %xxx{xxx} %% 
void LogFormatter::init(){
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size() ; ++i){
        if(m_pattern[i] != '%'){
            nstr.append(1,m_pattern[i]); //为什么从1开始, 不是从0开始 
            //: string.append(1,"1")表示向string后面添加1个字符
            //扩展 string后面能直接+ c-string  string s("hello"); const char* c = "world"; s.append(c); s.append(c, 3);添加c的一部分
            continue;
        }

        if((i+1) < m_pattern.size()){
            if(m_pattern[i+1] == '%'){
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i+1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < m_pattern.size()){
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}' )){
                str = m_pattern.substr(i+1, n-i-1);
                break;
            }
            if(fmt_status == 0){
                if(m_pattern[n] == '{'){
                    str = m_pattern.substr(i+1, n-i-1);
                   // std::cout << "*" << std::endl;
                    fmt_status = 1;//解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1){
                if(m_pattern[n] == '}'){
                    fmt = m_pattern.substr(fmt_begin +1,n-fmt_begin-1);
                   // std::cout << "#" << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()){
                if(str.empty()){
                    str = m_pattern.substr(i+1);
                }
            }
        }

        if(fmt_status == 0){
            if(!nstr.empty()){
                // vec.push_back(std::make_pair(nstr, "", 0));
                 vec.push_back(std::make_tuple(nstr, std::string() , 0));
                 nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            //i = n;
            i = n -1; //决定空格
        }else if(fmt_status == 1){
           // std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            vec.push_back(std::make_tuple("<pattern_error>", fmt, 0));
        }
    }

    if(!nstr.empty()){
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str,C) \
        {#str, [](const std::string& fmt) {return FormatItem::ptr(new C(fmt));}}

        XX(m,MessageFormatItem),
        XX(p,LevelFormatItem),
        XX(c,NameFormatItem),
        XX(t,ThreadIdFormatItem),
        XX(n,NewLineFormatItem),
        XX(d,DateTimeFormatItem),
        XX(f,FilenameFormatItem),
        XX(l,LineFormatItem),
        XX(T,TabFormatItem),
        XX(F,FiberIdFormatItem)
#undef XX 
    };

    for(auto& i : vec){
        if(std::get<2>(i) == 0){
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }else{
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()){
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
            }else{
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
        //str, format, type
       // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
   // std::cout << m_items.size() << std::endl;

} //init end

LoggerManager::LoggerManager(){
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
}

Logger::ptr LoggerManager::getLogger(const std::string& name){
    auto it = m_loggers.find(name);
    return it == m_loggers.end() ? m_root : it->second;
}


}
