#include"../sylar/config.h"
#include"../sylar/log.h"

#include<iostream>
#include<yaml-cpp/yaml.h>

sylar::ConfigVar<int>::ptr g_int_value_config = sylar::ConfigMgr::Lookup("system.port", (int)8080, "system port");
sylar::ConfigVar<float>::ptr g_int_valuex_config = sylar::ConfigMgr::Lookup("system.port", (float)8080, "system port"); //同名但类型不对，执行时会报错
sylar::ConfigVar<float>::ptr g_float_value_config = sylar::ConfigMgr::Lookup("system.value", (float)123.456, "system value");

sylar::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = sylar::ConfigMgr::Lookup("system.int_vec", std::vector<int>{1, 2, 3}, "system int vector");
sylar::ConfigVar<std::list<int> >::ptr g_int_lst_value_config = sylar::ConfigMgr::Lookup("system.int_lst", std::list<int>{4, 5, 6}, "system int list");
sylar::ConfigVar<std::set<int> >::ptr g_int_set_value_config = sylar::ConfigMgr::Lookup("system.int_set", std::set<int>{7, 8, 9}, "system int set");
sylar::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config = sylar::ConfigMgr::Lookup("system.int_uset", std::unordered_set<int>{7, 8, 9}, "system int uset");
sylar::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config = sylar::ConfigMgr::Lookup("system.str_int_map", std::map<std::string, int>{{"hello",0}, {"world",1}}, "system string int map");
sylar::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config = sylar::ConfigMgr::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"hello",4}, {"world",5}}, "system string int umap");

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

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getVal(); \
        for(auto& i : v){ \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
         SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getVal(); \
        for(auto& i : v){ \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": {" << i.first << " - " << i.second << "}"; \
        } \
         SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_lst_value_config, int_lst, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    YAML::Node root = YAML::LoadFile("/home/fangshao/CPP/Project/sylar/bin/conf/log.yaml");
    sylar::ConfigMgr::LoadFromYaml(root);
    //从yaml文件中获取自定义的值
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getVal();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_lst_value_config, int_lst, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
}

class Person {
public:
    Person() {}
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name= " << m_name << " age= " << m_age << " sex= " << m_sex << "]";
        return ss.str();
    }
};

namespace sylar {

//针对Person偏特化
template<>
class LexicalCast<std::string, Person> {
public:
    Person operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};
template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator() (const Person& p){
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}

sylar::ConfigVar<Person>::ptr g_person = sylar::ConfigMgr::Lookup("class.person", Person(), "class person");
sylar::ConfigVar<std::map<std::string, Person> >::ptr g_person_map = sylar::ConfigMgr::Lookup("class.person_map", std::map<std::string, Person>(), "class person map");
sylar::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map = sylar::ConfigMgr::Lookup("class.person_vec_map", std::map<std::string, std::vector<Person>>(), "class person map");

void test_class() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person->getVal().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) { \
        auto& m = g_var->getVal(); \
        for(auto& i : m){ \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << prefix << ": size= " << m.size(); \
    }

    XX_PM(g_person_map, "class.person before");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "class.person_vec_map before: "  << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/fangshao/CPP/Project/sylar/bin/conf/log.yaml");
    sylar::ConfigMgr::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person->getVal().toString() << " - " << g_person->toString();

    XX_PM(g_person_map, "class.person after");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "class.person_vec_map after: " << g_person_vec_map->toString();
}

int main(){
    // sylar::ConfigVar<int>::ptr g_int_value_config = sylar::ConfigMgr::Lookup("system port", (int)8080, "system port");
    
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT());
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getVal();
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();

    // test_yaml();

    // test_config();
    test_class();

    return 0;
}