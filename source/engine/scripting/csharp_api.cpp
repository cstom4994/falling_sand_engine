
#include "csharp_api.h"

#include <iostream>

namespace ME {
namespace Scripting {

void hello_from_cpp() { std::cout << "C++: Hello!" << std::endl; }

CSharpBasicApi::CSharpBasicApi(std::string& path_to_mono) {
    mono = new CSharpWrapper::mono(path_to_mono.c_str());

    this->OnInit();
}

void CSharpBasicApi::OnInit() {

    mono->init_jit("SimpleFunctionCallApp");

    // resolve external method in C# code
    mono->add_internal_call<void()>("MonoBindExamples.SimpleFunctionCall::HelloFromCpp()", ME_CS_CALLABLE(hello_from_cpp));

    // load assembly and get C# method
    CSharpWrapper::assembly assembly(mono->get_domain(), "HotFix_Project.dll");
    CSharpWrapper::method method = assembly.get_method("MonoBindExamples.SimpleFunctionCall::HelloFromCSharp()");

    // call C# method
    method.invoke_static<void()>();
}

CSharpBasicApi::~CSharpBasicApi() { delete mono; }

void CSharpBasicApi::OnUpdate() {}

}  // namespace Scripting

}  // namespace ME
