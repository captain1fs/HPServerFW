#include"../sylar/config.h"
#include"../sylar/log.h"

#include<iostream>
#include<yaml-cpp/yaml.h>

sylar::ConfigVar<int>::ptr g_int_value_config = sylar::ConfigMgr::Lookup("system.port", (int)8080, "system port");
sylar::ConfigVar<float>::ptr g_float_value_config = sylar::ConfigMgr::Lookup("system.value", (float)123.456, "system value");

void print_yaml(const YAML::Node& node, int level){
    if(node.IsNull()){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level*4, ' ') << "Null - " << node.Type() << " - " << level;
    }else if(node.IsScalar()){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level*4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    }else if(node.IsSequence()){
        for(size_t i = 0;i < node.size(); ++i){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level*4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }else if(node.IsMap()){
        for(auto it = node.begin(); it != node.end(); ++it){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level*4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    }
}

void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/fangshao/CPP/Project/sylar/bin/conf/log.yaml");
    print_yaml(root, 0);

    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}

void test_config(){
    //打印初始值
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getVal();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();

    YAML::Node root = YAML::LoadFile("/home/fangshao/CPP/Project/sylar/bin/conf/log.yaml");
    sylar::ConfigMgr::LoadFromYaml(root);
    //从yaml文件中获取自定义的值
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getVal();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();
}

int main(){
    // sylar::ConfigVar<int>::ptr g_int_value_config = sylar::ConfigMgr::Lookup("system port", (int)8080, "system port");
    
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT());
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getVal();
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();

    // test_yaml();

    test_config();

    return 0;
}