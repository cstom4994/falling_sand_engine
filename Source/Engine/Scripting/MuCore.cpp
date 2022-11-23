
#include "MuCore.hpp"
#include "Game/Core.hpp"
#include <string>


// --------------------------------------- expressionImplementation

namespace MuScript {
    ValueRef MuScriptInterpreter::needsToReturn(ExpressionRef exp, ScopeRef scope, Class *classs) {
        if (exp->type == ExpressionType::Return) {
            return getValue(exp, scope, classs);
        } else {
            auto result = consolidated(exp, scope, classs);
            if (result->type == ExpressionType::Return) {
                return get<ValueRef>(result->expression);
            }
        }
        return nullptr;
    }

    ValueRef MuScriptInterpreter::needsToReturn(const vector<ExpressionRef> &subexpressions, ScopeRef scope, Class *classs) {
        for (auto &&sub: subexpressions) {
            if (auto returnVal = needsToReturn(sub, scope, classs)) {
                return returnVal;
            }
        }
        return nullptr;
    }

    // walk the tree depth first and replace any function expressions
    // with a value expression of their results
    ExpressionRef MuScriptInterpreter::consolidated(ExpressionRef exp, ScopeRef scope, Class *classs) {
        switch (exp->type) {
            case ExpressionType::DefineVar: {
                auto &def = get<DefineVar>(exp->expression);
                auto &varr = scope->variables[def.name];
                if (def.defineExpression) {
                    auto val = getValue(def.defineExpression, scope, classs);
                    varr = make_shared<Value>(val->value);
                } else {
                    varr = make_shared<Value>();
                }
                return make_shared<Expression>(varr, ExpressionType::Value);
            } break;
            case ExpressionType::ResolveVar:
                return make_shared<Expression>(resolveVariable(get<ResolveVar>(exp->expression).name, scope), ExpressionType::Value);
            case ExpressionType::MemberVariable: {
                auto &expr = get<MemberVariable>(exp->expression);
                auto classToUse = expr.object ? getValue(expr.object, scope, classs)->getClass().get() : classs;
                return make_shared<Expression>(resolveVariable(expr.name, classToUse, scope), ExpressionType::Value);
            }
            case ExpressionType::MemberFunctionCall: {
                auto &expr = get<MemberFunctionCall>(exp->expression);
                List args;
                for (auto &&sub: expr.subexpressions) {
                    args.push_back(getValue(sub, scope, classs));
                }
                auto val = getValue(expr.object, scope, classs);
                if (val->getType() != Type::Class) {
                    args.insert(args.begin(), val);
                    return make_shared<Expression>(callFunction(resolveVariable(expr.functionName, scope)->getFunction(), scope, args, classs), ExpressionType::Value);
                }
                auto owningClass = val->getClass();
                return make_shared<Expression>(callFunction(resolveFunction(expr.functionName, owningClass.get(), scope), scope, args, owningClass), ExpressionType::Value);
            }
            case ExpressionType::Return:
                return make_shared<Expression>(getValue(get<Return>(exp->expression).expression, scope, classs), ExpressionType::Value);
                break;
            case ExpressionType::FunctionCall: {
                List args;
                auto &funcExpr = get<FunctionExpression>(exp->expression);

                for (auto &&sub: funcExpr.subexpressions) {
                    args.push_back(getValue(sub, scope, classs));
                }

                if (funcExpr.function->getType() == Type::String) {
                    funcExpr.function = resolveVariable(funcExpr.function->getString(), scope);
                }
                return make_shared<Expression>(callFunction(funcExpr.function->getFunction(), scope, args, classs), ExpressionType::Value);
            } break;
            case ExpressionType::Loop: {
                scope = newScope("loop", scope);
                auto &loopexp = get<Loop>(exp->expression);
                if (loopexp.initExpression) {
                    getValue(loopexp.initExpression, scope, classs);
                }
                ValueRef returnVal = nullptr;
                while (returnVal == nullptr && getValue(loopexp.testExpression, scope, classs)->getBool()) {
                    returnVal = needsToReturn(loopexp.subexpressions, scope, classs);
                    if (returnVal == nullptr && loopexp.iterateExpression) {
                        getValue(loopexp.iterateExpression, scope, classs);
                    }
                }
                closeScope(scope);
                if (returnVal) {
                    return make_shared<Expression>(returnVal, ExpressionType::Return);
                } else {
                    return make_shared<Expression>(make_shared<Value>(), ExpressionType::Value);
                }
            } break;
            case ExpressionType::ForEach: {
                scope = newScope("loop", scope);
                auto varr = resolveVariable(get<Foreach>(exp->expression).iterateName, scope);
                auto list = getValue(get<Foreach>(exp->expression).listExpression, scope, classs);
                auto &subs = get<Foreach>(exp->expression).subexpressions;
                ValueRef returnVal = nullptr;
                if (list->getType() == Type::List) {
                    for (auto &&in: list->getList()) {
                        *varr = *in;
                        returnVal = needsToReturn(subs, scope, classs);
                    }
                } else if (list->getType() == Type::Array) {
                    auto &arr = list->getArray();
                    switch (arr.getType()) {
                        case Type::Int: {
                            auto vec = list->getStdVector<Int>();
                            for (auto &&in: vec) {
                                *varr = Value(in);
                                returnVal = needsToReturn(subs, scope, classs);
                            }
                        } break;
                        case Type::Float: {
                            auto vec = list->getStdVector<Float>();
                            for (auto &&in: vec) {
                                *varr = Value(in);
                                returnVal = needsToReturn(subs, scope, classs);
                            }
                        } break;
                        case Type::Vec3: {
                            auto vec = list->getStdVector<vec3>();
                            for (auto &&in: vec) {
                                *varr = Value(in);
                                returnVal = needsToReturn(subs, scope, classs);
                            }
                        } break;
                        case Type::String: {
                            auto vec = list->getStdVector<string>();
                            for (auto &&in: vec) {
                                *varr = Value(in);
                                returnVal = needsToReturn(subs, scope, classs);
                            }
                        } break;
                        default:
                            break;
                    }
                }
                closeScope(scope);
                if (returnVal) {
                    return make_shared<Expression>(returnVal, ExpressionType::Return);
                } else {
                    return make_shared<Expression>(make_shared<Value>(), ExpressionType::Value);
                }
            } break;
            case ExpressionType::IfElse: {
                ValueRef returnVal = nullptr;
                for (auto &express: get<IfElse>(exp->expression)) {
                    if (!express.testExpression || getValue(express.testExpression, scope, classs)->getBool()) {
                        scope = newScope("ifelse", scope);
                        returnVal = needsToReturn(express.subexpressions, scope, classs);
                        closeScope(scope);
                        break;
                    }
                }
                if (returnVal) {
                    return make_shared<Expression>(returnVal, ExpressionType::Return);
                } else {
                    return make_shared<Expression>(make_shared<Value>(), ExpressionType::Value);
                }
            } break;
            default:
                break;
        }
        return make_shared<Expression>(get<ValueRef>(exp->expression), ExpressionType::Value);
    }

    // evaluate an expression from tokens
    ValueRef MuScriptInterpreter::getValue(const vector<string_view> &strings, ScopeRef scope, Class *classs) {
        return getValue(getExpression(strings, scope, classs), scope, classs);
    }

    // evaluate an expression from expressionRef
    ValueRef MuScriptInterpreter::getValue(ExpressionRef exp, ScopeRef scope, Class *classs) {
        // copy the expression so that we don't lose it when we consolidate
        return get<ValueRef>(consolidated(exp, scope, classs)->expression);
    }

    // since the 'else' block in  an if/elfe is technically in a different scope
    // ifelse espressions are not closed immediately and instead left dangling
    // until the next expression is anything other than an 'else' or the else is unconditional
    void MuScriptInterpreter::closeDanglingIfExpression() {
        if (currentExpression && currentExpression->type == ExpressionType::IfElse) {
            if (currentExpression->parent) {
                currentExpression = currentExpression->parent;
            } else {
                getValue(currentExpression, parseScope, nullptr);
                currentExpression = nullptr;
            }
        }
    }

    bool MuScriptInterpreter::closeCurrentExpression() {
        if (currentExpression) {
            if (currentExpression->type != ExpressionType::IfElse) {
                if (currentExpression->parent) {
                    currentExpression = currentExpression->parent;
                } else {
                    if (currentExpression->type != ExpressionType::FunctionDef) {
                        getValue(currentExpression, parseScope, nullptr);
                    }
                    currentExpression = nullptr;
                }
                return true;
            }
        }
        return false;
    }
}// namespace MuScript

// --------------------------------------- expressionImplementation


// --------------------------------------- functionImplementation


namespace MuScript {
    ValueRef MuScriptInterpreter::callFunction(const string &name, ScopeRef scope, const List &args) {
        return callFunction(resolveFunction(name, scope), scope, args);
    }

    void each(ExpressionRef collection, function<void(ExpressionRef)> func) {
        func(collection);
        for (auto &&ex: *collection) {
            each(ex, func);
        }
    }

    ValueRef MuScriptInterpreter::callFunction(FunctionRef fnc, ScopeRef scope, const List &args, Class *classs) {
        switch (fnc->getBodyType()) {
            case FunctionBodyType::Subexpressions: {
                auto &subexpressions = get<vector<ExpressionRef>>(fnc->body);
                // get function scope
                scope = fnc->type == FunctionType::constructor ? resolveScope(fnc->name, scope) : newScope(fnc->name, scope);
                auto limit = min(args.size(), fnc->argNames.size());
                vector<string> newVars;
                for (size_t i = 0; i < limit; ++i) {
                    auto &ref = scope->variables[fnc->argNames[i]];
                    if (ref == nullptr) {
                        newVars.push_back(fnc->argNames[i]);
                    }
                    ref = args[i];
                }

                ValueRef returnVal = nullptr;

                if (fnc->type == FunctionType::constructor) {
                    returnVal = make_shared<Value>(make_shared<Class>(scope));
                    for (auto &&sub: subexpressions) {
                        getValue(sub, scope, returnVal->getClass().get());
                    }
                } else {
                    for (auto &&sub: subexpressions) {
                        if (sub->type == ExpressionType::Return) {
                            returnVal = getValue(sub, scope, classs);
                            break;
                        } else {
                            auto result = consolidated(sub, scope, classs);
                            if (result->type == ExpressionType::Return) {
                                returnVal = get<ValueRef>(result->expression);
                                break;
                            }
                        }
                    }
                }

                if (fnc->type == FunctionType::constructor) {
                    for (auto &&vr: newVars) {
                        scope->variables.erase(vr);
                        returnVal->getClass()->variables.erase(vr);
                    }
                }

                closeScope(scope);
                return returnVal ? returnVal : make_shared<Value>();
            }
            case FunctionBodyType::Lambda: {
                scope = newScope(fnc->name, scope);
                auto returnVal = get<Lambda>(fnc->body)(args);
                closeScope(scope);
                return returnVal ? returnVal : make_shared<Value>();
            }
            case FunctionBodyType::ScopedLambda: {
                scope = newScope(fnc->name, scope);
                auto returnVal = get<ScopedLambda>(fnc->body)(scope, args);
                closeScope(scope);
                return returnVal ? returnVal : make_shared<Value>();
            }
            case FunctionBodyType::ClassLambda: {
                scope = resolveScope(fnc->name, scope);
                if (fnc->type == FunctionType::constructor) {
                    auto returnVal = make_shared<Value>(make_shared<Class>(scope));
                    get<ClassLambda>(fnc->body)(returnVal->getClass().get(), scope, args);
                    closeScope(scope);
                    return returnVal;
                } else if (fnc->type == FunctionType::free && args.size() >= 2 && args[1]->getType() == Type::Class) {
                    // apply function
                    classs = args[1]->getClass().get();
                }
                auto ret = get<ClassLambda>(fnc->body)(classs, scope, args);
                closeScope(scope);
                return ret;
            }
        }

        //empty func
        return make_shared<Value>();
    }

    FunctionRef MuScriptInterpreter::newFunction(const string &name, ScopeRef scope, FunctionRef func) {
        auto &ref = scope->functions[name];
        ref = func;
        if (ref->type == FunctionType::free && scope->isClassScope) {
            ref->type = FunctionType::member;
        }
        auto funcvar = resolveVariable(name, scope);
        funcvar->value = ref;
        return ref;
    }

    FunctionRef MuScriptInterpreter::newFunction(
            const string &name,
            ScopeRef scope,
            const vector<string> &argNames) {
        return newFunction(name, scope, make_shared<Function>(name, argNames));
    }

    FunctionRef MuScriptInterpreter::newConstructor(const string &name, ScopeRef scope, FunctionRef func) {
        auto &ref = scope->functions[name];
        ref = func;
        ref->type = FunctionType::constructor;
        auto funcvar = resolveVariable(name, scope);
        funcvar->value = ref;
        return ref;
    }

