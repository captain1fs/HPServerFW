#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include<memory>
#include<string>
#include<map>
#include<list>
#include<ctype.h>
#include<boost/lexical_cast.hpp>
#include<yaml-cpp/yaml.h>

#include"log.h"

namespace sylar {

//ConfigXXX类用于从yaml文件中读取配置信息
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name), m_description(description){
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() { }

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
 
    const std::string getName() const { return m_name; }
    const std::string getDescription() const { return m_description; }
private:
    std::string m_name;
    std::string m_description;
};

template<typename T>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    ConfigVar(const std::string& name , const T& val, const std::string& description = "")
        :ConfigVarBase(name, description)
        ,m_val(val){
    }

    std::string toString() override {
        try {
            return boost::lexical_cast<std::string>(m_val);
        } catch(std::exception& e ){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString execption" << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

    bool fromString(const std::string& val) override {
        try {
            m_val = boost::lexical_cast<T>(val);
        } catch(std::exception& e){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception" << e.what() << " convert: string to " << typeid(m_val).name();
        }
        return false;
    }

    const T getVal() const { return m_val; }
    void setVal(const T& val) { m_val = val; }
private:
    T m_val;
};

class ConfigMgr {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigMap;

    template<typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description = ""){
        auto tmp = Lookup<T>(name);
        if(tmp){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name = " << name << " exists.";
            return tmp;
        }

        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._12345678") != std::string::npos){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        s_datas[name] = v;
        return v;
    }

    template<typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name){
        auto it = s_datas.find(name);
        if(it == s_datas.end()){
            return  nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
    }

    static ConfigVarBase::ptr LookupBase(const std::string& name);
    static void LoadFromYaml(const YAML::Node& root);
private:
    static ConfigMap s_datas;
};

}

#endif