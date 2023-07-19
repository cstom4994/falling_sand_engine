
#include "csharp_api.h"

#include <iostream>

#include "engine/utils/utility.hpp"

namespace ME {

void hello_from_cpp() { ME::println("C++: Hello!"); }

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

}  // namespace ME