    FunctionRef MuScriptInterpreter::newConstructor(
            const string &name,
            ScopeRef scope,
            const vector<string> &argNames) {
        return newConstructor(name, scope, make_shared<Function>(name, argNames));
    }

    FunctionRef MuScriptInterpreter::newClass(const string &name, ScopeRef scope, const unordered_map<string, ValueRef> &variables, const ClassLambda &constructor, const unordered_map<string, ClassLambda> &functions) {
        scope = newClassScope(name, scope);

        scope->variables = variables;
        FunctionRef ret = newConstructor(name, scope->parent, make_shared<Function>(name, constructor));

        for (auto &func: functions) {
            newFunction(func.first, scope, func.second);
        }

        closeScope(scope);

        return ret;
    }

    // name resolution for variables
    ValueRef &MuScriptInterpreter::resolveVariable(const string &name, ScopeRef scope) {
        auto initialScope = scope;
        while (scope) {
            auto iter = scope->variables.find(name);
            if (iter != scope->variables.end()) {
                return iter->second;
            } else {
                scope = scope->parent;
            }
        }
        if (!scope) {
            for (auto m: modules) {
                auto iter = m.scope->variables.find(name);
                if (iter != m.scope->variables.end()) {
                    return iter->second;
                }
            }
        }
        return initialScope->insertVar(name, make_shared<Value>());
    }

    ValueRef &MuScriptInterpreter::resolveVariable(const string &name, Class *classs, ScopeRef scope) {
        auto iter = classs->variables.find(name);
        if (iter != classs->variables.end()) {
            return iter->second;
        }
        return resolveVariable(name, scope);
    }

    FunctionRef MuScriptInterpreter::resolveFunction(const string &name, Class *classs, ScopeRef scope) {
        auto iter = classs->functionScope->functions.find(name);
        if (iter != classs->functionScope->functions.end()) {
            return iter->second;
        }
        return resolveFunction(name, scope);
    }

    // name lookup for callfunction api method
    FunctionRef MuScriptInterpreter::resolveFunction(const string &name, ScopeRef scope) {
        auto initialScope = scope;
        while (scope) {
            auto iter = scope->functions.find(name);
            if (iter != scope->functions.end()) {
                return iter->second;
            } else {
                scope = scope->parent;
            }
        }
        auto &func = initialScope->functions[name];
        func = make_shared<Function>(name);
        return func;
    }

    ScopeRef MuScriptInterpreter::resolveScope(const string &name, ScopeRef scope) {
        auto initialScope = scope;
        while (scope) {
            auto iter = scope->scopes.find(name);
            if (iter != scope->scopes.end()) {
                return iter->second;
            } else {
                if (!scope->parent) {
                    if (scope->name == name) {
                        return scope;
                    }
                }
                scope = scope->parent;
            }
        }
        return initialScope->insertScope(make_shared<Scope>(name, initialScope));
    }
}// namespace MuScript

// --------------------------------------- functionImplementation


// --------------------------------------- modulesImplementation

namespace MuScript {
    ScopeRef MuScriptInterpreter::newModule(const string &name, ModulePrivilegeFlags flags, const unordered_map<string, Lambda> &functions) {
        auto &modSource = flags ? optionalModules : modules;
        modSource.emplace_back(flags, make_shared<Scope>(name, this));
        auto scope = modSource.back().scope;

        for (auto &funcPair: functions) {
            newFunction(funcPair.first, scope, funcPair.second);
        }

        return modSource.back().scope;
    }

    Module *MuScriptInterpreter::getOptionalModule(const string &name) {
        auto iter = std::find_if(optionalModules.begin(), optionalModules.end(), [&name](const auto &mod) { return mod.scope->name == name; });
        if (iter != optionalModules.end()) {
            return &*iter;
        }
        return nullptr;
    }


    // Example functions

    struct surprise
    {
    };

    void exception_test() {
        std::cout << "This function will throw an exception!" << std::endl;
        throw surprise();
    }

    void types_test(int i, float f, std::string d) {
        std::cout << "int: " << i << ", float: " << f << ", string: " << d << std::endl;
    }


    // Example variable

    int some_variable = 1337;

    // Example use case

    struct sample_struct
    {
        double vec_sum(std::vector<double> v) {
            double s = 0;
            for (double n: v)
                s += n;
            return s;
        }
    };


