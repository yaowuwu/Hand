#include "sylar/config.h"
//#include "config.h"

namespace sylar{

//头文件里的声明 static ConfigVarMap s_datas;
Config::ConfigVarMap Config::s_datas; //要拿出来到cc文件里定义一下

ConfigVarBase::ptr Config::LookupBase(const std::string& name){
    auto it = s_datas.find(name);
    return it == s_datas.end() ? nullptr : it->second;
}

//"A.B", 10
// A:
//   B: 10
//   C: str
static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node, 
                          std::list<std::pair<std::string, const YAML::Node> >& output){
    if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos){
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return; 
    }
    output.push_back(std::make_pair(prefix, node));
    if(node.IsMap()){
        for(auto it = node.begin(); it != node.end(); ++it){
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), 
                it->second, output);
        }
    }
}

void Config::LoadFromYaml(const YAML::Node& root){
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);

    for(auto& i : all_nodes){
        std::string key = i.first;
        if(key.empty()){
            continue;
        }
        //std::cout << "key: " << key << std::endl;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);
    
        if(var){
            if(i.second.IsScalar()){
                var->fromString(i.second.Scalar());//将string强转成T
               // std::cout << "i,second " << i.second.Scalar() << " " <<var->fromString(i.second.Scalar()) << std::endl;
            }else{
                std::stringstream ss;
                ss << i.second;

               // std::cout << i.first << " * " << i.second << std::endl;
                var->fromString(ss.str());
            }
        }
    }
}

}