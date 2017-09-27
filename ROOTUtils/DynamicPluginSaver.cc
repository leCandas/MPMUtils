/// \file DynamicPluginSaver.cc
#include "DynamicPluginSaver.hh"
#include <TObjString.h>
#include "StringManip.hh"
#include <cassert>

map<string, PluginRegistrar*>& DynamicPluginSaver::builderTable() {
    static map<string, PluginRegistrar*> BT;
    return BT;
}

DynamicPluginSaver::DynamicPluginSaver(OutputManager* pnt, const string& nm, const string& inflName): PluginSaver(pnt,nm,inflName) {
    configstr = registerAttrString("configstr", "");
    assert(configstr);
}

void DynamicPluginSaver::Reconfigure() {
    if(configstr->String().Length()) {
        Config cfg;
        cfg.setAutoConvert(true);
        string s = configstr->String().Data();
        cfg.readString(s);
        Configure(cfg.getRoot());
    } else printf("No configuration found in loaded file!\n");
}

void DynamicPluginSaver::Configure(const Setting& cfg) {
    // save copy of config to output
    auto srcfl = cfg.getSourceFile();
    if(srcfl) configstr->SetString(loadFileString(srcfl).c_str());

    // configure plugins
    if(cfg.exists("plugins")) {
        auto& plugs = cfg["plugins"];
        auto nplugs = plugs.getLength();
        for(int i=0; i<nplugs; i++) {
            string pname = plugs[i].getName();
            auto it = builderTable().find(pname);
            if(it == builderTable().end()) {
                fprintf(stderr,"Unknown plugin type '%s' configured! I die!\n", pname.c_str());
                printf("Available plugins:\n");
                for(auto kv: builderTable()) printf("\t%s\n", kv.first.c_str());
                printf("--------------------\n");
                throw;
            }
            myBuilders[pname] = it->second->makeBuilder(plugs[i]);
        }
    }
    buildPlugins();
}

void DynamicPluginSaver::LoadConfig(const string& fname) {
    Config cfg;
    cfg.setAutoConvert(true);
    cfg.readFile(fname.c_str());
    Configure(cfg.getRoot());
}