    void MuScriptInterpreter::createStandardLibrary() {

        // register compiled functions and standard library:
        newModule(
                "StandardLib"s,
                0,
                {
                        // math operators
                        {"=", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("=");
                             }
                             if (args.size() == 1) {
                                 return args[0];
                             }
                             *args[0] = *args[1];
                             return args[0];
                         }},

                        {"+", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("+");
                             }
                             if (args.size() == 1) {
                                 return args[0];
                             }
                             return make_shared<Value>(*args[0] + *args[1]);
                         }},

                        {"-", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("-");
                             }
                             if (args.size() == 1) {
                                 auto zero = Value(Int(0));
                                 upconvert(*args[0], zero);
                                 return make_shared<Value>(zero - *args[0]);
                             }
                             return make_shared<Value>(*args[0] - *args[1]);
                         }},

                        {"*", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("*");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>();
                             }
                             return make_shared<Value>(*args[0] * *args[1]);
                         }},

                        {"/", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("/");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>();
                             }
                             return make_shared<Value>(*args[0] / *args[1]);
                         }},

                        {"%", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("%");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>();
                             }
                             return make_shared<Value>(*args[0] % *args[1]);
                         }},

                        {"==", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("==");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>(Int(0));
                             }
                             return make_shared<Value>((Int) (*args[0] == *args[1]));
                         }},

                        {"!=", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("!=");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>(Int(0));
                             }
                             return make_shared<Value>((Int) (*args[0] != *args[1]));
                         }},

                        {"||", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("||");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>(Int(1));
                             }
                             return make_shared<Value>((Int) (*args[0] || *args[1]));
                         }},

                        {"&&", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("&&");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>(Int(0));
                             }
                             return make_shared<Value>((Int) (*args[0] && *args[1]));
                         }},

                        {"++", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             auto i = args.size() - 1;
                             if (i) {
                                 // prefix
                                 *args[i] += Value(Int(1));
                                 return args[i];
                             } else {
                                 // postfix
                                 auto val = make_shared<Value>(args[i]->value);
                                 *args[i] += Value(Int(1));
                                 return val;
                             }
                         }},

                        {"--", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             auto i = args.size() - 1;
                             if (i) {
                                 // prefix
                                 *args[i] -= Value(Int(1));
                                 return args[i];
                             } else {
                                 // postfix
                                 auto val = make_shared<Value>(args[i]->value);
                                 *args[i] -= Value(Int(1));
                                 return val;
                             }
                         }},

                        {"+=", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             if (args.size() == 1) {
                                 return args[0];
                             }
                             *args[0] += *args[1];
                             return args[0];
                         }},

                        {"-=", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             if (args.size() == 1) {
                                 return args[0];
                             }
                             *args[0] -= *args[1];
                             return args[0];
                         }},

                        {"*=", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             if (args.size() == 1) {
                                 return args[0];
                             }
                             *args[0] *= *args[1];
                             return args[0];
                         }},

                        {"/=", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             if (args.size() == 1) {
                                 return args[0];
                             }
                             *args[0] /= *args[1];
                             return args[0];
                         }},

                        {">", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable(">");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>(Int(0));
                             }
                             return make_shared<Value>((Int) (*args[0] > *args[1]));
                         }},

                        {"<", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("<");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>(Int(0));
                             }
                             return make_shared<Value>((Int) (*args[0] < *args[1]));
                         }},

                        {">=", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable(">=");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>(Int(0));
                             }
                             return make_shared<Value>((Int) (*args[0] >= *args[1]));
                         }},

                        {"<=", [this](const List &args) {
                             if (args.size() == 0) {
                                 return resolveVariable("<=");
                             }
                             if (args.size() < 2) {
                                 return make_shared<Value>(Int(0));
                             }
                             return make_shared<Value>((Int) (*args[0] <= *args[1]));
                         }},

                        {"!", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(Int(0));
                             }
                             if (args.size() == 1) {
                                 if (args[0]->getType() != Type::Int) return make_shared<Value>();
                                 auto val = Int(1);
                                 for (auto i = Int(1); i <= args[0]->getInt(); ++i) {
                                     val *= i;
                                 }
                                 return make_shared<Value>(val);
                             }
                             if (args.size() == 2) {
                                 return make_shared<Value>((Int) (!args[1]->getBool()));
                             }
                             return make_shared<Value>();
                         }},

                        // aliases
                        {"identity", [](List args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             return args[0];
                         }},

                        {"copy", [](List args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             if (args[0]->getType() == Type::Class) {
                                 return make_shared<Value>(make_shared<Class>(*args[0]->getClass()));
                             }
                             return make_shared<Value>(args[0]->value);
                         }},

                        {"listindex", [](List args) {
                             if (args.size() > 0) {
                                 if (args.size() == 1) {
                                     return args[0];
                                 }

                                 auto var = args[0];
                                 if (args[1]->getType() != Type::Int) {
                                     var->upconvert(Type::Dictionary);
                                 }

                                 switch (var->getType()) {
                                     case Type::Array: {
                                         auto ival = args[1]->getInt();
                                         auto &arr = var->getArray();
                                         if (ival < 0 || ival >= (Int) arr.size()) {
                                             throw Exception("Out of bounds array access index "s + std::to_string(ival) + ", array length " + std::to_string(arr.size()));
                                         } else {
                                             switch (arr.getType()) {
                                                 case Type::Int:
                                                     return make_shared<Value>(get<vector<Int>>(arr.value)[ival]);
                                                     break;
                                                 case Type::Float:
                                                     return make_shared<Value>(get<vector<Float>>(arr.value)[ival]);
                                                     break;
                                                 case Type::Vec3:
                                                     return make_shared<Value>(get<vector<vec3>>(arr.value)[ival]);
                                                     break;
                                                 case Type::String:
                                                     return make_shared<Value>(get<vector<string>>(arr.value)[ival]);
                                                     break;
                                                 default:
                                                     throw Exception("Attempting to access array of illegal type");
                                                     break;
                                             }
                                         }
                                     } break;
                                     default:
                                         var = make_shared<Value>(var->value);
                                         var->upconvert(Type::List);
                                         [[fallthrough]];
                                     case Type::List: {
                                         auto ival = args[1]->getInt();

                                         auto &list = var->getList();
                                         if (ival < 0 || ival >= (Int) list.size()) {
                                             throw Exception("Out of bounds list access index "s + std::to_string(ival) + ", list length " + std::to_string(list.size()));
                                         } else {
                                             return list[ival];
                                         }
                                     } break;
                                     case Type::Class: {
                                         auto strval = args[1]->getString();
                                         auto &struc = var->getClass();
                                         auto iter = struc->variables.find(strval);
                                         if (iter == struc->variables.end()) {
                                             throw Exception("Class `"s + struc->name + "` does not contain member `" + strval + "`");
                                         } else {
                                             return iter->second;
                                         }
                                     } break;
                                     case Type::Dictionary: {
                                         auto &dict = var->getDictionary();
                                         auto &ref = (*dict)[args[1]->getHash()];
                                         if (ref == nullptr) {
                                             ref = make_shared<Value>();
                                         }
                                         return ref;
                                     } break;
                                 }
                             }
                             return make_shared<Value>();
                         }},
                        // casting
                        {"bool", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(Int(0));
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::Int);
                             val.value = (Int) args[0]->getBool();
                             return make_shared<Value>(val);
                         }},

                        {"int", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(Int(0));
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::Int);
                             return make_shared<Value>(val);
                         }},

                        {"float", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(Float(0.0));
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::Float);
                             return make_shared<Value>(val);
                         }},

                        {"vec3", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(vec3());
                             }
                             if (args.size() < 3) {
                                 auto val = *args[0];
                                 val.hardconvert(Type::Float);
                                 return make_shared<Value>(vec3((float) val.getFloat()));
                             }
                             auto x = *args[0];
                             x.hardconvert(Type::Float);
                             auto y = *args[1];
                             y.hardconvert(Type::Float);
                             auto z = *args[2];
                             z.hardconvert(Type::Float);
                             return make_shared<Value>(vec3((float) x.getFloat(), (float) y.getFloat(), (float) z.getFloat()));
                         }},

                        {"string", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(""s);
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::String);
                             return make_shared<Value>(val);
                         }},

                        {"array", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(Array());
                             }
                             auto list = make_shared<Value>(args);
                             list->hardconvert(Type::Array);
                             return list;
                         }},

                        {"list", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(List());
                             }
                             return make_shared<Value>(args);
                         }},

                        {"dictionary", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(make_shared<Dictionary>());
                             }
                             if (args.size() == 1) {
                                 auto val = *args[0];
                                 val.hardconvert(Type::Dictionary);
                                 return make_shared<Value>(val);
                             }
                             auto dict = make_shared<Value>(make_shared<Dictionary>());
                             for (auto &&arg: args) {
                                 auto val = *arg;
                                 val.hardconvert(Type::Dictionary);
                                 dict->getDictionary()->merge(*val.getDictionary());
                             }
                             return dict;
                         }},

                        {"toarray", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(Array());
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::Array);
                             return make_shared<Value>(val);
                         }},

                        {"tolist", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>(List());
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::List);
                             return make_shared<Value>(val);
                         }},

                        // runtime inspect
                        {"inspect", [](List args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             return make_shared<Value>(args[0]);
                         }},

                        // overal stdlib
                        {"typeof", [](List args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             return make_shared<Value>(getTypeName(args[0]->getType()));
                         }},

                        {"sqrt", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::Float);
                             return make_shared<Value>(sqrt(val.getFloat()));
                         }},

                        {"sin", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::Float);
                             return make_shared<Value>(sin(val.getFloat()));
                         }},

                        {"cos", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::Float);
                             return make_shared<Value>(cos(val.getFloat()));
                         }},

                        {"tan", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::Float);
                             return make_shared<Value>(tan(val.getFloat()));
                         }},

                        {"pow", [](const List &args) {
                             if (args.size() < 2) {
                                 return make_shared<Value>(Float(0));
                             }
                             auto val = *args[0];
                             val.hardconvert(Type::Float);
                             auto val2 = *args[1];
                             val2.hardconvert(Type::Float);
                             return make_shared<Value>(pow(val.getFloat(), val2.getFloat()));
                         }},

                        {"abs", [](const List &args) {
                             if (args.size() == 0) {
                                 return make_shared<Value>();
                             }
                             switch (args[0]->getType()) {
                                 case Type::Int:
                                     return make_shared<Value>(Int(abs(args[0]->getInt())));
                                     break;
                                 case Type::Float:
                                     return make_shared<Value>(Float(fabs(args[0]->getFloat())));
                                     break;
                                 default:
                                     return make_shared<Value>();
                                     break;
                             }
                         }},

                        {"min", [](const List &args) {
                             if (args.size() < 2) {
                                 return make_shared<Value>();
                             }
                             auto val = *args[0];
                             auto val2 = *args[1];
                             upconvertThrowOnNonNumberToNumberCompare(val, val2);
                             if (val > val2) {
                                 return make_shared<Value>(val2.value);
                             }
                             return make_shared<Value>(val.value);
                         }},

                        {"max", [](const List &args) {
                             if (args.size() < 2) {
                                 return make_shared<Value>();
                             }
                             auto val = *args[0];
                             auto val2 = *args[1];
                             upconvertThrowOnNonNumberToNumberCompare(val, val2);
                             if (val < val2) {
                                 return make_shared<Value>(val2.value);
                             }
                             return make_shared<Value>(val.value);
                         }},

                        {"swap", [](const List &args) {
                             if (args.size() < 2) {
                                 return make_shared<Value>();
                             }
                             auto v = *args[0];
                             *args[0] = *args[1];
                             *args[1] = v;

                             return make_shared<Value>();
                         }},

                        {"print", [](const List &args) {
                             auto s = std::string{"[MU] "};
                             for (auto &&arg: args) {
                                 s += arg->getPrintString();
                             }
                             METADOT_INFO("{0}", s);
                             return make_shared<Value>();
                         }},

                        {"getline", [](const List &args) {
                             string s;
                             // blocking calls are fine
                             getline(std::cin, s);
                             if (args.size() > 0) {
                                 args[0]->value = s;
                             }
                             return make_shared<Value>(s);
                         }},

                        {"map", [this](const List &args) {
                             if (args.size() < 2 || args[1]->getType() != Type::Function) {
                                 return make_shared<Value>();
                             }
                             auto ret = make_shared<Value>(List());
                             auto &retList = ret->getList();
                             auto func = args[1]->getFunction();

                             if (args[0]->getType() == Type::Array) {
                                 auto val = *args[0];
                                 val.upconvert(Type::List);
                                 for (auto &&v: val.getList()) {
                                     retList.push_back(callFunction(func, {v}));
                                 }
                                 return ret;
                             }

                             for (auto &&v: args[0]->getList()) {
                                 retList.push_back(callFunction(func, {v}));
                             }
                             return ret;
                         }},

                        {"fold", [this](const List &args) {
                             if (args.size() < 3 || args[1]->getType() != Type::Function) {
                                 return make_shared<Value>();
                             }

                             auto func = args[1]->getFunction();
                             auto iter = args[2];

                             if (args[0]->getType() == Type::Array) {
                                 auto val = *args[0];
                                 val.upconvert(Type::List);
                                 for (auto &&v: val.getList()) {
                                     iter = callFunction(func, {iter, v});
                                 }
                                 return iter;
                             }

                             for (auto &&v: args[0]->getList()) {
                                 iter = callFunction(func, {iter, v});
                             }
                             return iter;
                         }},

                        {"clock", [](const List &) {
                             return make_shared<Value>(Int(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
                         }},

                        {"getduration", [](const List &args) {
                             if (args.size() == 2 && args[0]->getType() == Type::Int && args[1]->getType() == Type::Int) {
                                 std::chrono::duration<double> duration = std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(args[1]->getInt())) -
                                                                          std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(args[0]->getInt()));
                                 return make_shared<Value>(Float(duration.count()));
                             }
                             return make_shared<Value>();
                         }},

                        {"timesince", [](const List &args) {
                             if (args.size() == 1 && args[0]->getType() == Type::Int) {
                                 std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() -
                                                                          std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(args[0]->getInt()));
                                 return make_shared<Value>(Float(duration.count()));
                             }
                             return make_shared<Value>();
                         }},

                        // collection functions
                        {"length", [](const List &args) {
                             if (args.size() == 0 || (int) args[0]->getType() < (int) Type::String) {
                                 return make_shared<Value>(Int(0));
                             }
                             if (args[0]->getType() == Type::String) {
                                 return make_shared<Value>((Int) args[0]->getString().size());
                             }
                             if (args[0]->getType() == Type::Array) {
                                 return make_shared<Value>((Int) args[0]->getArray().size());
                             }
                             return make_shared<Value>((Int) args[0]->getList().size());
                         }},

                        {"find", [](const List &args) {
                             if (args.size() < 2 || (int) args[0]->getType() < (int) Type::Array) {
                                 return make_shared<Value>();
                             }
                             if (args[0]->getType() == Type::Array) {
                                 if (args[1]->getType() == args[0]->getArray().getType()) {
                                     switch (args[0]->getArray().getType()) {
                                         case Type::Int: {
                                             auto &arry = args[0]->getStdVector<Int>();
                                             auto iter = find(arry.begin(), arry.end(), args[1]->getInt());
                                             if (iter == arry.end()) {
                                                 return make_shared<Value>();
                                             }
                                             return make_shared<Value>((Int) (iter - arry.begin()));
                                         } break;
                                         case Type::Float: {
                                             auto &arry = args[0]->getStdVector<Float>();
                                             auto iter = find(arry.begin(), arry.end(), args[1]->getFloat());
                                             if (iter == arry.end()) {
                                                 return make_shared<Value>();
                                             }
                                             return make_shared<Value>((Int) (iter - arry.begin()));
                                         } break;
                                         case Type::Vec3: {
                                             auto &arry = args[0]->getStdVector<vec3>();
                                             auto iter = find(arry.begin(), arry.end(), args[1]->getVec3());
                                             if (iter == arry.end()) {
                                                 return make_shared<Value>();
                                             }
                                             return make_shared<Value>((Int) (iter - arry.begin()));
                                         } break;
                                         case Type::String: {
                                             auto &arry = args[0]->getStdVector<string>();
                                             auto iter = find(arry.begin(), arry.end(), args[1]->getString());
                                             if (iter == arry.end()) {
                                                 return make_shared<Value>();
                                             }
                                             return make_shared<Value>((Int) (iter - arry.begin()));
                                         } break;
                                         case Type::Function: {
                                             auto &arry = args[0]->getStdVector<FunctionRef>();
                                             auto iter = find(arry.begin(), arry.end(), args[1]->getFunction());
                                             if (iter == arry.end()) {
                                                 return make_shared<Value>();
                                             }
                                             return make_shared<Value>((Int) (iter - arry.begin()));
                                         } break;
                                         default:
                                             break;
                                     }
                                 }
                                 return make_shared<Value>();
                             }
                             auto &list = args[0]->getList();
                             for (size_t i = 0; i < list.size(); ++i) {
                                 if (*list[i] == *args[1]) {
                                     return make_shared<Value>((Int) i);
                                 }
                             }
                             return make_shared<Value>();
                         }},

                        {"erase", [](const List &args) {
                             if (args.size() < 2 || (int) args[0]->getType() < (int) Type::Array || args[1]->getType() != Type::Int) {
                                 return make_shared<Value>();
                             }

                             if (args[0]->getType() == Type::Array) {
                                 switch (args[0]->getArray().getType()) {
                                     case Type::Int:
                                         args[0]->getStdVector<Int>().erase(args[0]->getStdVector<Int>().begin() + args[1]->getInt());
                                         break;
                                     case Type::Float:
                                         args[0]->getStdVector<Float>().erase(args[0]->getStdVector<Float>().begin() + args[1]->getInt());
                                         break;
                                     case Type::Vec3:
                                         args[0]->getStdVector<vec3>().erase(args[0]->getStdVector<vec3>().begin() + args[1]->getInt());
                                         break;
                                     case Type::String:
                                         args[0]->getStdVector<string>().erase(args[0]->getStdVector<string>().begin() + args[1]->getInt());
                                         break;
                                     case Type::Function:
                                         args[0]->getStdVector<FunctionRef>().erase(args[0]->getStdVector<FunctionRef>().begin() + args[1]->getInt());
                                         break;
                                     default:
                                         break;
                                 }
                             } else {
                                 args[0]->getList().erase(args[0]->getList().begin() + args[1]->getInt());
                             }
                             return make_shared<Value>();
                         }},

                        {"pushback", [](const List &args) {
                             if (args.size() < 2 || (int) args[0]->getType() < (int) Type::Array) {
                                 return make_shared<Value>();
                             }

                             if (args[0]->getType() == Type::Array) {
                                 if (args[0]->getArray().getType() == args[1]->getType()) {
                                     switch (args[0]->getArray().getType()) {
                                         case Type::Int:
                                             args[0]->getStdVector<Int>().push_back(args[1]->getInt());
                                             break;
                                         case Type::Float:
                                             args[0]->getStdVector<Float>().push_back(args[1]->getFloat());
                                             break;
                                         case Type::Vec3:
                                             args[0]->getStdVector<vec3>().push_back(args[1]->getVec3());
                                             break;
                                         case Type::String:
                                             args[0]->getStdVector<string>().push_back(args[1]->getString());
                                             break;
                                         case Type::Function:
                                             args[0]->getStdVector<FunctionRef>().push_back(args[1]->getFunction());
                                             break;
                                         default:
                                             break;
                                     }
                                 }
                             } else {
                                 args[0]->getList().push_back(args[1]);
                             }
                             return make_shared<Value>();
                         }},

                        {"popback", [](const List &args) {
                             if (args.size() < 1 || (int) args[0]->getType() < (int) Type::Array) {
                                 return make_shared<Value>();
                             }
                             if (args[0]->getType() == Type::Array) {
                                 args[0]->getArray().pop_back();
                             } else {
                                 args[0]->getList().pop_back();
                             }
                             return make_shared<Value>();
                         }},

                        {"popfront", [](const List &args) {
                             if (args.size() < 1 || (int) args[0]->getType() < (int) Type::Array) {
                                 return make_shared<Value>();
                             }
                             if (args[0]->getType() == Type::Array) {
                                 switch (args[0]->getArray().getType()) {
                                     case Type::Int: {
                                         auto &arry = args[0]->getStdVector<Int>();
                                         arry.erase(arry.begin());
                                     } break;
                                     case Type::Float: {
                                         auto &arry = args[0]->getStdVector<Float>();
                                         arry.erase(arry.begin());
                                     } break;
                                     case Type::Vec3: {
                                         auto &arry = args[0]->getStdVector<vec3>();
                                         arry.erase(arry.begin());
                                     } break;
                                     case Type::Function: {
                                         auto &arry = args[0]->getStdVector<FunctionRef>();
                                         arry.erase(arry.begin());
                                     } break;
                                     case Type::String: {
                                         auto &arry = args[0]->getStdVector<string>();
                                         arry.erase(arry.begin());
                                     } break;

                                     default:
                                         break;
                                 }
                                 return args[0];
                             } else {
                                 auto &list = args[0]->getList();
                                 list.erase(list.begin());
                             }
                             return make_shared<Value>();
                         }},

                        {"front", [](const List &args) {
                             if (args.size() < 1 || (int) args[0]->getType() < (int) Type::Array) {
                                 return make_shared<Value>();
                             }
                             if (args[0]->getType() == Type::Array) {
                                 switch (args[0]->getArray().getType()) {
                                     case Type::Int:
                                         return make_shared<Value>(args[0]->getStdVector<Int>().front());
                                     case Type::Float:
                                         return make_shared<Value>(args[0]->getStdVector<Float>().front());
                                     case Type::Vec3:
                                         return make_shared<Value>(args[0]->getStdVector<vec3>().front());
                                     case Type::Function:
                                         return make_shared<Value>(args[0]->getStdVector<FunctionRef>().front());
                                     case Type::String:
                                         return make_shared<Value>(args[0]->getStdVector<string>().front());
                                     default:
                                         break;
                                 }
                                 return make_shared<Value>();
                             } else {
                                 return args[0]->getList().front();
                             }
                         }},

                        {"back", [](const List &args) {
                             if (args.size() < 1 || (int) args[0]->getType() < (int) Type::Array) {
                                 return make_shared<Value>();
                             }
                             if (args[0]->getType() == Type::Array) {
                                 switch (args[0]->getArray().getType()) {
                                     case Type::Int:
                                         return make_shared<Value>(args[0]->getStdVector<Int>().back());
                                     case Type::Float:
                                         return make_shared<Value>(args[0]->getStdVector<Float>().back());
                                     case Type::Vec3:
                                         return make_shared<Value>(args[0]->getStdVector<vec3>().back());
                                     case Type::Function:
                                         return make_shared<Value>(args[0]->getStdVector<FunctionRef>().back());
                                     case Type::String:
                                         return make_shared<Value>(args[0]->getStdVector<string>().back());
                                     default:
                                         break;
                                 }
                                 return make_shared<Value>();
                             } else {
                                 return args[0]->getList().back();
                             }
                         }},

                        {"reverse", [](const List &args) {
                             if (args.size() < 1 || (int) args[0]->getType() < (int) Type::String) {
                                 return make_shared<Value>();
                             }
                             auto copy = make_shared<Value>(args[0]->value);

                             if (args[0]->getType() == Type::String) {
                                 auto &str = copy->getString();
                                 std::reverse(str.begin(), str.end());
                                 return copy;
                             } else if (args[0]->getType() == Type::Array) {
                                 switch (copy->getArray().getType()) {
                                     case Type::Int: {
                                         auto &vl = copy->getStdVector<Int>();
                                         std::reverse(vl.begin(), vl.end());
                                         return copy;
                                     } break;
                                     case Type::Float: {
                                         auto &vl = copy->getStdVector<Float>();
                                         std::reverse(vl.begin(), vl.end());
                                         return copy;
                                     } break;
                                     case Type::Vec3: {
                                         auto &vl = copy->getStdVector<vec3>();
                                         std::reverse(vl.begin(), vl.end());
                                         return copy;
                                     } break;
                                     case Type::String: {
                                         auto &vl = copy->getStdVector<string>();
                                         std::reverse(vl.begin(), vl.end());
                                         return copy;
                                     } break;
                                     case Type::Function: {
                                         auto &vl = copy->getStdVector<FunctionRef>();
                                         std::reverse(vl.begin(), vl.end());
                                         return copy;
                                     } break;
                                     default:
                                         break;
                                 }
                             } else if (args[0]->getType() == Type::List) {
                                 auto &vl = copy->getList();
                                 std::reverse(vl.begin(), vl.end());
                                 return copy;
                             }
                             return make_shared<Value>();
                         }},

                        {"range", [](const List &args) {
                             if (args.size() == 2 && args[0]->getType() == args[1]->getType()) {
                                 if (args[0]->getType() == Type::Int) {
                                     auto ret = make_shared<Value>(Array(vector<Int>{}));
                                     auto &arry = ret->getStdVector<Int>();
                                     auto a = args[0]->getInt();
                                     auto b = args[1]->getInt();
                                     if (b > a) {
                                         arry.reserve(b - a);
                                         for (Int i = a; i <= b; i++) {
                                             arry.push_back(i);
                                         }
                                     } else {
                                         arry.reserve(a - b);
                                         for (Int i = a; i >= b; i--) {
                                             arry.push_back(i);
                                         }
                                     }
                                     return ret;
                                 } else if (args[0]->getType() == Type::Float) {
                                     auto ret = make_shared<Value>(Array(vector<Float>{}));
                                     auto &arry = ret->getStdVector<Float>();
                                     Float a = args[0]->getFloat();
                                     Float b = args[1]->getFloat();
                                     if (b > a) {
                                         arry.reserve((Int) (b - a));
                                         for (Float i = a; i <= b; i++) {
                                             arry.push_back(i);
                                         }
                                     } else {
                                         arry.reserve((Int) (a - b));
                                         for (Float i = a; i >= b; i--) {
                                             arry.push_back(i);
                                         }
                                     }
                                     return ret;
                                 }
                             }
                             if (args.size() < 3 || (int) args[0]->getType() < (int) Type::String) {
                                 return make_shared<Value>();
                             }
                             auto indexA = *args[1];
                             indexA.hardconvert(Type::Int);
                             auto indexB = *args[2];
                             indexB.hardconvert(Type::Int);
                             auto intdexA = indexA.getInt();
                             auto intdexB = indexB.getInt();

                             if (args[0]->getType() == Type::String) {
                                 return make_shared<Value>(args[0]->getString().substr(intdexA, intdexB));
                             } else if (args[0]->getType() == Type::Array) {
                                 if (args[0]->getArray().getType() == args[1]->getType()) {
                                     switch (args[0]->getArray().getType()) {
                                         case Type::Int:
                                             return make_shared<Value>(Array(vector<Int>(args[0]->getStdVector<Int>().begin() + intdexA, args[0]->getStdVector<Int>().begin() + intdexB)));
                                             break;
                                         case Type::Float:
                                             return make_shared<Value>(Array(vector<Float>(args[0]->getStdVector<Float>().begin() + intdexA, args[0]->getStdVector<Float>().begin() + intdexB)));
                                             break;
                                         case Type::Vec3:
                                             return make_shared<Value>(Array(vector<vec3>(args[0]->getStdVector<vec3>().begin() + intdexA, args[0]->getStdVector<vec3>().begin() + intdexB)));
                                             break;
                                         case Type::String:
                                             return make_shared<Value>(Array(vector<string>(args[0]->getStdVector<string>().begin() + intdexA, args[0]->getStdVector<string>().begin() + intdexB)));
                                             break;
                                         case Type::Function:
                                             return make_shared<Value>(Array(vector<FunctionRef>(args[0]->getStdVector<FunctionRef>().begin() + intdexA, args[0]->getStdVector<FunctionRef>().begin() + intdexB)));
                                             break;
                                         default:
                                             break;
                                     }
                                 }
                             } else {
                                 return make_shared<Value>(List(args[0]->getList().begin() + intdexA, args[0]->getList().begin() + intdexB));
                             }
                             return make_shared<Value>();
                         }},

                        {"replace", [](const List &args) {
                             if (args.size() < 3 || args[0]->getType() != Type::String || args[1]->getType() != Type::String || args[2]->getType() != Type::String) {
                                 return make_shared<Value>();
                             }

                             string &input = args[0]->getString();
                             const string &lookfor = args[1]->getString();
                             const string &replacewith = args[2]->getString();
                             size_t pos = 0;
                             size_t lpos = 0;
                             while ((pos = input.find(lookfor, lpos)) != string::npos) {
                                 input.replace(pos, lookfor.size(), replacewith);
                                 lpos = pos + replacewith.size();
                             }

                             return make_shared<Value>(input);
                         }},

                        {"startswith", [](const List &args) {
                             if (args.size() < 2 || args[0]->getType() != Type::String || args[1]->getType() != Type::String) {
                                 return make_shared<Value>();
                             }
                             return make_shared<Value>(Int(startswith(args[0]->getString(), args[1]->getString())));
                         }},

                        {"endswith", [](const List &args) {
                             if (args.size() < 2 || args[0]->getType() != Type::String || args[1]->getType() != Type::String) {
                                 return make_shared<Value>();
                             }
                             return make_shared<Value>(Int(endswith(args[0]->getString(), args[1]->getString())));
                         }},

                        {"contains", [](const List &args) {
                             if (args.size() < 2 || (int) args[0]->getType() < (int) Type::Array) {
                                 return make_shared<Value>(Int(0));
                             }
                             if (args[0]->getType() == Type::Array) {
                                 auto item = *args[1];
                                 switch (args[0]->getArray().getType()) {
                                     case Type::Int:
                                         item.hardconvert(Type::Int);
                                         return make_shared<Value>((Int) contains(args[0]->getStdVector<Int>(), item.getInt()));
                                     case Type::Float:
                                         item.hardconvert(Type::Float);
                                         return make_shared<Value>((Int) contains(args[0]->getStdVector<Float>(), item.getFloat()));
                                     case Type::Vec3:
                                         item.hardconvert(Type::Vec3);
                                         return make_shared<Value>((Int) contains(args[0]->getStdVector<vec3>(), item.getVec3()));
                                     case Type::String:
                                         item.hardconvert(Type::String);
                                         return make_shared<Value>((Int) contains(args[0]->getStdVector<string>(), item.getString()));
                                     default:
                                         break;
                                 }
                                 return make_shared<Value>(Int(0));
                             }
                             auto &list = args[0]->getList();
                             for (size_t i = 0; i < list.size(); ++i) {
                                 if (*list[i] == *args[1]) {
                                     return make_shared<Value>(Int(1));
                                 }
                             }
                             return make_shared<Value>(Int(0));
                         }},

                        {"split", [](const List &args) {
                             if (args.size() > 0 && args[0]->getType() == Type::String) {
                                 if (args.size() == 1) {
                                     vector<string> chars;
                                     for (auto c: args[0]->getString()) {
                                         chars.push_back(""s + c);
                                     }
                                     return make_shared<Value>(Array(chars));
                                 }
                                 return make_shared<Value>(Array(split(args[0]->getString(), args[1]->getPrintString())));
                             }
                             return make_shared<Value>();
                         }},

                        {"sort", [](const List &args) {
                             if (args.size() < 1 || (int) args[0]->getType() < (int) Type::Array) {
                                 return make_shared<Value>();
                             }
                             if (args[0]->getType() == Type::Array) {
                                 switch (args[0]->getArray().getType()) {
                                     case Type::Int: {
                                         auto &arry = args[0]->getStdVector<Int>();
                                         std::sort(arry.begin(), arry.end());
                                     } break;
                                     case Type::Float: {
                                         auto &arry = args[0]->getStdVector<Float>();
                                         std::sort(arry.begin(), arry.end());
                                     } break;
                                     case Type::String: {
                                         auto &arry = args[0]->getStdVector<string>();
                                         std::sort(arry.begin(), arry.end());
                                     } break;
                                     case Type::Vec3: {
                                         auto &arry = args[0]->getStdVector<vec3>();
                                         std::sort(arry.begin(), arry.end(), [](const vec3 &a, const vec3 &b) { return a.x < b.x; });
                                     } break;
                                     default:
                                         break;
                                 }
                                 return args[0];
                             }
                             auto &list = args[0]->getList();
                             std::sort(list.begin(), list.end(), [](const ValueRef &a, const ValueRef &b) { return *a < *b; });
                             return args[0];
                         }},
                        {"applyfunction", [this](List args) {
                             if (args.size() < 2 || args[1]->getType() != Type::Class) {
                                 auto func = args[0]->getType() == Type::Function ? args[0] : args[0]->getType() == Type::String ? resolveVariable(args[0]->getString())
                                                                                                                                 : throw Exception("Cannot call non existant function: null");
                                 auto list = List();
                                 for (size_t i = 1; i < args.size(); ++i) {
                                     list.push_back(args[i]);
                                 }
                                 return callFunction(func->getFunction(), list);
                             }
                             return make_shared<Value>();
                         }},
                });

        listIndexFunctionVarLocation = resolveVariable("listindex", modules.back().scope);
        identityFunctionVarLocation = resolveVariable("identity", modules.back().scope);
    }
}// namespace MuScript


