// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include "Game/InEngine.h"

namespace MetaEngine {

    class Module {
    protected:
        std::string m_name;

    public:
        Module(const std::string &name = "Layer");
        virtual ~Module();

        virtual void onAttach() {
        }

        virtual void onDetach() {
        }

        virtual void onUpdate() {
        }

        virtual void onRender() {
        }

        virtual void onImGuiRender() {
        }

        virtual void onImGuiInnerRender() {
        }

        inline const std::string &getName() const { return m_name; }
    };

    class ModuleStack {
    public:
        ModuleStack();
        ~ModuleStack();

        void pushLayer(Module *layer);
        void popLayer(Module *layer);

        std::vector<Module *>::iterator begin() { return m_Layers.begin(); }
        std::vector<Module *>::iterator end() { return m_Layers.end(); }

        Module *getInstances(std::string name) { return m_instances[name]; }

    private:
        std::vector<Module *> m_Layers;
        std::map<std::string, Module *> m_instances;

        unsigned int m_LayerInsertIndex = 0;
    };


#define METADOT_MODULE_GET(name, class, to) auto to = reinterpret_cast<class *>(m_ModuleStack->getInstances(name))

}// namespace MetaEngine
