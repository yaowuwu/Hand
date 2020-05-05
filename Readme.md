### 2020/05/02 大半夜编译gcc 2~3h 
### 我的五一献给c++服务器开发
### 2020/05/06 臭兰兰拉着我打游戏
# syalr

bin 
      安装boost
      yum install boost
      yum install boost-devel
      yum install boost-doc

## 开发环境
Centos7
gcc 9.1
cmake

## 日志系统

Log4J 仿照

Logger(定义日志类别)
   |
   |----Formatter(日志格式)
   |   
Appender(日志输出地方)

##配置系统

配置系统的原则: 约定优于配置

```cpp
template<T, FromStr, ToStr>
class ConfigVar;

template<F, T>
LexicalCast;

//容器 片特化, 目前支持vector 
```

更新虚拟机时间
sudo yum install -y man-pages.noarch
sudo ntpdate -u asia.pool.ntp.org

sp sylar/util.h  vim快捷键
:A ? 

yaml-cpp 
mkdir build && cd build && cmake .. && make &&  make install 

find /apps/sylar/include/yaml-cpp -name ".h" | xargs grep "LoadFile"

Config -> Yaml

yaml-cpp : github
```
YAML::Node ndoe = YAML;:LoadFile(filename);

node.IsMap()
for(auto it = node.begin(); it != node.end(); it++){ 

}

node.IsSequence()
for(size_t i = 0; i < node.size(); ++i){

}

node.IsScalar();

```

## 协程库封装

## socket函数库

## http协议开发

## 分布协议

## 推荐系统