// --------------------------------------- modulesImplementation


// --------------------------------------- optionalModules


namespace MuScript {
    void MuScriptInterpreter::createOptionalModules() {
        newModule(
                "file",
                ModulePrivilege::fileSystemRead | ModulePrivilege::fileSystemWrite,
                {
                        {"saveFile", [](const List &args) {
                             if (args.size() == 2 && args[0]->getType() == Type::String && args[1]->getType() == Type::String) {
                                 auto t = std::ofstream(args[1]->getString(), std::ofstream::out);
                                 t << args[0]->getString();
                                 t.flush();
                                 return make_shared<Value>(true);
                             }
                             return make_shared<Value>(false);
                         }},
                        {"readFile", [](const List &args) {
                             if (args.size() == 1 && args[0]->getType() == Type::String) {
                                 std::stringstream buffer;
                                 buffer << std::ifstream(args[0]->getString()).rdbuf();
                                 return make_shared<Value>(buffer.str());
                             }
                             return make_shared<Value>();
                         }},
                });

        // currently this kinda works, but the whole scoping system is too state based and it breaks down
        newModule(
                "thread",
                ModulePrivilege::fileSystemRead | ModulePrivilege::experimental,
                {
                        {"newThread", [this](const List &args) {
                             if (args.size() == 1 && args[0]->getType() == Type::Function) {
                                 auto func = args[0]->getFunction();
                                 auto ptr = new std::thread([this, func]() {
                                     callFunction(func, {});
                                 });
                                 return make_shared<Value>(ptr);
                             }
                             return make_shared<Value>();
                         }},
                        {"joinThread", [](const List &args) {
                             if (args.size() == 1 && args[0]->getType() == Type::UserPointer) {
                                 std::thread *ptr = (std::thread *) args[0]->getPointer();
                                 ptr->join();
                                 delete ptr;
                             }
                             return make_shared<Value>();
                         }},
                });
    }
}// namespace MuScript


