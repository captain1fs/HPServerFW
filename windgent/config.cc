#include "config.h"

namespace windgent {

ConfigVarBase::ptr ConfigMgr::LookupBase(const std::string& name){
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

static void ListAllMember(const std::string& prefix, const YAML::Node& node, std::list<std::pair<std::string, const YAML::Node> >& output){
    if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._12345678") != std::string::npos){
        LOG_ERROR(LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if(node.IsMap()){
        for(auto it = node.begin(); it != node.end(); ++it){
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

void ConfigMgr::LoadFromYaml(const YAML::Node& root){
    std::list<std::pair<std::string, const YAML::Node> > all_nodes;
    ListAllMember("", root, all_nodes);

    for(auto& node : all_nodes){
        std::string key = node.first;    // int_vec
        // std::cout << "ConfigMgr::LoadFromYaml() " << key << std::endl;
        if(key.empty()){
            continue;
        }
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);

        if(var){
            if(node.second.IsScalar()){
                var->fromString(node.second.Scalar());
            }else{
                std::stringstream ss;
                ss << node.second;
                // std::cout << "ConfigMgr::LoadFromYaml() " << ss.str() << std::endl;   //[10, 20, 30]
                var->fromString(ss.str());
            }
        }
    }
}

}