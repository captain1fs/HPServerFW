#ifndef __CONFIG_H__
#define __CONFIG_H__

#include<memory>
#include<string>
#include<vector>
#include<map>
#include<set>
#include<unordered_map>
#include<unordered_set>
#include<list>
#include<ctype.h>
#include<functional>
#include<boost/lexical_cast.hpp>
#include<yaml-cpp/yaml.h>

#include"log.h"

namespace windgent {

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
    virtual std::string getTypeName() const = 0;
 
    const std::string getName() const { return m_name; }
    const std::string getDescription() const { return m_description; }
private:
    std::string m_name;
    std::string m_description;
};

//复杂类型的转换
template<typename F, typename T>
class LexicalCast {
public:
    T operator() (const F& v){
        return boost::lexical_cast<T>(v);
    }
};
//针对vector偏特化
template<typename T>
class LexicalCast<std::string, std::vector<T> > {
public:
    std::vector<T> operator() (const std::string& v){
        typename std::vector<T> vec;
        YAML::Node node = YAML::Load(v);
        std::stringstream ss;
        for(size_t i = 0;i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            // std::cout << "LexicalCast<std::string, std::vector<T> >() " << ss.str() << std::endl;
            vec.push_back(LexicalCast<std::string, T>() (ss.str()));
        }
        return vec;
    }
};
template<typename T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator() (const std::vector<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//针对list偏特化
template<typename T>
class LexicalCast<std::string, std::list<T> > {
public:
    std::list<T> operator() (const std::string& v){
        typename std::list<T> lst;
        YAML::Node node = YAML::Load(v);
        std::stringstream ss;
        for(size_t i = 0;i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            lst.push_back(LexicalCast<std::string, T>() (ss.str()));
        }
        return lst;
    }
};
template<typename T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator() (const std::list<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//针对set偏特化
template<typename T>
class LexicalCast<std::string, std::set<T> > {
public:
    std::set<T> operator() (const std::string& v){
        typename std::set<T> st;
        YAML::Node node = YAML::Load(v);
        std::stringstream ss;
        for(size_t i = 0;i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            st.insert(LexicalCast<std::string, T>() (ss.str()));
        }
        return st;
    }
};
template<typename T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator() (const std::set<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<typename T>
class LexicalCast<std::string, std::unordered_set<T> > {
public:
    std::unordered_set<T> operator() (const std::string& v){
        typename std::unordered_set<T> st;
        YAML::Node node = YAML::Load(v);
        std::stringstream ss;
        for(size_t i = 0;i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            st.insert(LexicalCast<std::string, T>() (ss.str()));
        }
        return st;
    }
};
template<typename T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator() (const std::unordered_set<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//针对map偏特化
template<typename T>
class LexicalCast<std::string, std::map<std::string, T> > {
public:
    std::map<std::string, T> operator() (const std::string& v){
        typename std::map<std::string, T> mp;
        YAML::Node node = YAML::Load(v);
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it){
            ss.str("");
            ss << it->second;
            mp.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>() (ss.str())));
        }
        return mp;
    }
};
template<typename T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator() (const std::map<std::string, T>& v){
        YAML::Node node;
        for(auto& i : v){
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<typename T>
class LexicalCast<std::string, std::unordered_map<std::string, T> > {
public:
    std::unordered_map<std::string, T> operator() (const std::string& v){
        typename std::unordered_map<std::string, T> mp;
        YAML::Node node = YAML::Load(v);
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it){
            ss.str("");
            ss << it->second;
            mp.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>() (ss.str())));
        }
        return mp;
    }
};
template<typename T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator() (const std::unordered_map<std::string, T>& v){
        YAML::Node node;
        for(auto& i : v){
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//FromStr T operator()(const std::string&)
//ToStr std::string operator()(const T&)
template<typename T, typename FromStr = LexicalCast<std::string, T>
                   , typename ToStr = LexicalCast<T, std::string> >
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void (const T& old_val, const T& new_val)> on_change_cb;

    ConfigVar(const std::string& name , const T& val, const std::string& description = "")
        :ConfigVarBase(name, description)
        ,m_val(val){
    }


    //eg：当m_val是vector<int>类型时，根据ToStr指定的偏特化模板LexicalCast<vector<int>, std::string>>()将m_val转换为string类型
    std::string toString() override {
        try {
            // return boost::lexical_cast<std::string>(m_val);
            return ToStr()(m_val);
        } catch(std::exception& e ){
            LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString execption" << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

    //val是yaml文件中每种配置项的值，比如systrm.int_vec的[10, 20, 30]。此时T的类型是vectot<int>，
    //根据模板类型FromStr调用偏特化LexicalCast<std::string, std::vector<T>>()将val转换为vector类型，然后赋值给成员m_val
    bool fromString(const std::string& val) override {
        try {
            // m_val = boost::lexical_cast<T>(val);
            setVal(FromStr()(val));
        } catch(std::exception& e){
            LOG_ERROR(LOG_ROOT()) << "ConfigVar::fromString exception" << e.what() << " convert: string to " << typeid(m_val).name() << " - " << val;
        }
        return false;
    }

    const T getVal() const { return m_val; }
    void setVal(const T& val) { 
        if(val == m_val){
            return;
        }
        for(auto& i : m_cbs){
            i.second(m_val, val);
        }
        m_val = val;
    }
    std::string getTypeName() const override { return typeid(T).name(); }

    void addListener(uint64_t key, on_change_cb cb){
        m_cbs[key] = cb;
    }

    void delListener(uint64_t key){
        m_cbs.erase(key);
    }

    on_change_cb getListener(uint64_t key) const {
        auto it = m_cbs.find(key);
        return it != m_cbs.end() ? it->second : nullptr;
    }

    void clearListener() {
        m_cbs.clear();
    }
private:
    T m_val;
    std::map<uint64_t, on_change_cb> m_cbs;
};

class ConfigMgr {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template<typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description = ""){
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()){
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T> > (it->second);   //dynamic_cast转换失败时返回nullptr
            if(tmp){
                LOG_INFO(LOG_ROOT()) << "Lookup name = " << name << " exists.";
                return tmp;
            }else{
                LOG_INFO(LOG_ROOT()) << "Lookup name = " << name << " exists but type not " << typeid(T).name()
                    << " real_type = " << it->second->getTypeName() << ": " << it->second->toString();
                return nullptr;
            }
        }

        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._12345678") != std::string::npos){
            LOG_ERROR(LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;
    }

    template<typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name){
        auto it = GetDatas().find(name);
        if(it == GetDatas().end()){
            return  nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
    }

    static ConfigVarBase::ptr LookupBase(const std::string& name);
    static void LoadFromYaml(const YAML::Node& root);
private:
    //防止因静态成员初始化顺序问题，s_datas被使用时还没完成初始化，导致报错
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }
};

}

#endif