// --------------------------------------- optionalModules


// --------------------------------------- parsing


namespace MuScript {
    // tokenizer special characters
    const std::string WhitespaceChars = " \t\n"s;
    const string GrammarChars = " \t\n,.(){}[];+-/*%<>=!&|\""s;
    const string MultiCharTokenStartChars = "+-/*<>=!&|"s;
    const string NumericChars = "0123456789."s;
    const string NumericStartChars = "0123456789."s;
    const string DisallowedIdentifierStartChars = "0123456789.- \t\n,.(){}[];+-/*%<>=!&|\""s;

    vector<string_view> ViewTokenize(string_view input) {
        vector<string_view> ret;
        if (input.empty()) return ret;
        bool exitFromComment = false;

        size_t pos = 0;
        size_t lpos = 0;
        while ((pos = input.find_first_of(GrammarChars, lpos)) != string::npos) {
            size_t len = pos - lpos;
            // differentiate between decimals and dot syntax for function calls
            if (input[pos] == '.' && pos + 1 < input.size() && contains(NumericChars, input[pos + 1])) {
                pos = input.find_first_of(GrammarChars, pos + 1);
                ret.push_back(input.substr(lpos, pos - lpos));
                lpos = pos;
                continue;
            }
            if (len) {
                ret.push_back(input.substr(lpos, pos - lpos));
                lpos = pos;
            } else {
                // handle strings and escaped strings
                if (input[pos] == '\"' && pos > 0 && input[pos - 1] != '\\') {
                    auto originalPos = pos;
                    auto testpos = lpos + 1;
                    bool loop = true;
                    while (loop) {
                        pos = input.find('\"', testpos);
                        if (pos == string::npos) {
                            throw Exception("Quote mismatch at "s + string(input.substr(lpos, input.size() - lpos)));
                        }
                        loop = (input[pos - 1] == '\\');
                        testpos = ++pos;
                    }

                    ret.push_back(input.substr(originalPos, pos - originalPos));
                    lpos = pos;
                    continue;
                }
            }
            // special case for negative numbers
            if (input[pos] == '-' && contains(NumericChars, input[pos + 1]) && (ret.size() == 0 || contains(MultiCharTokenStartChars, ret.back().back()))) {
                pos = input.find_first_of(GrammarChars, pos + 1);
                if (input[pos] == '.' && pos + 1 < input.size() && contains(NumericChars, input[pos + 1])) {
                    pos = input.find_first_of(GrammarChars, pos + 1);
                }
                ret.push_back(input.substr(lpos, pos - lpos));
                lpos = pos;
            } else if (!contains(WhitespaceChars, input[pos])) {
                // process multicharacter special tokens like ++, //, -=, etc
                auto stride = 1;
                if (contains(MultiCharTokenStartChars, input[pos]) && pos + 1 < input.size() && contains(MultiCharTokenStartChars, input[pos + 1])) {
                    if (input[pos] == '/' && input[pos + 1] == '/') {
                        exitFromComment = true;
                        break;
                    }
                    ++stride;
                }
                ret.push_back(input.substr(lpos, stride));
                lpos += stride;
            } else {
                ++lpos;
            }
        }
        if (!exitFromComment && lpos < input.length()) {
            ret.push_back(input.substr(lpos, input.length()));
        }
        return ret;
    }

    // functions for figuring out the type of token

    bool isStringLiteral(string_view test) {
        return (test.size() > 1 && test[0] == '\"');
    }

    bool isFloatLiteral(string_view test) {
        return (test.size() > 1 && contains(test, '.'));
    }

    bool isVarOrFuncToken(string_view test) {
        return (test.size() > 0 && !contains(DisallowedIdentifierStartChars, test[0]));
    }

    bool isNumeric(string_view test) {
        if (test.size() > 1 && test[0] == '-') contains(NumericStartChars, test[1]);
        return (test.size() > 0 && contains(NumericStartChars, test[0]));
    }

    bool isMathOperator(string_view test) {
        if (test.size() == 1) {
            return contains("+-*/%<>=!"s, test[0]);
        }
        if (test.size() == 2) {
            return contains("=+-&|"s, test[1]) && contains(MultiCharTokenStartChars, test[0]);
        }
        return false;
    }

    bool isUnaryMathOperator(string_view test) {
        if (test.size() == 1) {
            return '!' == test[0];
        }
        if (test.size() == 2) {
            return contains("+-"s, test[1]) && test[1] == test[0];
        }
        return false;
    }

    bool isMemberCall(string_view test) {
        if (test.size() == 1) {
            return '.' == test[0];
        }
        return false;
    }

    bool isOpeningBracketOrParen(string_view test) {
        return (test.size() == 1 && (test[0] == '[' || test[0] == '('));
    }

    bool isClosingBracketOrParen(string_view test) {
        return (test.size() == 1 && (test[0] == ']' || test[0] == ')'));
    }

    bool needsUnaryPlacementFix(const vector<string_view> &strings, size_t i) {
        return (isUnaryMathOperator(strings[i]) && (i == 0 || !(isClosingBracketOrParen(strings[i - 1]) || isVarOrFuncToken(strings[i - 1]) || isNumeric(strings[i - 1]))));
    }

    bool checkPrecedence(OperatorPrecedence curr, OperatorPrecedence neww) {
        return (int) curr < (int) neww || (neww == curr && neww == OperatorPrecedence::incdec);
    }

    ExpressionRef MuScriptInterpreter::getResolveVarExpression(const string &name, bool classScope) {
        if (classScope) {
            return make_shared<Expression>(nullptr, name);
        } else {
            return make_shared<Expression>(ResolveVar(name));
        }
    }

