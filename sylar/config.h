#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
//#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include "sylar/log.h"

namespace sylar{

class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name)
        ,m_description(description){
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);

        }
    virtual ~ConfigVarBase(){}

    const std::string& getName() const {return m_name;}
    const std::string& getDescription() const {return m_description;}

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val)  = 0;//解析

protected:
    std::string m_name;
    std::string m_description;
};

//F from_type, T to_type
template<class F, class T>
class LexicalCast{
public:
    T operator() (const F& v){
        return boost::lexical_cast<T>(v);
    }
};

template<class T> //将string -> vector<T>
class LexicalCast<std::string, std::vector<T>>{
public:
    std::vector<T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v); //函数Load 先将string v转为yaml结构node
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i){
            ss.str("");//给流 初始化为空
            ss << node[i];
            vec.push_back(LexicalCast<std::string , T>() (ss.str())); //将流.str() ==> string 转成 T类型
        }
        return vec;
    }
};

template<class T>//将vector<T> --> string
class LexicalCast<std::vector<T>, std::string>{
public:
    std::string operator()(const std::vector<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>() (i))) ;//将vector里面的每一个T类型转换成string, 再LOAD成 yaml结构体 入node,
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//FromStr T operator() (const std::string&)
//ToStr std::string operator() (const T&)
template<class T , class FromStr = LexicalCast<std::string, T>
                 , class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVar> ptr;
   // typedef std::shared_ptr<ConfigVarBase> ptr; 粗心搞成父指针, 怎么可能找到子类函数
    ConfigVar( const std::string& name,const T& default_value,  const std::string& description = "")
        :ConfigVarBase(name, description)
        ,m_val(default_value){

        }

    std::string toString() override {
        try{
            //return boost::lexical_cast<std::string>(m_val);//将T强转成string
            return ToStr() (m_val);
        }catch(std::exception& e){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exeception" <<e.what() << " convert: " 
            << typeid(m_val).name() << " to string";
        }
        return "";
    }

    bool fromString(const std::string& val) override {
        try{
            //m_val = boost::lexical_cast<T>(val);//将string强转成T
            setValue(FromStr() (val));
        }catch (std::exception& e){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception" << e.what() << " convert: string to " 
            << typeid(m_val).name(); 
        }
        return false;
    }

    const T getValue()  { return m_val; }
    void setValue(const T& v) { m_val = v; }
private:
    T m_val;
};//模板子类继承非模板父类

//管理类
class Config{
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template<class T>//创建
    static typename ConfigVar<T>::ptr Lookup( const std::string& name, const T& default_value, const std::string& description = ""){
        auto tmp = Lookup<T>(name);
        if(tmp){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup namne = " << name << " exists";
            return tmp;
        }

        //if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._012345678") != std::string::npos){
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invaild " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>( name,default_value, description));
        s_datas[name] = v;
        return v;
    }

    template<class T>//查找
    static typename ConfigVar<T>::ptr Lookup(const std::string& name){
        auto it = s_datas.find(name);
        if(it == s_datas.end()){//此处笔误,写成s_datas.find(name) , 导致一直成立, 一直为nullptr
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
    }

    static void LoadFromYaml(const YAML::Node& root);

    static ConfigVarBase::ptr  LookupBase(const std::string& name);

private:
    static ConfigVarMap s_datas;
};

}

#endif