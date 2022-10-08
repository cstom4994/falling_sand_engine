// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

#pragma once

#include "Engine/ModuleStack.h"

namespace MetaEngine {

    class EditorLayer : public Module
    {
    private:

    public:
        EditorLayer() : Module("EditorLayer") {};
        ~EditorLayer() override = default;

        void onUpdate() override;
        void onAttach() override;
        void onDetach() override;
        void onImGuiRender() override;

    };
}
