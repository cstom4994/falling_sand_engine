

#ifndef META_DETAIL_FUNCTIONIMPL_HPP
#define META_DETAIL_FUNCTIONIMPL_HPP

#include "engine/meta/function.hpp"
#include "engine/meta/functiontraits.hpp"
#include "engine/meta/metalua.hpp"
#include "engine/meta/valuemapper.hpp"

namespace Meta {
namespace detail {

//--------------------------------------------------------------------------------------

struct FunctionParamInfo {
    const std::type_info* m_typeinfo;
    ValueKind m_valueType;
};

template <int SZ>
struct FunctionMapParamsToValueKind {
    typedef std::array<FunctionParamInfo, SZ> ReturnType;

    template <typename T>
    static FunctionParamInfo apply() {
        return {&typeid(T), mapType<T>()};
    }
};

// Apply B to canonical function F = R(A...)
template <typename FuncArgTuple, typename B>
struct FunctionApplyToParams;

template <typename B, typename... A>
struct FunctionApplyToParams<std::tuple<A...>, B> {
    static typename B::ReturnType foreach () { return typename B::ReturnType{{B::template apply<A>()...}}; }
};

template <typename B>
struct FunctionApplyToParams<std::tuple<void>, B> {
    static typename B::ReturnType foreach () { return typename B::ReturnType{}; }
};

//--------------------------------------------------------------------------------------

template <typename R, typename... P>
struct ReturnPolicy {
    static constexpr policy::ReturnKind kind = policy::ReturnKind::Copy;  // default
};

template <>
struct ReturnPolicy<void>  // nothing returned
{
    static constexpr policy::ReturnKind kind = policy::ReturnKind::NoReturn;
};

template <typename R>
struct ReturnPolicy<R, policy::ReturnInternalRef> {
    static constexpr policy::ReturnKind kind = policy::ReturnKind::InternalRef;
};

//--------------------------------------------------------------------------------------
// FunctionImpl
//--------------------------------------------------------------------------------------

/*
 *  Bake the function type information into non-templated Function.
 */
template <typename T, typename F, typename... P>
class FunctionImpl : public Function {
    typedef T FuncTraits;
    typedef std::tuple<P...> FuncPolicies;

    static constexpr size_t c_nParams = std::tuple_size<typename FuncTraits::Details::ParamTypes>::value;

    std::array<FunctionParamInfo, c_nParams> m_paramInfo;

public:
    FunctionImpl(IdRef name, F function, P... policies) : Function(name) {
        m_name = name;
        m_funcType = FuncTraits::kind;
        m_returnType = mapType<typename FuncTraits::ExposedType>();
        m_returnPolicy = ReturnPolicy<typename FuncTraits::ExposedType, P...>::kind;
        m_paramInfo = FunctionApplyToParams<typename FuncTraits::Details::ParamTypes, FunctionMapParamsToValueKind<c_nParams>>::foreach ();
        Function::m_usesData = &m_userData;

        processUses<uses::Uses::eRuntimeModule>(m_name, function);
        META_IF_LUA(processUses<uses::Uses::eLuaModule>(m_name, function);)
    }

private:
    FunctionImpl(const FunctionImpl&) = delete;

    uses::Uses::PerFunctionUserData m_userData;

    template <int M>
    void processUses(IdRef name, F function) {
        typedef typename std::tuple_element<M, uses::Uses::Users>::type Processor;

        std::get<M>(m_userData) = Processor::template perFunction<F, T, FuncPolicies>(name, function);
    }

    size_t paramCount() const override { return c_nParams; }

    ValueKind paramType(size_t index) const override {
        // Make sure that the index is not out of range
        if (index >= c_nParams) META_ERROR(OutOfRange(index, c_nParams));

        return m_paramInfo[index].m_valueType;
    }
};

// Used by ClassBuilder to create new function instance.
template <typename F, typename... P>
static inline Function* newFunction(IdRef name, F function, P... policies) {
    typedef detail::FunctionTraits<F> FuncTraits;

    static_assert(FuncTraits::kind != FunctionKind::None, "Type is not a function");

    return new FunctionImpl<FuncTraits, F, P...>(name, function, policies...);
}

}  // namespace detail
}  // namespace Meta

#endif  // META_DETAIL_FUNCTIONIMPL_HPP