    // recursively build an expression tree from a list of tokens
    ExpressionRef MuScriptInterpreter::getExpression(const vector<string_view> &strings, ScopeRef scope, Class *classs) {
        ExpressionRef root = nullptr;
        size_t i = 0;
        while (i < strings.size()) {
            if (isMathOperator(strings[i])) {
                auto prev = root;
                root = make_shared<Expression>(resolveVariable(string(strings[i]), modules[0].scope));
                auto curr = prev;
                if (curr) {
                    // find operations of lesser precedence
                    if (curr->type == ExpressionType::FunctionCall) {
                        auto &rootExpression = get<FunctionExpression>(root->expression);
                        auto curfunc = get<FunctionExpression>(curr->expression).function->getFunction();
                        auto newfunc = rootExpression.function->getFunction();
                        if (curfunc && checkPrecedence(curfunc->opPrecedence, newfunc->opPrecedence)) {
                            while (get<FunctionExpression>(curr->expression).subexpressions.back()->type == ExpressionType::FunctionCall) {
                                auto fnc = get<FunctionExpression>(get<FunctionExpression>(curr->expression).subexpressions.back()->expression).function;
                                if (fnc->getType() != Type::Function) {
                                    break;
                                }
                                curfunc = fnc->getFunction();
                                if (curfunc && checkPrecedence(curfunc->opPrecedence, newfunc->opPrecedence)) {
                                    curr = get<FunctionExpression>(curr->expression).subexpressions.back();
                                } else {
                                    break;
                                }
                            }
                            auto &currExpression = get<FunctionExpression>(curr->expression);
                            // swap values around to correct the otherwise incorect order of operations (except unary)
                            if (needsUnaryPlacementFix(strings, i)) {
                                rootExpression.subexpressions.insert(rootExpression.subexpressions.begin(), make_shared<Expression>(make_shared<Value>(), root));
                            } else {
                                rootExpression.subexpressions.push_back(currExpression.subexpressions.back());
                                currExpression.subexpressions.pop_back();
                            }

                            // gather any subexpressions from list literals/indexing or function call args
                            if (i + 1 < strings.size() && !isMathOperator(strings[i + 1])) {
                                vector<string_view> minisub = {strings[++i]};
                                // list literal or parenthesis expression
                                char checkstr = 0;
                                if (isOpeningBracketOrParen(strings[i])) {
                                    checkstr = strings[i][0];
                                }
                                // list index or function call
                                else if (strings.size() > i + 1 && isOpeningBracketOrParen(strings[i + 1])) {
                                    ++i;
                                    checkstr = strings[i][0];
                                    minisub.push_back(strings[i]);
                                }
                                // gather tokens until the end of the sub expression
                                if (checkstr != 0) {
                                    auto endstr = checkstr == '[' ? ']' : ')';
                                    int nestLayers = 1;
                                    while (nestLayers > 0 && ++i < strings.size()) {
                                        if (strings[i].size() == 1) {
                                            if (strings[i][0] == endstr) {
                                                --nestLayers;
                                            } else if (strings[i][0] == checkstr) {
                                                ++nestLayers;
                                            }
                                        }
                                        minisub.push_back(strings[i]);
                                        if (nestLayers == 0) {
                                            if (i + 1 < strings.size() && isOpeningBracketOrParen(strings[i + 1])) {
                                                ++nestLayers;
                                                checkstr = strings[++i][0];
                                                endstr = checkstr == '[' ? ']' : ')';
                                            }
                                        }
                                    }
                                }
                                rootExpression.subexpressions.push_back(getExpression(move(minisub), scope, classs));
                            }
                            currExpression.subexpressions.push_back(root);
                            root = prev;
                        } else {
                            rootExpression.subexpressions.push_back(curr);
                        }
                    } else {
                        get<FunctionExpression>(root->expression).subexpressions.push_back(curr);
                    }
                } else {
                    if (needsUnaryPlacementFix(strings, i)) {
                        auto &rootExpression = get<FunctionExpression>(root->expression);
                        rootExpression.subexpressions.insert(rootExpression.subexpressions.begin(), make_shared<Expression>(make_shared<Value>(), root));
                    }
                }
            } else if (isStringLiteral(strings[i])) {
                // trim quotation marks
                auto val = string(strings[i].substr(1, strings[i].size() - 2));
                replaceWhitespaceLiterals(val);
                auto newExpr = make_shared<Expression>(make_shared<Value>(val), ExpressionType::Value);
                if (root) {
                    get<FunctionExpression>(root->expression).subexpressions.push_back(newExpr);
                } else {
                    root = newExpr;
                }
            } else if (strings[i] == "(" || strings[i] == "[" || isVarOrFuncToken(strings[i])) {
                if (strings[i] == "(" || i + 2 < strings.size() && strings[i + 1] == "(") {
                    // function
                    ExpressionRef cur = nullptr;
                    if (strings[i] == "(") {
                        if (root) {
                            if (root->type == ExpressionType::FunctionCall) {
                                if (get<FunctionExpression>(root->expression).function->getFunction()->opPrecedence == OperatorPrecedence::func) {
                                    cur = make_shared<Expression>(make_shared<Value>());
                                    get<FunctionExpression>(cur->expression).subexpressions.push_back(root);
                                    root = cur;
                                } else {
                                    get<FunctionExpression>(root->expression).subexpressions.push_back(make_shared<Expression>(identityFunctionVarLocation));
                                    cur = get<FunctionExpression>(root->expression).subexpressions.back();
                                }
                            } else {
                                get<MemberFunctionCall>(root->expression).subexpressions.push_back(make_shared<Expression>(identityFunctionVarLocation));
                                cur = get<MemberFunctionCall>(root->expression).subexpressions.back();
                            }
                        } else {
                            root = make_shared<Expression>(identityFunctionVarLocation);
                            cur = root;
                        }
                    } else {
                        auto funccall = make_shared<Expression>(make_shared<Value>(string(strings[i])));
                        if (root) {
                            get<FunctionExpression>(root->expression).subexpressions.push_back(funccall);
                            cur = get<FunctionExpression>(root->expression).subexpressions.back();
                        } else {
                            root = funccall;
                            cur = root;
                        }
                        ++i;
                    }
                    vector<string_view> minisub;
                    int nestLayers = 1;
                    while (nestLayers > 0 && ++i < strings.size()) {
                        if (nestLayers == 1 && strings[i] == ",") {
                            if (minisub.size()) {
                                get<FunctionExpression>(cur->expression).subexpressions.push_back(getExpression(move(minisub), scope, classs));
                                minisub.clear();
                            }
                        } else if (isClosingBracketOrParen(strings[i])) {
                            if (--nestLayers > 0) {
                                minisub.push_back(strings[i]);
                            } else {
                                if (minisub.size()) {
                                    get<FunctionExpression>(cur->expression).subexpressions.push_back(getExpression(move(minisub), scope, classs));
                                    minisub.clear();
                                }
                            }
                        } else if (isOpeningBracketOrParen(strings[i]) || !(strings[i].size() == 1 && contains("+-*%/"s, strings[i][0])) && i + 2 < strings.size() && strings[i + 1] == "(") {
                            ++nestLayers;
                            if (strings[i] == "(") {
                                minisub.push_back(strings[i]);
                            } else {
                                minisub.push_back(strings[i]);
                                ++i;
                                if (isClosingBracketOrParen(strings[i])) {
                                    --nestLayers;
                                }
                                if (nestLayers > 0) {
                                    minisub.push_back(strings[i]);
                                }
                            }
                        } else {
                            minisub.push_back(strings[i]);
                        }
                    }

                } else if (strings[i] == "[" || i + 2 < strings.size() && strings[i + 1] == "[") {
                    // list
                    bool indexOfIndex = i > 0 && (isClosingBracketOrParen(strings[i - 1]) || strings[i - 1].back() == '\"') || (i > 1 && strings[i - 2] == ".");
                    ExpressionRef cur = nullptr;
                    if (!indexOfIndex && strings[i] == "[") {
                        // list literal / collection literal
                        if (root) {
                            get<FunctionExpression>(root->expression).subexpressions.push_back(make_shared<Expression>(make_shared<Value>(List()), ExpressionType::Value));
                            cur = get<FunctionExpression>(root->expression).subexpressions.back();
                        } else {
                            root = make_shared<Expression>(make_shared<Value>(List()), ExpressionType::Value);
                            cur = root;
                        }
                        vector<string_view> minisub;
                        int nestLayers = 1;
                        while (nestLayers > 0 && ++i < strings.size()) {
                            if (nestLayers == 1 && strings[i] == ",") {
                                if (minisub.size()) {
                                    auto val = *getValue(move(minisub), scope, classs);
                                    get<ValueRef>(cur->expression)->getList().push_back(make_shared<Value>(val.value));
                                    minisub.clear();
                                }
                            } else if (isClosingBracketOrParen(strings[i])) {
                                if (--nestLayers > 0) {
                                    minisub.push_back(strings[i]);
                                } else {
                                    if (minisub.size()) {
                                        auto val = *getValue(move(minisub), scope, classs);
                                        get<ValueRef>(cur->expression)->getList().push_back(make_shared<Value>(val.value));
                                        minisub.clear();
                                    }
                                }
                            } else if (isOpeningBracketOrParen(strings[i])) {
                                ++nestLayers;
                                minisub.push_back(strings[i]);
                            } else {
                                minisub.push_back(strings[i]);
                            }
                        }
                        auto &list = get<ValueRef>(cur->expression)->getList();
                        if (list.size()) {
                            bool canBeArray = true;
                            auto type = list[0]->getType();
                            for (auto &val: list) {
                                if (val->getType() == Type::Null || val->getType() != type || (int) val->getType() >= (int) Type::Array) {
                                    canBeArray = false;
                                    break;
                                }
                            }
                            if (canBeArray) {
                                get<ValueRef>(cur->expression)->hardconvert(Type::Array);
                            }
                        }
                    } else {
                        // list access
                        auto indexexpr = make_shared<Expression>(listIndexFunctionVarLocation);
                        if (indexOfIndex) {
                            cur = root;
                            auto parent = root;
                            while (cur->type == ExpressionType::FunctionCall && get<FunctionExpression>(cur->expression).function->getType() == Type::Function && get<FunctionExpression>(cur->expression).function->getFunction()->opPrecedence != OperatorPrecedence::func) {
                                parent = cur;
                                cur = get<FunctionExpression>(cur->expression).subexpressions.back();
                            }
                            get<FunctionExpression>(indexexpr->expression).subexpressions.push_back(cur);
                            if (cur == root) {
                                root = indexexpr;
                                cur = indexexpr;
                            } else {
                                get<FunctionExpression>(parent->expression).subexpressions.pop_back();
                                get<FunctionExpression>(parent->expression).subexpressions.push_back(indexexpr);
                                cur = get<FunctionExpression>(parent->expression).subexpressions.back();
                            }
                        } else {
                            if (root) {
                                get<FunctionExpression>(root->expression).subexpressions.push_back(indexexpr);
                                cur = get<FunctionExpression>(root->expression).subexpressions.back();
                            } else {
                                root = indexexpr;
                                cur = root;
                            }
                            get<FunctionExpression>(cur->expression).subexpressions.push_back(getResolveVarExpression(string(strings[i]), parseScope->isClassScope));
                            ++i;
                        }

                        vector<string_view> minisub;
                        int nestLayers = 1;
                        while (nestLayers > 0 && ++i < strings.size()) {
                            if (strings[i] == "]") {
                                if (--nestLayers > 0) {
                                    minisub.push_back(strings[i]);
                                } else {
                                    if (minisub.size()) {
                                        get<FunctionExpression>(cur->expression).subexpressions.push_back(getExpression(move(minisub), scope, classs));
                                        minisub.clear();
                                    }
                                }
                            } else if (strings[i] == "[") {
                                ++nestLayers;
                                minisub.push_back(strings[i]);
                            } else {
                                minisub.push_back(strings[i]);
                            }
                        }
                    }
                } else {
                    // variable
                    ExpressionRef newExpr;
                    if (strings[i] == "true") {
                        newExpr = make_shared<Expression>(make_shared<Value>(Int(1)), ExpressionType::Value);
                    } else if (strings[i] == "false") {
                        newExpr = make_shared<Expression>(make_shared<Value>(Int(0)), ExpressionType::Value);
                    } else if (strings[i] == "null") {
                        newExpr = make_shared<Expression>(make_shared<Value>(), ExpressionType::Value);
                    } else {
                        newExpr = getResolveVarExpression(string(strings[i]), parseScope->isClassScope);
                    }

                    if (root) {
                        if (root->type == ExpressionType::ResolveVar || root->type == ExpressionType::MemberVariable) {
                            throw Exception("Syntax Error: unexpected series of values at "s + string(strings[i]) + ", possible missing `,`");
                        } else {
                            get<FunctionExpression>(root->expression).subexpressions.push_back(newExpr);
                        }
                    } else {
                        root = newExpr;
                    }
                }
            } else if (isMemberCall(strings[i])) {
                // member var
                bool isfunc = strings.size() > i + 2 && strings[i + 2] == "("s;
                if (isfunc) {
                    ExpressionRef expr;
                    {
                        if (root->type == ExpressionType::FunctionCall && get<FunctionExpression>(root->expression).subexpressions.size()) {
                            expr = make_shared<Expression>(get<FunctionExpression>(root->expression).subexpressions.back(), string(strings[++i]), vector<ExpressionRef>());
                            get<FunctionExpression>(root->expression).subexpressions.pop_back();
                            get<FunctionExpression>(root->expression).subexpressions.push_back(expr);
                        } else {
                            expr = make_shared<Expression>(root, string(strings[++i]), vector<ExpressionRef>());
                            root = expr;
                        }
                    }
                    bool addedArgs = false;
                    auto previ = i;
                    ++i;
                    vector<string_view> minisub;
                    int nestLayers = 1;
                    while (nestLayers > 0 && ++i < strings.size()) {
                        if (nestLayers == 1 && strings[i] == ",") {
                            if (minisub.size()) {
                                get<MemberFunctionCall>(expr->expression).subexpressions.push_back(getExpression(move(minisub), scope, classs));
                                minisub.clear();
                                addedArgs = true;
                            }
                        } else if (strings[i] == ")") {
                            if (--nestLayers > 0) {
                                minisub.push_back(strings[i]);
                            } else {
                                if (minisub.size()) {
                                    get<MemberFunctionCall>(expr->expression).subexpressions.push_back(getExpression(move(minisub), scope, classs));
                                    minisub.clear();
                                    addedArgs = true;
                                }
                            }
                        } else if (strings[i] == "(") {
                            ++nestLayers;
                            minisub.push_back(strings[i]);
                        } else {
                            minisub.push_back(strings[i]);
                        }
                    }
                    if (!addedArgs) {
                        i = previ;
                    }
                } else {
                    if (root->type == ExpressionType::FunctionCall && get<FunctionExpression>(root->expression).subexpressions.size()) {
                        auto expr = make_shared<Expression>(get<FunctionExpression>(root->expression).subexpressions.back(), string(strings[++i]));
                        get<FunctionExpression>(root->expression).subexpressions.pop_back();
                        get<FunctionExpression>(root->expression).subexpressions.push_back(expr);
                    } else {
                        root = make_shared<Expression>(root, string(strings[++i]));
                    }
                }
            } else {
                // number
                auto val = fromChars(strings[i]);
                bool isFloat = contains(strings[i], '.');
                auto newExpr = make_shared<Expression>(ValueRef(isFloat ? new Value((Float) val) : new Value((Int) val)), ExpressionType::Value);
                if (root) {
                    get<FunctionExpression>(root->expression).subexpressions.push_back(newExpr);
                } else {
                    root = newExpr;
                }
            }
            ++i;
        }

        return root;
    }

