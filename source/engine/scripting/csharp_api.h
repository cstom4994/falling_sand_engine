#pragma once

#include "csharp_bind.hpp"

namespace ME {
class CSharpBasicApi {

public:
    CSharpBasicApi(std::string& path_to_mono);
    ~CSharpBasicApi();

    void OnInit();
    void OnUpdate();

    csharp_wrapper::mono* GetMono() { return mono; }

private:
    csharp_wrapper::mono* mono;
};
}  // namespace ME
