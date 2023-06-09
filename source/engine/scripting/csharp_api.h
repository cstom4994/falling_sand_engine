#pragma once

#include "csharp_bind.hpp"

namespace ME {
namespace Scripting {
class CSharpBasicApi {

public:
    CSharpBasicApi(std::string& path_to_mono);
    ~CSharpBasicApi();

    void OnInit();
    void OnUpdate();

    CSharpWrapper::mono* GetMono() { return mono; }

private:
    CSharpWrapper::mono* mono;
};
}  // namespace Scripting
}  // namespace ME