    // parse one token at a time, uses the state machine
    void MuScriptInterpreter::parse(string_view token) {
        auto tempState = parseState;
        switch (parseState) {
            case ParseState::beginExpression: {
                bool wasElse = false;
                bool closedScope = false;
                bool closedExpr = false;
                bool isEndCurlBracket = false;
                if (token == "fn" || token == "func" || token == "function") {
                    parseState = ParseState::defineFunc;
                } else if (token == "var") {
                    parseState = ParseState::defineVar;
                } else if (token == "for" || token == "while") {
                    parseState = ParseState::loopCall;
                    if (currentExpression) {
                        auto newexpr = make_shared<Expression>(Loop(), currentExpression);
                        currentExpression->push_back(newexpr);
                        currentExpression = newexpr;
                    } else {
                        currentExpression = make_shared<Expression>(Loop());
                    }
                } else if (token == "foreach") {
                    parseState = ParseState::forEach;
                    if (currentExpression) {
                        auto newexpr = make_shared<Expression>(Foreach(), currentExpression);
                        currentExpression->push_back(newexpr);
                        currentExpression = newexpr;
                    } else {
                        currentExpression = make_shared<Expression>(Foreach());
                    }
                } else if (token == "if") {
                    parseState = ParseState::ifCall;
                    if (currentExpression) {
                        auto newexpr = make_shared<Expression>(IfElse(), currentExpression);
                        currentExpression->push_back(newexpr);
                        currentExpression = newexpr;
                    } else {
                        currentExpression = make_shared<Expression>(IfElse());
                    }
                } else if (token == "else") {
                    parseState = ParseState::expectIfEnd;
                    wasElse = true;
                } else if (token == "class") {
                    parseState = ParseState::defineClass;
                } else if (token == "{") {
                    parseScope = newScope("__anon"s, parseScope);
                    clearParseStacks();
                } else if (token == "}") {
                    wasElse = !currentExpression || currentExpression->type != ExpressionType::IfElse;
                    bool wasFreefunc = !currentExpression || (currentExpression->type == ExpressionType::FunctionDef && get<FunctionExpression>(currentExpression->expression).function->getFunction()->type == FunctionType::free);
                    closedExpr = closeCurrentExpression();
                    if (!closedExpr && wasFreefunc || parseScope->name == "__anon") {
                        closeScope(parseScope);
                    }
                    closedScope = true;
                    isEndCurlBracket = true;
                } else if (token == "return") {
                    parseState = ParseState::returnLine;
                } else if (token == "import") {
                    parseState = ParseState::importModule;
                } else if (token == ";") {
                    clearParseStacks();
                } else {
                    parseState = ParseState::readLine;
                    parseStrings.push_back(token);
                }
                if (!closedExpr && (closedScope && lastStatementClosedScope || (!lastStatementWasElse && !wasElse && lastTokenEndCurlBraket))) {
                    bool wasIfExpr = currentExpression && currentExpression->type == ExpressionType::IfElse;
                    closeDanglingIfExpression();
                    if (closedScope && wasIfExpr && currentExpression->type != ExpressionType::IfElse) {
                        closeCurrentExpression();
                        closedScope = false;
                    }
                }
                lastStatementClosedScope = closedScope;
                lastTokenEndCurlBraket = isEndCurlBracket;
                lastStatementWasElse = wasElse;
            } break;
            case ParseState::loopCall:
                if (token == ")") {
                    if (--outerNestLayer <= 0) {
                        vector<vector<string_view>> exprs = {};
                        exprs.push_back({});
                        for (auto &&str: parseStrings) {
                            if (str == ";") {
                                exprs.push_back({});
                            } else {
                                exprs.back().push_back(str);
                            }
                        }
                        auto &loop = get<Loop>(currentExpression->expression);
                        switch (exprs.size()) {
                            case 1:
                                loop.testExpression = getExpression(exprs[0], parseScope, nullptr);
                                break;
                            case 2:
                                loop.testExpression = getExpression(exprs[0], parseScope, nullptr);
                                loop.iterateExpression = getExpression(exprs[1], parseScope, nullptr);
                                break;
                            case 3: {
                                auto name = exprs[0].front();
                                exprs[0].erase(exprs[0].begin(), exprs[0].begin() + 2);
                                loop.initExpression = make_shared<Expression>(DefineVar(string(name), getExpression(exprs[0], parseScope, nullptr)));
                                loop.testExpression = getExpression(exprs[1], parseScope, nullptr);
                                loop.iterateExpression = getExpression(exprs[2], parseScope, nullptr);
                            } break;
                            default:
                                break;
                        }

                        clearParseStacks();
                        outerNestLayer = 0;
                    } else {
                        parseStrings.push_back(token);
                    }
                } else if (token == "(") {
                    if (++outerNestLayer > 1) {
                        parseStrings.push_back(token);
                    }
                } else {
                    parseStrings.push_back(token);
                }
                break;
            case ParseState::forEach:
                if (token == ")") {
                    if (--outerNestLayer <= 0) {
                        vector<vector<string_view>> exprs = {};
                        exprs.push_back({});
                        for (auto &&str: parseStrings) {
                            if (str == ";") {
                                exprs.push_back({});
                            } else {
                                exprs.back().push_back(move(str));
                            }
                        }
                        if (exprs.size() != 2) {
                            clearParseStacks();
                            throw Exception("Syntax error, `foreach` requires 2 statements, "s + std::to_string(exprs.size()) + " statements supplied instead");
                        }

                        auto name = string(exprs[0][0]);
                        resolveVariable(name, parseScope);
                        get<Foreach>(currentExpression->expression).iterateName = move(name);
                        get<Foreach>(currentExpression->expression).listExpression = getExpression(exprs[1], parseScope, nullptr);

                        clearParseStacks();
                        outerNestLayer = 0;
                    } else {
                        parseStrings.push_back(token);
                    }
                } else if (token == "(") {
                    if (++outerNestLayer > 1) {
                        parseStrings.push_back(token);
                    }
                } else {
                    parseStrings.push_back(token);
                }
                break;
            case ParseState::ifCall:
                if (token == ")") {
                    if (--outerNestLayer <= 0) {
                        currentExpression->push_back(If());
                        get<IfElse>(currentExpression->expression).back().testExpression = getExpression(move(parseStrings), parseScope, nullptr);
                        clearParseStacks();
                    } else {
                        parseStrings.push_back(token);
                    }
                } else if (token == "(") {
                    if (++outerNestLayer > 1) {
                        parseStrings.push_back(token);
                    }
                } else {
                    parseStrings.push_back(token);
                }
                break;
            case ParseState::readLine:
                if (token == ";") {
                    auto line = std::move(parseStrings);
                    clearParseStacks();
                    // we clear before evaluating lines so any exceptions can clear the offending code
                    if (!currentExpression) {
                        getValue(line, parseScope, nullptr);
                    } else {
                        currentExpression->push_back(getExpression(line, parseScope, nullptr));
                    }

                } else {
                    parseStrings.push_back(token);
                }
                break;
            case ParseState::returnLine:
                if (token == ";") {
                    if (currentExpression) {
                        currentExpression->push_back(make_shared<Expression>(Return(getExpression(move(parseStrings), parseScope, nullptr))));
                    }
                    clearParseStacks();
                } else {
                    parseStrings.push_back(token);
                }
                break;
            case ParseState::expectIfEnd:
                if (token == "if") {
                    clearParseStacks();
                    parseState = ParseState::ifCall;
                } else if (token == "{") {
                    parseScope = newScope("ifelse"s, parseScope);
                    currentExpression->push_back(If());
                    clearParseStacks();
                } else {
                    clearParseStacks();
                    throw Exception("Malformed Syntax: Incorrect token `" + string(token) + "` following `else` keyword");
                }
                break;
            case ParseState::defineVar:
                if (token == ";") {
                    if (parseStrings.size() == 0) {
                        throw Exception("Malformed Syntax: `var` keyword must be followed by user supplied name");
                    }
                    auto name = parseStrings.front();
                    ExpressionRef defineExpr = nullptr;
                    if (parseStrings.size() > 2) {
                        parseStrings.erase(parseStrings.begin());
                        parseStrings.erase(parseStrings.begin());
                        defineExpr = getExpression(move(parseStrings), parseScope, nullptr);
                    }
                    if (currentExpression) {
                        currentExpression->push_back(make_shared<Expression>(DefineVar(string(name), defineExpr)));
                    } else {
                        getValue(make_shared<Expression>(DefineVar(string(name), defineExpr)), parseScope, nullptr);
                    }
                    clearParseStacks();
                } else {
                    parseStrings.push_back(token);
                }
                break;
            case ParseState::defineClass:
                parseScope = newClassScope(string(token), parseScope);
                parseState = ParseState::classArgs;
                parseStrings.clear();
                break;
            case ParseState::classArgs:
                if (token == ",") {
                    if (parseStrings.size()) {
                        auto otherscope = resolveScope(string(parseStrings.back()), parseScope);
                        parseScope->variables.insert(otherscope->variables.begin(), otherscope->variables.end());
                        parseScope->functions.insert(otherscope->functions.begin(), otherscope->functions.end());
                        parseStrings.clear();
                    }
                } else if (token == "{") {
                    if (parseStrings.size()) {
                        auto otherscope = resolveScope(string(parseStrings.back()), parseScope);
                        parseScope->variables.insert(otherscope->variables.begin(), otherscope->variables.end());
                        parseScope->functions.insert(otherscope->functions.begin(), otherscope->functions.end());
                    }
                    clearParseStacks();
                } else {
                    parseStrings.push_back(token);
                }
                break;
            case ParseState::defineFunc:
                parseStrings.push_back(token);
                parseState = ParseState::funcArgs;
                break;
            case ParseState::importModule:
                clearParseStacks();
                if (token.size() > 2 && token.front() == '\"' && token.back() == '\"') {
                    // import file
                    evaluateFile(string(token.substr(1, token.size() - 2)));
                    clearParseStacks();
                } else {
                    auto modName = string(token);
                    auto iter = std::find_if(modules.begin(), modules.end(), [&modName](auto &mod) { return mod.scope->name == modName; });
                    if (iter == modules.end()) {
                        auto newMod = getOptionalModule(modName);
                        if (newMod) {
                            if (shouldAllow(allowedModulePrivileges, newMod->requiredPermissions)) {
                                modules.emplace_back(newMod->requiredPermissions, newMod->scope);
                            } else {
                                throw Exception("Error: Cannot import restricted module: "s + modName);
                            }
                        }
                    }
                }
                break;
            case ParseState::funcArgs:
                if (token == "(" || token == ",") {
                    // eat these tokens
                } else if (token == ")") {
                    auto fncName = move(parseStrings.front());
                    parseStrings.erase(parseStrings.begin());
                    vector<string> args;
                    args.reserve(parseStrings.size());
                    for (auto parseString: parseStrings) {
                        args.emplace_back(parseString);
                    }
                    auto isConstructor = parseScope->isClassScope && parseScope->name == fncName;
                    auto newfunc = isConstructor ? newConstructor(string(fncName), parseScope->parent, args) : newFunction(string(fncName), parseScope, args);
                    if (currentExpression) {
                        auto newexpr = make_shared<Expression>(newfunc, currentExpression);
                        currentExpression->push_back(newexpr);
                        currentExpression = newexpr;
                    } else {
                        currentExpression = make_shared<Expression>(newfunc, nullptr);
                    }
                } else if (token == "{") {
                    clearParseStacks();
                } else {
                    parseStrings.push_back(token);
                }
                break;
            default:
                break;
        }

        prevState = tempState;
    }

    bool MuScriptInterpreter::readLine(string_view text) {
        ++currentLine;
        auto tokenCount = 0;
        auto tokens = ViewTokenize(text);
        bool didExcept = false;
        try {
            for (auto &token: tokens) {
                parse(token);
                ++tokenCount;
            }
        } catch (Exception e) {
#if defined MUSCRIPT_DO_INTERNAL_PRINT
            callFunctionWithArgs(resolveFunction("print"), "Error at line "s + std::to_string(currentLine) + ", at: " + std::to_string(tokenCount) + string(tokens[tokenCount]) + ": " + e.wh + "\n");
#else
            printf("Error at line %llu at %i: %s : %s\n", currentLine, tokenCount, string(tokens[tokenCount]).c_str(), e.wh.c_str());
#endif
            clearParseStacks();
            parseScope = globalScope;
            currentExpression = nullptr;
            didExcept = true;
        } catch (std::exception &e) {
#if defined MUSCRIPT_DO_INTERNAL_PRINT
            callFunctionWithArgs(resolveFunction("print"), "Error at line "s + std::to_string(currentLine) + ", at: " + std::to_string(tokenCount) + string(tokens[tokenCount]) + ": " + e.what() + "\n");
#else
            printf("Error at line %llu at %i: %s : %s\n", currentLine, tokenCount, string(tokens[tokenCount]).c_str(), e.what());
#endif
            clearParseStacks();
            parseScope = globalScope;
            currentExpression = nullptr;
            didExcept = true;
        }
        return didExcept;
    }

    bool MuScriptInterpreter::evaluate(string_view script) {
        for (auto &line: split(script, '\n')) {
            if (readLine(line)) {
                return true;
            }
        }
        // close any dangling if-expressions that may exist
        return readLine(";"s);
    }

    bool MuScriptInterpreter::evaluateFile(const string &path) {
        string s;
        auto file = std::ifstream(path);
        if (file) {
            file.seekg(0, std::ios::end);
            s.reserve(file.tellg());
            file.seekg(0, std::ios::beg);
            // bash kata scripts have a header line we need to skip
            if (endswith(path, ".sh")) {
                getline(file, s);
            }
            s.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return evaluate(s);
        } else {
            printf("file: %s not found\n", path.c_str());
            return 1;
        }
    }

    bool MuScriptInterpreter::readLine(string_view text, ScopeRef scope) {
        auto temp = parseScope;
        parseScope = scope;
        auto result = readLine(text);
        parseScope = temp;
        return result;
    }

    bool MuScriptInterpreter::evaluate(string_view script, ScopeRef scope) {
        auto temp = parseScope;
        parseScope = scope;
        auto result = evaluate(script);
        parseScope = temp;
        return result;
    }

    bool MuScriptInterpreter::evaluateFile(const string &path, ScopeRef scope) {
        auto temp = parseScope;
        parseScope = scope;
        auto result = evaluateFile(path);
        parseScope = temp;
        return result;
    }

    void ClearScope(ScopeRef s) {
        s->parent = nullptr;
        for (auto &&c: s->scopes) {
            ClearScope(c.second);
        }
        s->scopes.clear();
    }

    void MuScriptInterpreter::clearState() {
        clearParseStacks();
        ClearScope(globalScope);
        globalScope = make_shared<Scope>(this);
        parseScope = globalScope;
        currentExpression = nullptr;
        if (modules.size() > 1) {
            modules.erase(modules.begin() + 1, modules.end());
        }
    }

    // general purpose clear to reset state machine for next statement
    void MuScriptInterpreter::clearParseStacks() {
        parseState = ParseState::beginExpression;
        parseStrings.clear();
        outerNestLayer = 0;
    }
}// namespace MuScript


// --------------------------------------- parsing


// --------------------------------------- scopeImplementation


namespace MuScript {
    // scope control lets you have object lifetimes
    ScopeRef MuScriptInterpreter::newScope(const string &name, ScopeRef scope) {
        // if the scope exists we just use it as is
        auto iter = scope->scopes.find(name);
        if (iter != scope->scopes.end()) {
            return iter->second;
        } else {
            return scope->insertScope(make_shared<Scope>(name, scope));
        }
    }

    ScopeRef MuScriptInterpreter::insertScope(ScopeRef existing, ScopeRef parent) {
        existing->parent = parent;
        return parent->insertScope(existing);
    }

    void MuScriptInterpreter::closeScope(ScopeRef &scope) {
        if (scope->parent) {
            if (scope->isClassScope) {
                scope = scope->parent;
            } else {
                auto name = scope->name;
                scope->functions.clear();
                scope->variables.clear();
                scope->scopes.clear();
                scope = scope->parent;
                scope->scopes.erase(name);
            }
        }
    }

