// Copyright(c) 2022, KaoruXun All rights reserved.
























#include <iostream>

#include "dukpp.hpp"

#include "Engine/Core.hpp"




static void native_print(dukpp::duk_value const& arg) {
    METADOT_INFO(arg.as_c_string());
}

static bool native_call_method(std::string method, dukpp::this_context tctx) {
    auto ctx = dukpp::context_view(tctx);
    ctx.call(method);

    return true;
}

static void native_variadic(dukpp::variadic_args const& args) {
    for (auto const& val : args) {
        std::cout << val.type_name() << "  " << val << std::endl;
    }
}

class Dog {
    std::string name;
public:
    Dog() : name("") {
        std::cout << "Dog created" << std::endl;
    }

    ~Dog() {
        std::cout << "Dog deleted" << std::endl;
    }

    void SetName(std::optional<std::string> const& n) {
        name = n.value_or("Dog");
    }

    std::optional<std::string> GetName() const {
        return name;
    }

    void Bark() const {
        std::cout << "Bark!" << std::endl;
    }
};

class Cls {
};

void stub() {
}

static const char* test_ts = R"js(
print('Hello world!');
)js";

void testDukpp() {
    dukpp::context ctx;
    // duk_push_object(ctx.get_duk_context());
    // duk_push_object(ctx.get_duk_context());
    // duk_put_prop_string(ctx.get_duk_context(), -2, "exports");
    // duk_put_global_string(ctx.get_duk_context(), "module");
    // DukPreloadTypescript(ctx.get_duk_context());
    // duk_eval_string_noresult(ctx.get_duk_context(), ts_js);

    //auto content = ts_transpile(ctx.get_duk_context(), test_ts);
    //duk_eval_string_noresult(ctx.get_duk_context(), content.c_str());

    ctx["print"] = native_print;
    ctx["call_method"] = native_call_method;

    ctx.peval(R"(
call_method("func");
function func() {
    print("Called Func");
}
)");

    ctx.register_class<Dog>("Dog")
        .add_method("bark", &Dog::Bark)
        .add_property("name", &Dog::GetName, &Dog::SetName);

    ctx.peval(R"(
var dog = new Dog();
dog.name = "Puppy";
print("Dog's Name: " + dog.name);
dog.bark();
)");


    ctx["nativeFunc"] = &stub;
    Cls cls;
    ctx["obj"] = &cls;

    std::cout << ctx.peval<dukpp::duk_value>(R"(null)") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"(undefined)") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"(12345)") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"(1.2345)") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"("string")") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"(true)") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"([1, 2, 3, 4])") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"(o = {a: 1, b: 2};)") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"(function func(){}; func;)") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"(nativeFunc;)") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"(obj;)") << std::endl;
    std::cout << ctx.peval<dukpp::duk_value>(R"(u8 = new Uint8Array([ 0x41, 0x42, 0x43, 0x44 ]);)") << std::endl;
    auto val = ctx.peval<dukpp::duk_value>(R"(pu8 = Uint8Array.allocPlain(8);
for (var i = 0; i < pu8.length; i++) {
    pu8[i] = 0x41 + i;
}
pu8;
)");
    std::cout << val << std::endl; // print plan buffer via duk_value

    duk_size_t sz = 0;
    auto buf = val.as_plain_buffer(&sz);
    buf[0] = 0x60;

    std::cout << ctx.peval<dukpp::duk_value>(R"(pu8;)") << std::endl; // eval pu8 again

    ctx["copied_val"] = val; // *copy* value to "copied_val"

    auto copied_val = ctx.peval<dukpp::duk_value>("copied_val;");
    copied_val.as_plain_buffer()[0] = 0x11;

    std::cout << ctx.peval<dukpp::duk_value>(R"(pu8;)") << std::endl; // eval pu8 again
    std::cout << ctx.peval<dukpp::duk_value>(R"(copied_val;)") << std::endl;






    ctx["variadic"] = native_variadic;

    ctx.peval(R"(
variadic("hello", 123, [2, 3]);
)");

    return;
}



