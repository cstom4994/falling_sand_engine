// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

namespace MetaEngine {

    class EditorLayer {
    private:
    public:
        EditorLayer(){};
        ~EditorLayer() = default;

        void onUpdate();
        void onAttach();
        void onDetach();
        void onImGuiRender();
    };
}// namespace MetaEngine