    ScopeRef MuScriptInterpreter::newClassScope(const string &name, ScopeRef scope) {
        auto ref = newScope(name, scope);
        ref->isClassScope = true;
        return ref;
    }
}// namespace MuScript


// --------------------------------------- scopeImplementation

namespace MuScript {
    template<typename T>
    vector<T> &Array::getStdVector() {
        return get<vector<T>>(value);
    }

    template<typename T>
    vector<T> &Value::getStdVector() {
        return get<Array>(value).getStdVector<T>();
    }

    Class::Class(const Class &o) : name(o.name), functionScope(o.functionScope) {
        for (auto &&v: o.variables) {
            variables[v.first] = make_shared<Value>(v.second->value);
        }
    }

    Class::Class(const ScopeRef &o) : name(o->name), functionScope(o) {
        for (auto &&v: o->variables) {
            variables[v.first] = make_shared<Value>(v.second->value);
        }
    }

    Class::~Class() {
        auto iter = functionScope->functions.find("~"s + name);
        if (iter != functionScope->functions.end()) {
            functionScope->host->callFunction(iter->second, functionScope, {}, this);
        }
    }

    size_t Value::getHash() {
        size_t hash = 0;
        switch (getType()) {
            default:
                break;
            case Type::Int:
                hash = (size_t) getInt();
                break;
            case Type::Float:
                hash = std::hash<Float>{}(getFloat());
                break;
            case Type::Vec3:
                hash = std::hash<float>{}(getVec3().x) ^ std::hash<float>{}(getVec3().y) ^ std::hash<float>{}(getVec3().z);
                break;
            case Type::Function:
                hash = std::hash<size_t>{}((size_t) getFunction().get());
                break;
            case Type::String:
                hash = std::hash<string>{}(getString());
                break;
        }
        return hash ^ typeHashBits(getType());
    }

    // convert this value up to the newType
    void Value::upconvert(Type newType) {
        if (newType > getType()) {
            switch (newType) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Int:
                    value = Int(0);
                    break;
                case Type::Float:
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::Null:
                            value = 0.f;
                            break;
                        case Type::Int:
                            value = (Float) getInt();
                            break;
                    }
                    break;

                case Type::Vec3:
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::Null:
                            value = vec3();
                            break;
                        case Type::Int:
                            value = vec3((float) getInt());
                            break;
                        case Type::Float:
                            value = vec3((float) getFloat());
                            break;
                    }
                    break;
                case Type::String:
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::Null:
                            value = "null"s;
                            break;
                        case Type::Int:
                            value = std::to_string(getInt());
                            break;
                        case Type::Float:
                            value = std::to_string(getFloat());
                            break;
                        case Type::Vec3: {
                            auto &vec = getVec3();
                            value = std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z);
                            break;
                        }
                        case Type::Function:
                            value = getFunction()->name;
                            break;
                    }
                    break;
                case Type::Array:
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::Null:
                            value = Array();
                            getArray().push_back(Int(0));
                            break;
                        case Type::Int:
                            value = Array(vector<Int>{getInt()});
                            break;
                        case Type::Float:
                            value = Array(vector<Float>{getFloat()});
                            break;
                        case Type::Vec3:
                            value = Array(vector<vec3>{getVec3()});
                            break;
                        case Type::String: {
                            auto str = getString();
                            value = Array(vector<string>{});
                            auto &arry = getStdVector<string>();
                            for (auto &&ch: str) {
                                arry.push_back(""s + ch);
                            }
                        } break;
                    }
                    break;
                case Type::List:
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::Null:
                        case Type::Int:
                        case Type::Float:
                        case Type::Vec3:
                            value = List({make_shared<Value>(value)});
                            break;
                        case Type::String: {
                            auto str = getString();
                            value = List();
                            auto &list = getList();
                            for (auto &&ch: str) {
                                list.push_back(make_shared<Value>(""s + ch));
                            }
                        } break;
                        case Type::Array:
                            Array arr = getArray();
                            value = List();
                            auto &list = getList();
                            switch (arr.getType()) {
                                case Type::Int:
                                    for (auto &&item: get<vector<Int>>(arr.value)) {
                                        list.push_back(make_shared<Value>(item));
                                    }
                                    break;
                                case Type::Float:
                                    for (auto &&item: get<vector<Float>>(arr.value)) {
                                        list.push_back(make_shared<Value>(item));
                                    }
                                    break;
                                case Type::Vec3:
                                    for (auto &&item: get<vector<vec3>>(arr.value)) {
                                        list.push_back(make_shared<Value>(item));
                                    }
                                    break;
                                case Type::String:
                                    for (auto &&item: get<vector<string>>(arr.value)) {
                                        list.push_back(make_shared<Value>(item));
                                    }
                                    break;
                                default:
                                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                                    break;
                            }
                            break;
                    }
                    break;
                case Type::Dictionary:
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::Null:
                        case Type::Int:
                        case Type::Float:
                        case Type::Vec3:
                        case Type::String:
                            value = make_shared<Dictionary>();
                            break;
                        case Type::Array: {
                            Array arr = getArray();
                            value = make_shared<Dictionary>();
                            auto hashbits = typeHashBits(Type::Int);
                            auto &dict = getDictionary();
                            size_t index = 0;
                            switch (arr.getType()) {
                                case Type::Int:
                                    for (auto &&item: get<vector<Int>>(arr.value)) {
                                        (*dict)[index++ ^ hashbits] = make_shared<Value>(item);
                                    }
                                    break;
                                case Type::Float:
                                    for (auto &&item: get<vector<Float>>(arr.value)) {
                                        (*dict)[index++ ^ hashbits] = make_shared<Value>(item);
                                    }
                                    break;
                                case Type::Vec3:
                                    for (auto &&item: get<vector<vec3>>(arr.value)) {
                                        (*dict)[index++ ^ hashbits] = make_shared<Value>(item);
                                    }
                                    break;
                                case Type::String:
                                    for (auto &&item: get<vector<string>>(arr.value)) {
                                        (*dict)[index++ ^ hashbits] = make_shared<Value>(item);
                                    }
                                    break;
                                default:
                                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                                    break;
                            }
                        } break;
                        case Type::List: {
                            auto hashbits = typeHashBits(Type::Int);
                            List list = getList();
                            value = make_shared<Dictionary>();
                            auto &dict = getDictionary();
                            size_t index = 0;
                            for (auto &&item: list) {
                                (*dict)[index++ ^ hashbits] = item;
                            }
                        } break;
                    }
                    break;
            }
        }
    }

    // convert this value to the newType even if it's a downcast
    void Value::hardconvert(Type newType) {
        if (newType >= getType()) {
            upconvert(newType);
        } else {
            switch (newType) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Null:
                    value = Int(0);
                    break;
                case Type::Int:
                    switch (getType()) {
                        default:
                            break;
                        case Type::Float:
                            value = (Int) getFloat();
                            break;
                        case Type::String:
                            value = (Int) fromChars(getString());
                            break;
                        case Type::Array:
                            value = (Int) getArray().size();
                            break;
                        case Type::List:
                            value = (Int) getList().size();
                            break;
                    }
                    break;
                case Type::Float:
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::String:
                            value = (Float) fromChars(getString());
                            break;
                        case Type::Array:
                            value = (Float) getArray().size();
                            break;
                        case Type::List:
                            value = (Float) getList().size();
                            break;
                    }
                    break;
                case Type::String:
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::Array: {
                            string newval;
                            auto &arr = getArray();
                            switch (arr.getType()) {
                                case Type::Int:
                                    for (auto &&item: get<vector<Int>>(arr.value)) {
                                        newval += Value(item).getPrintString() + ", ";
                                    }
                                    break;
                                case Type::Float:
                                    for (auto &&item: get<vector<Float>>(arr.value)) {
                                        newval += Value(item).getPrintString() + ", ";
                                    }
                                    break;
                                case Type::Vec3:
                                    for (auto &&item: get<vector<vec3>>(arr.value)) {
                                        newval += Value(item).getPrintString() + ", ";
                                    }
                                    break;
                                case Type::String:
                                    for (auto &&item: get<vector<string>>(arr.value)) {
                                        newval += Value(item).getPrintString() + ", ";
                                    }
                                    break;
                                default:
                                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                                    break;
                            }
                            if (arr.size()) {
                                newval.pop_back();
                                newval.pop_back();
                            }
                            value = newval;
                        } break;
                        case Type::List: {
                            string newval;
                            auto &list = getList();
                            for (auto val: list) {
                                newval += val->getPrintString() + ", ";
                            }
                            if (newval.size()) {
                                newval.pop_back();
                                newval.pop_back();
                            }
                            value = newval;
                        } break;
                        case Type::Dictionary: {
                            string newval;
                            auto &dict = getDictionary();
                            for (auto &&val: *dict) {
                                newval += "`"s + std::to_string(val.first) + ": " + val.second->getPrintString() + "`, ";
                            }
                            if (newval.size()) {
                                newval.pop_back();
                                newval.pop_back();
                            }
                            value = newval;
                        } break;
                        case Type::Class: {
                            auto &strct = getClass();
                            string newval = strct->name + ":\n"s;
                            for (auto &&val: strct->variables) {
                                newval += "`"s + val.first + ": " + val.second->getPrintString() + "`\n";
                            }
                            value = newval;
                        } break;
                    }
                    break;
                case Type::Array: {
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::Dictionary: {
                            Array arr;
                            auto dict = getDictionary();
                            auto listType = dict->begin()->second->getType();
                            switch (listType) {
                                case Type::Int:
                                    arr = Array(vector<Int>{});
                                    for (auto &&item: *dict) {
                                        if (item.second->getType() == listType) {
                                            arr.push_back(item.second->getInt());
                                        }
                                    }
                                    break;
                                case Type::Float:
                                    arr = Array(vector<Float>{});
                                    for (auto &&item: *dict) {
                                        if (item.second->getType() == listType) {
                                            arr.push_back(item.second->getFloat());
                                        }
                                    }
                                    break;
                                case Type::Vec3:
                                    arr = Array(vector<vec3>{});
                                    for (auto &&item: *dict) {
                                        if (item.second->getType() == listType) {
                                            arr.push_back(item.second->getVec3());
                                        }
                                    }
                                    break;
                                case Type::Function:
                                    arr = Array(vector<FunctionRef>{});
                                    for (auto &&item: *dict) {
                                        if (item.second->getType() == listType) {
                                            arr.push_back(item.second->getFunction());
                                        }
                                    }
                                    break;
                                case Type::String:
                                    arr = Array(vector<string>{});
                                    for (auto &&item: *dict) {
                                        if (item.second->getType() == listType) {
                                            arr.push_back(item.second->getString());
                                        }
                                    }
                                    break;
                                default:
                                    throw Exception("Array cannot contain collections");
                                    break;
                            }
                            value = arr;
                        } break;
                        case Type::List: {
                            auto list = getList();
                            auto listType = list[0]->getType();
                            Array arr;
                            switch (listType) {
                                case Type::Null:
                                    arr = Array(vector<Int>{});
                                    break;
                                case Type::Int:
                                    arr = Array(vector<Int>{});
                                    for (auto &&item: list) {
                                        if (item->getType() == listType) {
                                            arr.push_back(item->getInt());
                                        }
                                    }
                                    break;
                                case Type::Float:
                                    arr = Array(vector<Float>{});
                                    for (auto &&item: list) {
                                        if (item->getType() == listType) {
                                            arr.push_back(item->getFloat());
                                        }
                                    }
                                    break;
                                case Type::Vec3:
                                    arr = Array(vector<vec3>{});
                                    for (auto &&item: list) {
                                        if (item->getType() == listType) {
                                            arr.push_back(item->getVec3());
                                        }
                                    }
                                    break;
                                case Type::Function:
                                    arr = Array(vector<FunctionRef>{});
                                    for (auto &&item: list) {
                                        if (item->getType() == listType) {
                                            arr.push_back(item->getFunction());
                                        }
                                    }
                                    break;
                                case Type::String:
                                    arr = Array(vector<string>{});
                                    for (auto &&item: list) {
                                        if (item->getType() == listType) {
                                            arr.push_back(item->getString());
                                        }
                                    }
                                    break;
                                default:
                                    throw Exception("Array cannot contain collections");
                                    break;
                            }
                            value = arr;
                        } break;
                    }
                    break;
                } break;
                case Type::List: {
                    switch (getType()) {
                        default:
                            throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                            break;
                        case Type::Dictionary: {
                            List list;
                            for (auto &&item: *getDictionary()) {
                                list.push_back(item.second);
                            }
                            value = list;
                        } break;
                        case Type::Class:
                            value = List({make_shared<Value>(value)});
                            break;
                    }
                } break;
                case Type::Dictionary: {
                    Dictionary dict;
                    for (auto &&item: getClass()->variables) {
                        dict[std::hash<string>()(item.first) ^ typeHashBits(Type::String)] = item.second;
                    }
                }
            }
        }
    }
}// namespace MuScript
