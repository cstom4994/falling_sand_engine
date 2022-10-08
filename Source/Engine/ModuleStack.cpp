// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

#include "Engine/InEngine.h"
#include "ModuleStack.h"

namespace MetaEngine {

    Module::Module(const std::string& debugName)
        : m_name(debugName)
    {
    }

    Module::~Module()
    {
    }


    ModuleStack::ModuleStack()
    {
    }

    ModuleStack::~ModuleStack()
    {
        for (Module* layer : m_Layers) {
            m_instances.erase(layer->getName());
            delete layer;
        }
    }

    void ModuleStack::pushLayer(Module* layer)
    {
        m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
        m_LayerInsertIndex++;
        m_instances.insert(std::make_pair(layer->getName(), layer));
        layer->onAttach();
    }

    void ModuleStack::pushOverlay(Module* overlay)
    {
        m_Layers.emplace_back(overlay);
        m_instances.insert(std::make_pair(overlay->getName(), overlay));
        overlay->onAttach();
    }

    void ModuleStack::popLayer(Module* layer)
    {
        auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
        if (it != m_Layers.end())
        {
            m_instances.erase(layer->getName());
            m_Layers.erase(it);
            m_LayerInsertIndex--;
        }
        layer->onDetach();
    }

    void ModuleStack::popOverlay(Module* overlay)
    {
        auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
        if (it != m_Layers.end()) {
            m_instances.erase(overlay->getName());
            m_Layers.erase(it);
        }
        overlay->onDetach();
    }

}
