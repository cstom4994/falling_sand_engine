#pragma once

#include <cmath>
#include <type_traits>
#include <utility>


namespace MetaEngine::Math {
    namespace detail {

        struct nothing
        {
        };

        template<size_t Begin, size_t End>
        struct static_for
        {
            template<class Func>
            constexpr void operator()(Func &&f) {
                f(Begin);
                static_for<Begin + 1, End>()(std::forward<Func>(f));
            }
        };

        template<size_t N>
        struct static_for<N, N>
        {
            template<class Func>
            constexpr void operator()(Func &&) {
            }
        };

        template<class T>
        constexpr auto decay(T &&t) -> decltype(t.decay()) {
            return t.decay();
        }

        template<class T>
        constexpr typename std::enable_if<
                std::is_arithmetic<typename std::remove_reference<T>::type>::value, T>::type
        decay(T &&t) {
            return t;
        }

        // will be available in C++20
        template<class T>
        struct remove_cvref
        {
            using type = std::remove_cv_t<std::remove_reference_t<T>>;
        };

        template<class T>
        constexpr size_t get_size() {
            if constexpr (std::is_arithmetic<typename remove_cvref<T>::type>::value) {
                return 1;
            } else {
                return remove_cvref<T>::type::num_components;
            }
        }

    }// namespace detail

    namespace detail {

        template<typename vector_type, typename T, size_t N, size_t... indices>
        struct swizzler
        {
            static constexpr auto num_components = sizeof...(indices);// same as vector_type's

            T data[N];// N might differ from num_components ex: .xxxx from vec3

            vector_type decay() const {
                vector_type vec;
                assign_across(vec, 0, indices...);
                return vec;
            }

            operator vector_type() const {
                return decay();
            }

            operator vector_type() {
                return decay();
            }

            swizzler &operator=(const vector_type &vec) {
                assign_across(vec, 0, indices...);
                return *this;
            }
            //TODO: constrain the assignment only when indices are different

            template<typename O>
            swizzler &operator+=(O &&o) {
                return operator=(decay() + std::forward<O>(o));
            }

            template<typename O>
            swizzler &operator-=(O &&o) {
                return operator=(decay() - std::forward<O>(o));
            }

            template<typename O>
            swizzler &operator*=(O &&o) {
                return operator=(decay() * std::forward<O>(o));
            }

            template<typename O>
            swizzler &operator/=(O &&o) {
                return operator=(decay() / std::forward<O>(o));
            }

            vector_type operator-() const {
                return vector_type((-data[indices])...);
            }

        private:
            template<typename... Indices>
            void assign_across(vector_type &vec, size_t i, Indices... swizz_i) const {
                ((vec[i++] = data[swizz_i]), ...);
            }

            template<typename... Indices>
            void assign_across(const vector_type &vec, size_t i, Indices... swizz_i) {
                ((data[swizz_i] = vec[i++]), ...);
            }
        };

    }// namespace detail

    namespace detail {

        template<template<typename, size_t...> class vector, typename scalar_type, size_t... Ns>
        struct builtin_func_lib
        {
#define FUNC(x) lib_##x
#ifdef _MSC_VER
#define LIB static __forceinline
#else
#define LIB static __attribute__((always_inline))
#endif

            using vector_type = vector<scalar_type, Ns...>;
            using vector_arg_type = const vector_type &;
            using bool_vector_type = vector<bool, Ns...>;

            static constexpr auto one = scalar_type(1);
            static constexpr auto zero = scalar_type(0);

            //
            // 8.1 Angle and Trigonometry Function
            //
            LIB vector_type FUNC(radians)(vector_arg_type degrees) {
                constexpr auto pi_over_180 = scalar_type(3.14159265358979323846 / 180);
                return vector_type((degrees.data[Ns] * pi_over_180)...);
            }

            LIB vector_type FUNC(degrees)(vector_arg_type radians) {
                constexpr auto _180_over_pi = scalar_type(180 / 3.14159265358979323846);
                return vector_type((radians.data[Ns] * _180_over_pi)...);
            }

            LIB vector_type FUNC(sin)(vector_arg_type t) {
                return vector_type(std::sin(t.data[Ns])...);
            }

            LIB vector_type FUNC(cos)(vector_arg_type t) {
                return vector_type(std::cos(t.data[Ns])...);
            }

            LIB vector_type FUNC(tan)(vector_arg_type t) {
                return vector_type(std::tan(t.data[Ns])...);
            }

            LIB vector_type FUNC(asin)(vector_arg_type t) {
                return vector_type(std::asin(t.data[Ns])...);
            }

            LIB vector_type FUNC(acos)(vector_arg_type t) {
                return vector_type(std::acos(t.data[Ns])...);
            }

            LIB vector_type FUNC(atan)(vector_arg_type y, vector_arg_type x) {
                return vector_type(std::atan2(y.data[Ns], x.data[Ns])...);
            }

            LIB vector_type FUNC(atan)(vector_arg_type t) {
                return vector_type(std::atan(t.data[Ns])...);
            }

            // TODO: sinh cosh tanh asinh acosh atanh

            //
            // 8.2 Exponential Functions
            //
            LIB vector_type FUNC(pow)(vector_arg_type x, vector_arg_type y) {
                return vector_type(std::pow(x.data[Ns], y.data[Ns])...);
            }

            LIB vector_type FUNC(exp)(vector_arg_type t) {
                return vector_type(std::exp(t.data[Ns])...);
            }

            LIB vector_type FUNC(log)(vector_arg_type t) {
                return vector_type(std::log(t.data[Ns])...);
            }

            LIB vector_type FUNC(exp2)(vector_arg_type t) {
                return vector_type(std::exp2(t.data[Ns])...);
            }

            LIB vector_type FUNC(log2)(vector_arg_type t) {
                return vector_type(std::log2(t.data[Ns])...);
            }

            LIB vector_type FUNC(sqrt)(vector_arg_type t) {
                return vector_type(std::sqrt(t.data[Ns])...);
            }

            LIB scalar_type rsqrt(scalar_type t) {
                return one / std::sqrt(t);//NOTE: https://sites.google.com/site/burlachenkok/various_way_to_implement-rsqrtx-in-c
            }

            LIB vector_type FUNC(inversesqrt)(vector_arg_type t) {
                return vector_type(rsqrt(t.data[Ns])...);
            }

            //
            // 8.3 Common Functions
            //
            LIB vector_type FUNC(abs)(vector_arg_type t) {
                return vector_type(std::abs(t.data[Ns])...);
            }

            LIB scalar_type sign(scalar_type x) {
                return (zero < x) - (x < zero);
            }

            LIB vector_type FUNC(sign)(vector_arg_type t) {
                return vector_type(sign(t.data[Ns])...);
            }

            LIB vector_type FUNC(floor)(vector_arg_type t) {
                return vector_type(std::floor(t.data[Ns])...);
            }

            LIB vector_type FUNC(trunc)(vector_arg_type t) {
                return vector_type(std::trunc(t.data[Ns])...);
            }

            //TODO: round roundEven

            LIB vector_type FUNC(ceil)(vector_arg_type t) {
                return vector_type(std::ceil(t.data[Ns])...);
            }

            LIB vector_type FUNC(fract)(vector_arg_type t) {
                return vector_type((t.data[Ns] - std::floor(t.data[Ns]))...);
            }

            LIB vector_type FUNC(mod)(vector_arg_type x, scalar_type y) {
                return vector_type((x.data[Ns] - y * std::floor(x.data[Ns] / y))...);
            }

            LIB vector_type FUNC(mod)(vector_arg_type x, vector_arg_type y) {
                return vector_type((x.data[Ns] - y.data[Ns] * std::floor(x.data[Ns] / y.data[Ns]))...);
            }

            LIB vector_type FUNC(min)(vector_arg_type left, vector_arg_type right) {
                return vector_type((left.data[Ns] < right.data[Ns] ? left.data[Ns] : right.data[Ns])...);
            }

            LIB vector_type FUNC(min)(vector_arg_type left, scalar_type right) {
                return vector_type((left.data[Ns] < right ? left.data[Ns] : right)...);
            }

            LIB vector_type FUNC(max)(vector_arg_type left, vector_arg_type right) {
                return vector_type((left.data[Ns] > right.data[Ns] ? left.data[Ns] : right.data[Ns])...);
            }

            LIB vector_type FUNC(max)(vector_arg_type left, scalar_type right) {
                return vector_type((left.data[Ns] > right ? left.data[Ns] : right)...);
            }

            LIB vector_type FUNC(clamp)(vector_arg_type x, vector_arg_type minVal, vector_arg_type maxVal) {
                return FUNC(min)(FUNC(max)(x, minVal), maxVal);
            }

            LIB vector_type FUNC(clamp)(vector_arg_type x, scalar_type minVal, scalar_type maxVal) {
                return FUNC(min)(FUNC(max)(x, minVal), maxVal);
            }

            LIB vector_type FUNC(mix)(vector_arg_type x, vector_arg_type y, vector_arg_type a) {
                return x * (vector_type(scalar_type(1)) - a) + y * a;
            }

            LIB vector_type FUNC(mix)(vector_arg_type x, vector_arg_type y, scalar_type a) {
                return x * (scalar_type(1) - a) + y * a;
            }

            // doens't work in MSVC
            //LIB vector_type FUNC(mix)(vector_arg_type x, vector_arg_type y, const bool_vector_type &a)
            //{
            //	return vector_type((a.data[Ns] ? y.data[Ns] : x.data[Ns])...);
            //}

            LIB vector_type FUNC(step)(scalar_type edge, vector_arg_type x) {
                return vector_type((x.data[Ns] < edge ? scalar_type(0) : scalar_type(1))...);
            }

            LIB vector_type FUNC(step)(vector_arg_type edge, vector_arg_type x) {
                return vector_type((x.data[Ns] < edge.data[Ns] ? scalar_type(0) : scalar_type(1))...);
            }

            LIB vector_type FUNC(smoothstep)(scalar_type edge0, scalar_type edge1, vector_arg_type x) {
                auto t = FUNC(clamp)((x - edge0) / (edge1 - edge0), zero, one);
                return t * t * (scalar_type(3) - scalar_type(2) * t);
            }

            LIB vector_type FUNC(smoothstep)(vector_arg_type edge0, vector_arg_type edge1, vector_arg_type x) {
                auto t = FUNC(clamp)((x - edge0) / (edge1 - edge0), zero, one);
                return t * t * (scalar_type(3) - scalar_type(2) * t);
            }
            //
            // 8.5 Geometric functions
            //
            LIB scalar_type FUNC(length)(vector_arg_type v) {
                return std::sqrt(FUNC(dot)(v, v));
            }

            LIB scalar_type FUNC(distance)(vector_arg_type p0, vector_arg_type p1) {
                return FUNC(length)(p0 - p1);
            }

            LIB vector_type FUNC(normalize)(vector_arg_type v) {
                vector_type out = v;
                out /= FUNC(length)(v);
                return out;
            }

            LIB scalar_type FUNC(dot)(vector_arg_type a, vector_arg_type b) {
                scalar_type sum = 0;
                ((sum += a.data[Ns] * b.data[Ns]), ...);
                return sum;
            }

            LIB vector_type FUNC(cross)(vector_arg_type a, vector_arg_type b) {
                static_assert(vector_type::num_components == 3, "cross product only works for vec3");

                return vector_type(
                        a.y * b.z - a.z * b.y,
                        a.z * b.x - a.x * b.z,
                        a.x * b.y - a.y * b.x);
            }

            LIB vector_type FUNC(faceforward)(vector_arg_type N, vector_arg_type I, vector_arg_type Nref) {
                return (FUNC(dot)(Nref, I) < scalar_type(0) ? N : (-N));
            }

            LIB vector_type FUNC(reflect)(vector_arg_type I, vector_arg_type N) {
                return (I - scalar_type(2) * FUNC(dot)(I, N) * N);
            }

            LIB vector_type FUNC(refract)(vector_arg_type I, vector_arg_type N, scalar_type eta) {
                auto k = one - eta * eta * (one - FUNC(dot)(N, I) * FUNC(dot)(N, I));
                if (k < zero) {
                    return vector_type();
                } else {
                    return eta * I - (eta * FUNC(dot)(N, I) + sqrt(k)) * N;
                }
            }

            //
            // 8.7 Vector Relational Functions
            //
            LIB bool_vector_type FUNC(lessThan)(vector_arg_type x, vector_arg_type y) {
                return bool_vector_type((x.data[Ns] < y.data[Ns])...);
            }

            LIB bool_vector_type FUNC(lessThanEqual)(vector_arg_type x, vector_arg_type y) {
                return bool_vector_type((x.data[Ns] <= y.data[Ns])...);
            }

            LIB bool_vector_type FUNC(greaterThan)(vector_arg_type x, vector_arg_type y) {
                return bool_vector_type((x.data[Ns] > y.data[Ns])...);
            }

            LIB bool_vector_type FUNC(greaterThanEqual)(vector_arg_type x, vector_arg_type y) {
                return bool_vector_type((x.data[Ns] >= y.data[Ns])...);
            }

            LIB bool_vector_type FUNC(equal)(vector_arg_type x, vector_arg_type y) {
                return bool_vector_type((x.data[Ns] == y.data[Ns])...);
            }

            LIB bool_vector_type FUNC(notEqual)(vector_arg_type x, vector_arg_type y) {
                return bool_vector_type((x.data[Ns] != y.data[Ns])...);
            }

            LIB bool FUNC(any)(typename std::conditional<std::is_same<scalar_type, bool>::value, vector_arg_type, nothing>::type b) {
                return (... || b.data[Ns]);
            }

            LIB bool FUNC(all)(typename std::conditional<std::is_same<scalar_type, bool>::value, vector_arg_type, nothing>::type b) {
                return (... && b.data[Ns]);
            }

            LIB bool_vector_type FUNC(_not)(typename std::conditional<std::is_same<scalar_type, bool>::value, vector_arg_type, nothing>::type b) {
                return bool_vector_type((!b.data[Ns])...);
            }

            //
            // 8.13.1 Derivative Functions
            // NOTE: can only fake these
            LIB vector_type FUNC(dFdx)(vector_arg_type p) {
                return p * scalar_type(.01);
            }

            LIB vector_type FUNC(dFdy)(vector_arg_type p) {
                return p * scalar_type(.01);
            }

            LIB vector_type FUNC(fwidth)(vector_arg_type p) {
                return FUNC(abs)(FUNC(dFdx)(p)) + FUNC(abs)(FUNC(dFdy)(p));
            }

#undef LIB
#undef FUNC
        };

    }// namespace detail

    namespace detail {

        template<typename vector_type, typename scalar_type>
        struct binary_vec_ops
        {
#define DEF_OP_BINARY(op, impl_op, a_type, b_type)       \
    friend vector_type operator op(a_type a, b_type b) { \
        auto out = vector_type(a);                       \
        out impl_op b;                                   \
        return out;                                      \
    }

            DEF_OP_BINARY(+, +=, const vector_type &, const vector_type &)
            DEF_OP_BINARY(-, -=, const vector_type &, const vector_type &)
            DEF_OP_BINARY(*, *=, const vector_type &, const vector_type &)
            DEF_OP_BINARY(/, /=, const vector_type &, const vector_type &)
            DEF_OP_BINARY(+, +=, const vector_type &, scalar_type)
            DEF_OP_BINARY(+, +=, scalar_type, const vector_type &)
            DEF_OP_BINARY(-, -=, const vector_type &, scalar_type)
            DEF_OP_BINARY(-, -=, scalar_type, const vector_type &)
            DEF_OP_BINARY(*, *=, const vector_type &, scalar_type)
            DEF_OP_BINARY(*, *=, scalar_type, const vector_type &)
            DEF_OP_BINARY(/, /=, const vector_type &, scalar_type)
            DEF_OP_BINARY(/, /=, scalar_type, const vector_type &)

#undef DEF_OP_BINARY
        };

    }// namespace detail


    template<typename T, size_t... Ns>
    struct vector;

    namespace detail {

        template<typename T, size_t N, template<size_t...> class swizzler_wrapper>
        struct vector_base;

        template<typename, size_t>
        struct vec_equiv;

        template<typename T>
        struct vec_equiv<T, 1>
        {
            using type = vector<T, 0>;
        };

        template<typename T>
        struct vec_equiv<T, 2>
        {
            using type = vector<T, 0, 1>;
        };

        template<typename T>
        struct vec_equiv<T, 3>
        {
            using type = vector<T, 0, 1, 2>;
        };

        template<typename T>
        struct vec_equiv<T, 4>
        {
            using type = vector<T, 0, 1, 2, 3>;
        };

        template<typename T, size_t... Ns>
        struct vector_base_selector
        {
            static_assert(sizeof...(Ns) >= 1, "must have at least 1 component");

            template<size_t... indices>// 0 for x (or r), 1 for y (or g), etc
            struct swizzler_wrapper_factory
            {
                // NOTE: need to pass the equivalent vector type
                // this can be different than the current 'host' vector
                // for ex:
                // .xy is vec2 that is part of a vec3
                // .xy is also vec2 but part of a vec4
                // they need to be same underlying type
                using type = detail::swizzler<
                        typename vec_equiv<T, sizeof...(indices)>::type, T, sizeof...(Ns), indices...>;
            };

            template<size_t x>
            struct swizzler_wrapper_factory<x>
            {
                using type = T;// one component vectors are just scalars
            };

            using base_type = vector_base<T, sizeof...(Ns), swizzler_wrapper_factory>;
        };

        template<typename T, template<size_t...> class swizzler_wrapper>
        struct vector_base<T, 1, swizzler_wrapper>
        {
            union {
                T data[1];

                struct
                {
                    typename swizzler_wrapper<0>::type x;
                };
                struct
                {
                    typename swizzler_wrapper<0>::type r;
                };
                struct
                {
                    typename swizzler_wrapper<0>::type s;
                };

                typename swizzler_wrapper<0, 0>::type xx, rr, ss;
                typename swizzler_wrapper<0, 0, 0>::type xxx, rrr, sss;
                typename swizzler_wrapper<0, 0, 0, 0>::type xxxx, rrrr, ssss;
            };
        };

        template<typename T, template<size_t...> class swizzler_wrapper>
        struct vector_base<T, 2, swizzler_wrapper>
        {
            union {
                T data[2];

                struct
                {
                    typename swizzler_wrapper<0>::type x;
                    typename swizzler_wrapper<1>::type y;
                };
                struct
                {
                    typename swizzler_wrapper<0>::type r;
                    typename swizzler_wrapper<1>::type g;
                };
                struct
                {
                    typename swizzler_wrapper<0>::type s;
                    typename swizzler_wrapper<1>::type t;
                };

                typename swizzler_wrapper<0, 0>::type xx, rr, ss;
                typename swizzler_wrapper<0, 1>::type xy, rg, st;
                typename swizzler_wrapper<1, 0>::type yx, gr, ts;
                typename swizzler_wrapper<1, 1>::type yy, gg, tt;
                typename swizzler_wrapper<0, 0, 0>::type xxx, rrr, sss;
                typename swizzler_wrapper<0, 0, 1>::type xxy, rrg, sst;
                typename swizzler_wrapper<0, 1, 0>::type xyx, rgr, sts;
                typename swizzler_wrapper<0, 1, 1>::type xyy, rgg, stt;
                typename swizzler_wrapper<1, 0, 0>::type yxx, grr, tss;
                typename swizzler_wrapper<1, 0, 1>::type yxy, grg, tst;
                typename swizzler_wrapper<1, 1, 0>::type yyx, ggr, tts;
                typename swizzler_wrapper<1, 1, 1>::type yyy, ggg, ttt;
                typename swizzler_wrapper<0, 0, 0, 0>::type xxxx, rrrr, ssss;
                typename swizzler_wrapper<0, 0, 0, 1>::type xxxy, rrrg, ssst;
                typename swizzler_wrapper<0, 0, 1, 0>::type xxyx, rrgr, ssts;
                typename swizzler_wrapper<0, 0, 1, 1>::type xxyy, rrgg, sstt;
                typename swizzler_wrapper<0, 1, 0, 0>::type xyxx, rgrr, stss;
                typename swizzler_wrapper<0, 1, 0, 1>::type xyxy, rgrg, stst;
                typename swizzler_wrapper<0, 1, 1, 0>::type xyyx, rggr, stts;
                typename swizzler_wrapper<0, 1, 1, 1>::type xyyy, rggg, sttt;
                typename swizzler_wrapper<1, 0, 0, 0>::type yxxx, grrr, tsss;
                typename swizzler_wrapper<1, 0, 0, 1>::type yxxy, grrg, tsst;
                typename swizzler_wrapper<1, 0, 1, 0>::type yxyx, grgr, tsts;
                typename swizzler_wrapper<1, 0, 1, 1>::type yxyy, grgg, tstt;
                typename swizzler_wrapper<1, 1, 0, 0>::type yyxx, ggrr, ttss;
                typename swizzler_wrapper<1, 1, 0, 1>::type yyxy, ggrg, ttst;
                typename swizzler_wrapper<1, 1, 1, 0>::type yyyx, gggr, ttts;
                typename swizzler_wrapper<1, 1, 1, 1>::type yyyy, gggg, tttt;
            };
        };

        template<typename T, template<size_t...> class swizzler_wrapper>
        struct vector_base<T, 3, swizzler_wrapper>
        {
            union {
                T data[3];

                struct
                {
                    typename swizzler_wrapper<0>::type x;
                    typename swizzler_wrapper<1>::type y;
                    typename swizzler_wrapper<2>::type z;
                };
                struct
                {
                    typename swizzler_wrapper<0>::type r;
                    typename swizzler_wrapper<1>::type g;
                    typename swizzler_wrapper<2>::type b;
                };
                struct
                {
                    typename swizzler_wrapper<0>::type s;
                    typename swizzler_wrapper<1>::type t;
                    typename swizzler_wrapper<2>::type p;
                };

                typename swizzler_wrapper<0, 0>::type xx, rr, ss;
                typename swizzler_wrapper<0, 1>::type xy, rg, st;
                typename swizzler_wrapper<0, 2>::type xz, rb, sp;
                typename swizzler_wrapper<1, 0>::type yx, gr, ts;
                typename swizzler_wrapper<1, 1>::type yy, gg, tt;
                typename swizzler_wrapper<1, 2>::type yz, gb, tp;
                typename swizzler_wrapper<2, 0>::type zx, br, ps;
                typename swizzler_wrapper<2, 1>::type zy, bg, pt;
                typename swizzler_wrapper<2, 2>::type zz, bb, pp;
                typename swizzler_wrapper<0, 0, 0>::type xxx, rrr, sss;
                typename swizzler_wrapper<0, 0, 1>::type xxy, rrg, sst;
                typename swizzler_wrapper<0, 0, 2>::type xxz, rrb, ssp;
                typename swizzler_wrapper<0, 1, 0>::type xyx, rgr, sts;
                typename swizzler_wrapper<0, 1, 1>::type xyy, rgg, stt;
                typename swizzler_wrapper<0, 1, 2>::type xyz, rgb, stp;
                typename swizzler_wrapper<0, 2, 0>::type xzx, rbr, sps;
                typename swizzler_wrapper<0, 2, 1>::type xzy, rbg, spt;
                typename swizzler_wrapper<0, 2, 2>::type xzz, rbb, spp;
                typename swizzler_wrapper<1, 0, 0>::type yxx, grr, tss;
                typename swizzler_wrapper<1, 0, 1>::type yxy, grg, tst;
                typename swizzler_wrapper<1, 0, 2>::type yxz, grb, tsp;
                typename swizzler_wrapper<1, 1, 0>::type yyx, ggr, tts;
                typename swizzler_wrapper<1, 1, 1>::type yyy, ggg, ttt;
                typename swizzler_wrapper<1, 1, 2>::type yyz, ggb, ttp;
                typename swizzler_wrapper<1, 2, 0>::type yzx, gbr, tps;
                typename swizzler_wrapper<1, 2, 1>::type yzy, gbg, tpt;
                typename swizzler_wrapper<1, 2, 2>::type yzz, gbb, tpp;
                typename swizzler_wrapper<2, 0, 0>::type zxx, brr, pss;
                typename swizzler_wrapper<2, 0, 1>::type zxy, brg, pst;
                typename swizzler_wrapper<2, 0, 2>::type zxz, brb, psp;
                typename swizzler_wrapper<2, 1, 0>::type zyx, bgr, pts;
                typename swizzler_wrapper<2, 1, 1>::type zyy, bgg, ptt;
                typename swizzler_wrapper<2, 1, 2>::type zyz, bgb, ptp;
                typename swizzler_wrapper<2, 2, 0>::type zzx, bbr, pps;
                typename swizzler_wrapper<2, 2, 1>::type zzy, bbg, ppt;
                typename swizzler_wrapper<2, 2, 2>::type zzz, bbb, ppp;
                typename swizzler_wrapper<0, 0, 0, 0>::type xxxx, rrrr, ssss;
                typename swizzler_wrapper<0, 0, 0, 1>::type xxxy, rrrg, ssst;
                typename swizzler_wrapper<0, 0, 0, 2>::type xxxz, rrrb, sssp;
                typename swizzler_wrapper<0, 0, 1, 0>::type xxyx, rrgr, ssts;
                typename swizzler_wrapper<0, 0, 1, 1>::type xxyy, rrgg, sstt;
                typename swizzler_wrapper<0, 0, 1, 2>::type xxyz, rrgb, sstp;
                typename swizzler_wrapper<0, 0, 2, 0>::type xxzx, rrbr, ssps;
                typename swizzler_wrapper<0, 0, 2, 1>::type xxzy, rrbg, sspt;
                typename swizzler_wrapper<0, 0, 2, 2>::type xxzz, rrbb, sspp;
                typename swizzler_wrapper<0, 1, 0, 0>::type xyxx, rgrr, stss;
                typename swizzler_wrapper<0, 1, 0, 1>::type xyxy, rgrg, stst;
                typename swizzler_wrapper<0, 1, 0, 2>::type xyxz, rgrb, stsp;
                typename swizzler_wrapper<0, 1, 1, 0>::type xyyx, rggr, stts;
                typename swizzler_wrapper<0, 1, 1, 1>::type xyyy, rggg, sttt;
                typename swizzler_wrapper<0, 1, 1, 2>::type xyyz, rggb, sttp;
                typename swizzler_wrapper<0, 1, 2, 0>::type xyzx, rgbr, stps;
                typename swizzler_wrapper<0, 1, 2, 1>::type xyzy, rgbg, stpt;
                typename swizzler_wrapper<0, 1, 2, 2>::type xyzz, rgbb, stpp;
                typename swizzler_wrapper<0, 2, 0, 0>::type xzxx, rbrr, spss;
                typename swizzler_wrapper<0, 2, 0, 1>::type xzxy, rbrg, spst;
                typename swizzler_wrapper<0, 2, 0, 2>::type xzxz, rbrb, spsp;
                typename swizzler_wrapper<0, 2, 1, 0>::type xzyx, rbgr, spts;
                typename swizzler_wrapper<0, 2, 1, 1>::type xzyy, rbgg, sptt;
                typename swizzler_wrapper<0, 2, 1, 2>::type xzyz, rbgb, sptp;
                typename swizzler_wrapper<0, 2, 2, 0>::type xzzx, rbbr, spps;
                typename swizzler_wrapper<0, 2, 2, 1>::type xzzy, rbbg, sppt;
                typename swizzler_wrapper<0, 2, 2, 2>::type xzzz, rbbb, sppp;
                typename swizzler_wrapper<1, 0, 0, 0>::type yxxx, grrr, tsss;
                typename swizzler_wrapper<1, 0, 0, 1>::type yxxy, grrg, tsst;
                typename swizzler_wrapper<1, 0, 0, 2>::type yxxz, grrb, tssp;
                typename swizzler_wrapper<1, 0, 1, 0>::type yxyx, grgr, tsts;
                typename swizzler_wrapper<1, 0, 1, 1>::type yxyy, grgg, tstt;
                typename swizzler_wrapper<1, 0, 1, 2>::type yxyz, grgb, tstp;
                typename swizzler_wrapper<1, 0, 2, 0>::type yxzx, grbr, tsps;
                typename swizzler_wrapper<1, 0, 2, 1>::type yxzy, grbg, tspt;
                typename swizzler_wrapper<1, 0, 2, 2>::type yxzz, grbb, tspp;
                typename swizzler_wrapper<1, 1, 0, 0>::type yyxx, ggrr, ttss;
                typename swizzler_wrapper<1, 1, 0, 1>::type yyxy, ggrg, ttst;
                typename swizzler_wrapper<1, 1, 0, 2>::type yyxz, ggrb, ttsp;
                typename swizzler_wrapper<1, 1, 1, 0>::type yyyx, gggr, ttts;
                typename swizzler_wrapper<1, 1, 1, 1>::type yyyy, gggg, tttt;
                typename swizzler_wrapper<1, 1, 1, 2>::type yyyz, gggb, tttp;
                typename swizzler_wrapper<1, 1, 2, 0>::type yyzx, ggbr, ttps;
                typename swizzler_wrapper<1, 1, 2, 1>::type yyzy, ggbg, ttpt;
                typename swizzler_wrapper<1, 1, 2, 2>::type yyzz, ggbb, ttpp;
                typename swizzler_wrapper<1, 2, 0, 0>::type yzxx, gbrr, tpss;
                typename swizzler_wrapper<1, 2, 0, 1>::type yzxy, gbrg, tpst;
                typename swizzler_wrapper<1, 2, 0, 2>::type yzxz, gbrb, tpsp;
                typename swizzler_wrapper<1, 2, 1, 0>::type yzyx, gbgr, tpts;
                typename swizzler_wrapper<1, 2, 1, 1>::type yzyy, gbgg, tptt;
                typename swizzler_wrapper<1, 2, 1, 2>::type yzyz, gbgb, tptp;
                typename swizzler_wrapper<1, 2, 2, 0>::type yzzx, gbbr, tpps;
                typename swizzler_wrapper<1, 2, 2, 1>::type yzzy, gbbg, tppt;
                typename swizzler_wrapper<1, 2, 2, 2>::type yzzz, gbbb, tppp;
                typename swizzler_wrapper<2, 0, 0, 0>::type zxxx, brrr, psss;
                typename swizzler_wrapper<2, 0, 0, 1>::type zxxy, brrg, psst;
                typename swizzler_wrapper<2, 0, 0, 2>::type zxxz, brrb, pssp;
                typename swizzler_wrapper<2, 0, 1, 0>::type zxyx, brgr, psts;
                typename swizzler_wrapper<2, 0, 1, 1>::type zxyy, brgg, pstt;
                typename swizzler_wrapper<2, 0, 1, 2>::type zxyz, brgb, pstp;
                typename swizzler_wrapper<2, 0, 2, 0>::type zxzx, brbr, psps;
                typename swizzler_wrapper<2, 0, 2, 1>::type zxzy, brbg, pspt;
                typename swizzler_wrapper<2, 0, 2, 2>::type zxzz, brbb, pspp;
                typename swizzler_wrapper<2, 1, 0, 0>::type zyxx, bgrr, ptss;
                typename swizzler_wrapper<2, 1, 0, 1>::type zyxy, bgrg, ptst;
                typename swizzler_wrapper<2, 1, 0, 2>::type zyxz, bgrb, ptsp;
                typename swizzler_wrapper<2, 1, 1, 0>::type zyyx, bggr, ptts;
                typename swizzler_wrapper<2, 1, 1, 1>::type zyyy, bggg, pttt;
                typename swizzler_wrapper<2, 1, 1, 2>::type zyyz, bggb, pttp;
                typename swizzler_wrapper<2, 1, 2, 0>::type zyzx, bgbr, ptps;
                typename swizzler_wrapper<2, 1, 2, 1>::type zyzy, bgbg, ptpt;
                typename swizzler_wrapper<2, 1, 2, 2>::type zyzz, bgbb, ptpp;
                typename swizzler_wrapper<2, 2, 0, 0>::type zzxx, bbrr, ppss;
                typename swizzler_wrapper<2, 2, 0, 1>::type zzxy, bbrg, ppst;
                typename swizzler_wrapper<2, 2, 0, 2>::type zzxz, bbrb, ppsp;
                typename swizzler_wrapper<2, 2, 1, 0>::type zzyx, bbgr, ppts;
                typename swizzler_wrapper<2, 2, 1, 1>::type zzyy, bbgg, pptt;
                typename swizzler_wrapper<2, 2, 1, 2>::type zzyz, bbgb, pptp;
                typename swizzler_wrapper<2, 2, 2, 0>::type zzzx, bbbr, ppps;
                typename swizzler_wrapper<2, 2, 2, 1>::type zzzy, bbbg, pppt;
                typename swizzler_wrapper<2, 2, 2, 2>::type zzzz, bbbb, pppp;
            };
        };

        template<typename T, template<size_t...> class swizzler_wrapper>
        struct vector_base<T, 4, swizzler_wrapper>
        {
            union {
                T data[4];

                struct
                {
                    typename swizzler_wrapper<0>::type x;
                    typename swizzler_wrapper<1>::type y;
                    typename swizzler_wrapper<2>::type z;
                    typename swizzler_wrapper<3>::type w;
                };
                struct
                {
                    typename swizzler_wrapper<0>::type r;
                    typename swizzler_wrapper<1>::type g;
                    typename swizzler_wrapper<2>::type b;
                    typename swizzler_wrapper<3>::type a;
                };
                struct
                {
                    typename swizzler_wrapper<0>::type s;
                    typename swizzler_wrapper<1>::type t;
                    typename swizzler_wrapper<2>::type p;
                    typename swizzler_wrapper<3>::type q;
                };

                typename swizzler_wrapper<0, 0>::type xx, rr, ss;
                typename swizzler_wrapper<0, 1>::type xy, rg, st;
                typename swizzler_wrapper<0, 2>::type xz, rb, sp;
                typename swizzler_wrapper<0, 3>::type xw, ra, sq;
                typename swizzler_wrapper<1, 0>::type yx, gr, ts;
                typename swizzler_wrapper<1, 1>::type yy, gg, tt;
                typename swizzler_wrapper<1, 2>::type yz, gb, tp;
                typename swizzler_wrapper<1, 3>::type yw, ga, tq;
                typename swizzler_wrapper<2, 0>::type zx, br, ps;
                typename swizzler_wrapper<2, 1>::type zy, bg, pt;
                typename swizzler_wrapper<2, 2>::type zz, bb, pp;
                typename swizzler_wrapper<2, 3>::type zw, ba, pq;
                typename swizzler_wrapper<3, 0>::type wx, ar, qs;
                typename swizzler_wrapper<3, 1>::type wy, ag, qt;
                typename swizzler_wrapper<3, 2>::type wz, ab, qp;
                typename swizzler_wrapper<3, 3>::type ww, aa, qq;
                typename swizzler_wrapper<0, 0, 0>::type xxx, rrr, sss;
                typename swizzler_wrapper<0, 0, 1>::type xxy, rrg, sst;
                typename swizzler_wrapper<0, 0, 2>::type xxz, rrb, ssp;
                typename swizzler_wrapper<0, 0, 3>::type xxw, rra, ssq;
                typename swizzler_wrapper<0, 1, 0>::type xyx, rgr, sts;
                typename swizzler_wrapper<0, 1, 1>::type xyy, rgg, stt;
                typename swizzler_wrapper<0, 1, 2>::type xyz, rgb, stp;
                typename swizzler_wrapper<0, 1, 3>::type xyw, rga, stq;
                typename swizzler_wrapper<0, 2, 0>::type xzx, rbr, sps;
                typename swizzler_wrapper<0, 2, 1>::type xzy, rbg, spt;
                typename swizzler_wrapper<0, 2, 2>::type xzz, rbb, spp;
                typename swizzler_wrapper<0, 2, 3>::type xzw, rba, spq;
                typename swizzler_wrapper<0, 3, 0>::type xwx, rar, sqs;
                typename swizzler_wrapper<0, 3, 1>::type xwy, rag, sqt;
                typename swizzler_wrapper<0, 3, 2>::type xwz, rab, sqp;
                typename swizzler_wrapper<0, 3, 3>::type xww, raa, sqq;
                typename swizzler_wrapper<1, 0, 0>::type yxx, grr, tss;
                typename swizzler_wrapper<1, 0, 1>::type yxy, grg, tst;
                typename swizzler_wrapper<1, 0, 2>::type yxz, grb, tsp;
                typename swizzler_wrapper<1, 0, 3>::type yxw, gra, tsq;
                typename swizzler_wrapper<1, 1, 0>::type yyx, ggr, tts;
                typename swizzler_wrapper<1, 1, 1>::type yyy, ggg, ttt;
                typename swizzler_wrapper<1, 1, 2>::type yyz, ggb, ttp;
                typename swizzler_wrapper<1, 1, 3>::type yyw, gga, ttq;
                typename swizzler_wrapper<1, 2, 0>::type yzx, gbr, tps;
                typename swizzler_wrapper<1, 2, 1>::type yzy, gbg, tpt;
                typename swizzler_wrapper<1, 2, 2>::type yzz, gbb, tpp;
                typename swizzler_wrapper<1, 2, 3>::type yzw, gba, tpq;
                typename swizzler_wrapper<1, 3, 0>::type ywx, gar, tqs;
                typename swizzler_wrapper<1, 3, 1>::type ywy, gag, tqt;
                typename swizzler_wrapper<1, 3, 2>::type ywz, gab, tqp;
                typename swizzler_wrapper<1, 3, 3>::type yww, gaa, tqq;
                typename swizzler_wrapper<2, 0, 0>::type zxx, brr, pss;
                typename swizzler_wrapper<2, 0, 1>::type zxy, brg, pst;
                typename swizzler_wrapper<2, 0, 2>::type zxz, brb, psp;
                typename swizzler_wrapper<2, 0, 3>::type zxw, bra, psq;
                typename swizzler_wrapper<2, 1, 0>::type zyx, bgr, pts;
                typename swizzler_wrapper<2, 1, 1>::type zyy, bgg, ptt;
                typename swizzler_wrapper<2, 1, 2>::type zyz, bgb, ptp;
                typename swizzler_wrapper<2, 1, 3>::type zyw, bga, ptq;
                typename swizzler_wrapper<2, 2, 0>::type zzx, bbr, pps;
                typename swizzler_wrapper<2, 2, 1>::type zzy, bbg, ppt;
                typename swizzler_wrapper<2, 2, 2>::type zzz, bbb, ppp;
                typename swizzler_wrapper<2, 2, 3>::type zzw, bba, ppq;
                typename swizzler_wrapper<2, 3, 0>::type zwx, bar, pqs;
                typename swizzler_wrapper<2, 3, 1>::type zwy, bag, pqt;
                typename swizzler_wrapper<2, 3, 2>::type zwz, bab, pqp;
                typename swizzler_wrapper<2, 3, 3>::type zww, baa, pqq;
                typename swizzler_wrapper<3, 0, 0>::type wxx, arr, qss;
                typename swizzler_wrapper<3, 0, 1>::type wxy, arg, qst;
                typename swizzler_wrapper<3, 0, 2>::type wxz, arb, qsp;
                typename swizzler_wrapper<3, 0, 3>::type wxw, ara, qsq;
                typename swizzler_wrapper<3, 1, 0>::type wyx, agr, qts;
                typename swizzler_wrapper<3, 1, 1>::type wyy, agg, qtt;
                typename swizzler_wrapper<3, 1, 2>::type wyz, agb, qtp;
                typename swizzler_wrapper<3, 1, 3>::type wyw, aga, qtq;
                typename swizzler_wrapper<3, 2, 0>::type wzx, abr, qps;
                typename swizzler_wrapper<3, 2, 1>::type wzy, abg, qpt;
                typename swizzler_wrapper<3, 2, 2>::type wzz, abb, qpp;
                typename swizzler_wrapper<3, 2, 3>::type wzw, aba, qpq;
                typename swizzler_wrapper<3, 3, 0>::type wwx, aar, qqs;
                typename swizzler_wrapper<3, 3, 1>::type wwy, aag, qqt;
                typename swizzler_wrapper<3, 3, 2>::type wwz, aab, qqp;
                typename swizzler_wrapper<3, 3, 3>::type www, aaa, qqq;
                typename swizzler_wrapper<0, 0, 0, 0>::type xxxx, rrrr, ssss;
                typename swizzler_wrapper<0, 0, 0, 1>::type xxxy, rrrg, ssst;
                typename swizzler_wrapper<0, 0, 0, 2>::type xxxz, rrrb, sssp;
                typename swizzler_wrapper<0, 0, 0, 3>::type xxxw, rrra, sssq;
                typename swizzler_wrapper<0, 0, 1, 0>::type xxyx, rrgr, ssts;
                typename swizzler_wrapper<0, 0, 1, 1>::type xxyy, rrgg, sstt;
                typename swizzler_wrapper<0, 0, 1, 2>::type xxyz, rrgb, sstp;
                typename swizzler_wrapper<0, 0, 1, 3>::type xxyw, rrga, sstq;
                typename swizzler_wrapper<0, 0, 2, 0>::type xxzx, rrbr, ssps;
                typename swizzler_wrapper<0, 0, 2, 1>::type xxzy, rrbg, sspt;
                typename swizzler_wrapper<0, 0, 2, 2>::type xxzz, rrbb, sspp;
                typename swizzler_wrapper<0, 0, 2, 3>::type xxzw, rrba, sspq;
                typename swizzler_wrapper<0, 0, 3, 0>::type xxwx, rrar, ssqs;
                typename swizzler_wrapper<0, 0, 3, 1>::type xxwy, rrag, ssqt;
                typename swizzler_wrapper<0, 0, 3, 2>::type xxwz, rrab, ssqp;
                typename swizzler_wrapper<0, 0, 3, 3>::type xxww, rraa, ssqq;
                typename swizzler_wrapper<0, 1, 0, 0>::type xyxx, rgrr, stss;
                typename swizzler_wrapper<0, 1, 0, 1>::type xyxy, rgrg, stst;
                typename swizzler_wrapper<0, 1, 0, 2>::type xyxz, rgrb, stsp;
                typename swizzler_wrapper<0, 1, 0, 3>::type xyxw, rgra, stsq;
                typename swizzler_wrapper<0, 1, 1, 0>::type xyyx, rggr, stts;
                typename swizzler_wrapper<0, 1, 1, 1>::type xyyy, rggg, sttt;
                typename swizzler_wrapper<0, 1, 1, 2>::type xyyz, rggb, sttp;
                typename swizzler_wrapper<0, 1, 1, 3>::type xyyw, rgga, sttq;
                typename swizzler_wrapper<0, 1, 2, 0>::type xyzx, rgbr, stps;
                typename swizzler_wrapper<0, 1, 2, 1>::type xyzy, rgbg, stpt;
                typename swizzler_wrapper<0, 1, 2, 2>::type xyzz, rgbb, stpp;
                typename swizzler_wrapper<0, 1, 2, 3>::type xyzw, rgba, stpq;
                typename swizzler_wrapper<0, 1, 3, 0>::type xywx, rgar, stqs;
                typename swizzler_wrapper<0, 1, 3, 1>::type xywy, rgag, stqt;
                typename swizzler_wrapper<0, 1, 3, 2>::type xywz, rgab, stqp;
                typename swizzler_wrapper<0, 1, 3, 3>::type xyww, rgaa, stqq;
                typename swizzler_wrapper<0, 2, 0, 0>::type xzxx, rbrr, spss;
                typename swizzler_wrapper<0, 2, 0, 1>::type xzxy, rbrg, spst;
                typename swizzler_wrapper<0, 2, 0, 2>::type xzxz, rbrb, spsp;
                typename swizzler_wrapper<0, 2, 0, 3>::type xzxw, rbra, spsq;
                typename swizzler_wrapper<0, 2, 1, 0>::type xzyx, rbgr, spts;
                typename swizzler_wrapper<0, 2, 1, 1>::type xzyy, rbgg, sptt;
                typename swizzler_wrapper<0, 2, 1, 2>::type xzyz, rbgb, sptp;
                typename swizzler_wrapper<0, 2, 1, 3>::type xzyw, rbga, sptq;
                typename swizzler_wrapper<0, 2, 2, 0>::type xzzx, rbbr, spps;
                typename swizzler_wrapper<0, 2, 2, 1>::type xzzy, rbbg, sppt;
                typename swizzler_wrapper<0, 2, 2, 2>::type xzzz, rbbb, sppp;
                typename swizzler_wrapper<0, 2, 2, 3>::type xzzw, rbba, sppq;
                typename swizzler_wrapper<0, 2, 3, 0>::type xzwx, rbar, spqs;
                typename swizzler_wrapper<0, 2, 3, 1>::type xzwy, rbag, spqt;
                typename swizzler_wrapper<0, 2, 3, 2>::type xzwz, rbab, spqp;
                typename swizzler_wrapper<0, 2, 3, 3>::type xzww, rbaa, spqq;
                typename swizzler_wrapper<0, 3, 0, 0>::type xwxx, rarr, sqss;
                typename swizzler_wrapper<0, 3, 0, 1>::type xwxy, rarg, sqst;
                typename swizzler_wrapper<0, 3, 0, 2>::type xwxz, rarb, sqsp;
                typename swizzler_wrapper<0, 3, 0, 3>::type xwxw, rara, sqsq;
                typename swizzler_wrapper<0, 3, 1, 0>::type xwyx, ragr, sqts;
                typename swizzler_wrapper<0, 3, 1, 1>::type xwyy, ragg, sqtt;
                typename swizzler_wrapper<0, 3, 1, 2>::type xwyz, ragb, sqtp;
                typename swizzler_wrapper<0, 3, 1, 3>::type xwyw, raga, sqtq;
                typename swizzler_wrapper<0, 3, 2, 0>::type xwzx, rabr, sqps;
                typename swizzler_wrapper<0, 3, 2, 1>::type xwzy, rabg, sqpt;
                typename swizzler_wrapper<0, 3, 2, 2>::type xwzz, rabb, sqpp;
                typename swizzler_wrapper<0, 3, 2, 3>::type xwzw, raba, sqpq;
                typename swizzler_wrapper<0, 3, 3, 0>::type xwwx, raar, sqqs;
                typename swizzler_wrapper<0, 3, 3, 1>::type xwwy, raag, sqqt;
                typename swizzler_wrapper<0, 3, 3, 2>::type xwwz, raab, sqqp;
                typename swizzler_wrapper<0, 3, 3, 3>::type xwww, raaa, sqqq;
                typename swizzler_wrapper<1, 0, 0, 0>::type yxxx, grrr, tsss;
                typename swizzler_wrapper<1, 0, 0, 1>::type yxxy, grrg, tsst;
                typename swizzler_wrapper<1, 0, 0, 2>::type yxxz, grrb, tssp;
                typename swizzler_wrapper<1, 0, 0, 3>::type yxxw, grra, tssq;
                typename swizzler_wrapper<1, 0, 1, 0>::type yxyx, grgr, tsts;
                typename swizzler_wrapper<1, 0, 1, 1>::type yxyy, grgg, tstt;
                typename swizzler_wrapper<1, 0, 1, 2>::type yxyz, grgb, tstp;
                typename swizzler_wrapper<1, 0, 1, 3>::type yxyw, grga, tstq;
                typename swizzler_wrapper<1, 0, 2, 0>::type yxzx, grbr, tsps;
                typename swizzler_wrapper<1, 0, 2, 1>::type yxzy, grbg, tspt;
                typename swizzler_wrapper<1, 0, 2, 2>::type yxzz, grbb, tspp;
                typename swizzler_wrapper<1, 0, 2, 3>::type yxzw, grba, tspq;
                typename swizzler_wrapper<1, 0, 3, 0>::type yxwx, grar, tsqs;
                typename swizzler_wrapper<1, 0, 3, 1>::type yxwy, grag, tsqt;
                typename swizzler_wrapper<1, 0, 3, 2>::type yxwz, grab, tsqp;
                typename swizzler_wrapper<1, 0, 3, 3>::type yxww, graa, tsqq;
                typename swizzler_wrapper<1, 1, 0, 0>::type yyxx, ggrr, ttss;
                typename swizzler_wrapper<1, 1, 0, 1>::type yyxy, ggrg, ttst;
                typename swizzler_wrapper<1, 1, 0, 2>::type yyxz, ggrb, ttsp;
                typename swizzler_wrapper<1, 1, 0, 3>::type yyxw, ggra, ttsq;
                typename swizzler_wrapper<1, 1, 1, 0>::type yyyx, gggr, ttts;
                typename swizzler_wrapper<1, 1, 1, 1>::type yyyy, gggg, tttt;
                typename swizzler_wrapper<1, 1, 1, 2>::type yyyz, gggb, tttp;
                typename swizzler_wrapper<1, 1, 1, 3>::type yyyw, ggga, tttq;
                typename swizzler_wrapper<1, 1, 2, 0>::type yyzx, ggbr, ttps;
                typename swizzler_wrapper<1, 1, 2, 1>::type yyzy, ggbg, ttpt;
                typename swizzler_wrapper<1, 1, 2, 2>::type yyzz, ggbb, ttpp;
                typename swizzler_wrapper<1, 1, 2, 3>::type yyzw, ggba, ttpq;
                typename swizzler_wrapper<1, 1, 3, 0>::type yywx, ggar, ttqs;
                typename swizzler_wrapper<1, 1, 3, 1>::type yywy, ggag, ttqt;
                typename swizzler_wrapper<1, 1, 3, 2>::type yywz, ggab, ttqp;
                typename swizzler_wrapper<1, 1, 3, 3>::type yyww, ggaa, ttqq;
                typename swizzler_wrapper<1, 2, 0, 0>::type yzxx, gbrr, tpss;
                typename swizzler_wrapper<1, 2, 0, 1>::type yzxy, gbrg, tpst;
                typename swizzler_wrapper<1, 2, 0, 2>::type yzxz, gbrb, tpsp;
                typename swizzler_wrapper<1, 2, 0, 3>::type yzxw, gbra, tpsq;
                typename swizzler_wrapper<1, 2, 1, 0>::type yzyx, gbgr, tpts;
                typename swizzler_wrapper<1, 2, 1, 1>::type yzyy, gbgg, tptt;
                typename swizzler_wrapper<1, 2, 1, 2>::type yzyz, gbgb, tptp;
                typename swizzler_wrapper<1, 2, 1, 3>::type yzyw, gbga, tptq;
                typename swizzler_wrapper<1, 2, 2, 0>::type yzzx, gbbr, tpps;
                typename swizzler_wrapper<1, 2, 2, 1>::type yzzy, gbbg, tppt;
                typename swizzler_wrapper<1, 2, 2, 2>::type yzzz, gbbb, tppp;
                typename swizzler_wrapper<1, 2, 2, 3>::type yzzw, gbba, tppq;
                typename swizzler_wrapper<1, 2, 3, 0>::type yzwx, gbar, tpqs;
                typename swizzler_wrapper<1, 2, 3, 1>::type yzwy, gbag, tpqt;
                typename swizzler_wrapper<1, 2, 3, 2>::type yzwz, gbab, tpqp;
                typename swizzler_wrapper<1, 2, 3, 3>::type yzww, gbaa, tpqq;
                typename swizzler_wrapper<1, 3, 0, 0>::type ywxx, garr, tqss;
                typename swizzler_wrapper<1, 3, 0, 1>::type ywxy, garg, tqst;
                typename swizzler_wrapper<1, 3, 0, 2>::type ywxz, garb, tqsp;
                typename swizzler_wrapper<1, 3, 0, 3>::type ywxw, gara, tqsq;
                typename swizzler_wrapper<1, 3, 1, 0>::type ywyx, gagr, tqts;
                typename swizzler_wrapper<1, 3, 1, 1>::type ywyy, gagg, tqtt;
                typename swizzler_wrapper<1, 3, 1, 2>::type ywyz, gagb, tqtp;
                typename swizzler_wrapper<1, 3, 1, 3>::type ywyw, gaga, tqtq;
                typename swizzler_wrapper<1, 3, 2, 0>::type ywzx, gabr, tqps;
                typename swizzler_wrapper<1, 3, 2, 1>::type ywzy, gabg, tqpt;
                typename swizzler_wrapper<1, 3, 2, 2>::type ywzz, gabb, tqpp;
                typename swizzler_wrapper<1, 3, 2, 3>::type ywzw, gaba, tqpq;
                typename swizzler_wrapper<1, 3, 3, 0>::type ywwx, gaar, tqqs;
                typename swizzler_wrapper<1, 3, 3, 1>::type ywwy, gaag, tqqt;
                typename swizzler_wrapper<1, 3, 3, 2>::type ywwz, gaab, tqqp;
                typename swizzler_wrapper<1, 3, 3, 3>::type ywww, gaaa, tqqq;
                typename swizzler_wrapper<2, 0, 0, 0>::type zxxx, brrr, psss;
                typename swizzler_wrapper<2, 0, 0, 1>::type zxxy, brrg, psst;
                typename swizzler_wrapper<2, 0, 0, 2>::type zxxz, brrb, pssp;
                typename swizzler_wrapper<2, 0, 0, 3>::type zxxw, brra, pssq;
                typename swizzler_wrapper<2, 0, 1, 0>::type zxyx, brgr, psts;
                typename swizzler_wrapper<2, 0, 1, 1>::type zxyy, brgg, pstt;
                typename swizzler_wrapper<2, 0, 1, 2>::type zxyz, brgb, pstp;
                typename swizzler_wrapper<2, 0, 1, 3>::type zxyw, brga, pstq;
                typename swizzler_wrapper<2, 0, 2, 0>::type zxzx, brbr, psps;
                typename swizzler_wrapper<2, 0, 2, 1>::type zxzy, brbg, pspt;
                typename swizzler_wrapper<2, 0, 2, 2>::type zxzz, brbb, pspp;
                typename swizzler_wrapper<2, 0, 2, 3>::type zxzw, brba, pspq;
                typename swizzler_wrapper<2, 0, 3, 0>::type zxwx, brar, psqs;
                typename swizzler_wrapper<2, 0, 3, 1>::type zxwy, brag, psqt;
                typename swizzler_wrapper<2, 0, 3, 2>::type zxwz, brab, psqp;
                typename swizzler_wrapper<2, 0, 3, 3>::type zxww, braa, psqq;
                typename swizzler_wrapper<2, 1, 0, 0>::type zyxx, bgrr, ptss;
                typename swizzler_wrapper<2, 1, 0, 1>::type zyxy, bgrg, ptst;
                typename swizzler_wrapper<2, 1, 0, 2>::type zyxz, bgrb, ptsp;
                typename swizzler_wrapper<2, 1, 0, 3>::type zyxw, bgra, ptsq;
                typename swizzler_wrapper<2, 1, 1, 0>::type zyyx, bggr, ptts;
                typename swizzler_wrapper<2, 1, 1, 1>::type zyyy, bggg, pttt;
                typename swizzler_wrapper<2, 1, 1, 2>::type zyyz, bggb, pttp;
                typename swizzler_wrapper<2, 1, 1, 3>::type zyyw, bgga, pttq;
                typename swizzler_wrapper<2, 1, 2, 0>::type zyzx, bgbr, ptps;
                typename swizzler_wrapper<2, 1, 2, 1>::type zyzy, bgbg, ptpt;
                typename swizzler_wrapper<2, 1, 2, 2>::type zyzz, bgbb, ptpp;
                typename swizzler_wrapper<2, 1, 2, 3>::type zyzw, bgba, ptpq;
                typename swizzler_wrapper<2, 1, 3, 0>::type zywx, bgar, ptqs;
                typename swizzler_wrapper<2, 1, 3, 1>::type zywy, bgag, ptqt;
                typename swizzler_wrapper<2, 1, 3, 2>::type zywz, bgab, ptqp;
                typename swizzler_wrapper<2, 1, 3, 3>::type zyww, bgaa, ptqq;
                typename swizzler_wrapper<2, 2, 0, 0>::type zzxx, bbrr, ppss;
                typename swizzler_wrapper<2, 2, 0, 1>::type zzxy, bbrg, ppst;
                typename swizzler_wrapper<2, 2, 0, 2>::type zzxz, bbrb, ppsp;
                typename swizzler_wrapper<2, 2, 0, 3>::type zzxw, bbra, ppsq;
                typename swizzler_wrapper<2, 2, 1, 0>::type zzyx, bbgr, ppts;
                typename swizzler_wrapper<2, 2, 1, 1>::type zzyy, bbgg, pptt;
                typename swizzler_wrapper<2, 2, 1, 2>::type zzyz, bbgb, pptp;
                typename swizzler_wrapper<2, 2, 1, 3>::type zzyw, bbga, pptq;
                typename swizzler_wrapper<2, 2, 2, 0>::type zzzx, bbbr, ppps;
                typename swizzler_wrapper<2, 2, 2, 1>::type zzzy, bbbg, pppt;
                typename swizzler_wrapper<2, 2, 2, 2>::type zzzz, bbbb, pppp;
                typename swizzler_wrapper<2, 2, 2, 3>::type zzzw, bbba, pppq;
                typename swizzler_wrapper<2, 2, 3, 0>::type zzwx, bbar, ppqs;
                typename swizzler_wrapper<2, 2, 3, 1>::type zzwy, bbag, ppqt;
                typename swizzler_wrapper<2, 2, 3, 2>::type zzwz, bbab, ppqp;
                typename swizzler_wrapper<2, 2, 3, 3>::type zzww, bbaa, ppqq;
                typename swizzler_wrapper<2, 3, 0, 0>::type zwxx, barr, pqss;
                typename swizzler_wrapper<2, 3, 0, 1>::type zwxy, barg, pqst;
                typename swizzler_wrapper<2, 3, 0, 2>::type zwxz, barb, pqsp;
                typename swizzler_wrapper<2, 3, 0, 3>::type zwxw, bara, pqsq;
                typename swizzler_wrapper<2, 3, 1, 0>::type zwyx, bagr, pqts;
                typename swizzler_wrapper<2, 3, 1, 1>::type zwyy, bagg, pqtt;
                typename swizzler_wrapper<2, 3, 1, 2>::type zwyz, bagb, pqtp;
                typename swizzler_wrapper<2, 3, 1, 3>::type zwyw, baga, pqtq;
                typename swizzler_wrapper<2, 3, 2, 0>::type zwzx, babr, pqps;
                typename swizzler_wrapper<2, 3, 2, 1>::type zwzy, babg, pqpt;
                typename swizzler_wrapper<2, 3, 2, 2>::type zwzz, babb, pqpp;
                typename swizzler_wrapper<2, 3, 2, 3>::type zwzw, baba, pqpq;
                typename swizzler_wrapper<2, 3, 3, 0>::type zwwx, baar, pqqs;
                typename swizzler_wrapper<2, 3, 3, 1>::type zwwy, baag, pqqt;
                typename swizzler_wrapper<2, 3, 3, 2>::type zwwz, baab, pqqp;
                typename swizzler_wrapper<2, 3, 3, 3>::type zwww, baaa, pqqq;
                typename swizzler_wrapper<3, 0, 0, 0>::type wxxx, arrr, qsss;
                typename swizzler_wrapper<3, 0, 0, 1>::type wxxy, arrg, qsst;
                typename swizzler_wrapper<3, 0, 0, 2>::type wxxz, arrb, qssp;
                typename swizzler_wrapper<3, 0, 0, 3>::type wxxw, arra, qssq;
                typename swizzler_wrapper<3, 0, 1, 0>::type wxyx, argr, qsts;
                typename swizzler_wrapper<3, 0, 1, 1>::type wxyy, argg, qstt;
                typename swizzler_wrapper<3, 0, 1, 2>::type wxyz, argb, qstp;
                typename swizzler_wrapper<3, 0, 1, 3>::type wxyw, arga, qstq;
                typename swizzler_wrapper<3, 0, 2, 0>::type wxzx, arbr, qsps;
                typename swizzler_wrapper<3, 0, 2, 1>::type wxzy, arbg, qspt;
                typename swizzler_wrapper<3, 0, 2, 2>::type wxzz, arbb, qspp;
                typename swizzler_wrapper<3, 0, 2, 3>::type wxzw, arba, qspq;
                typename swizzler_wrapper<3, 0, 3, 0>::type wxwx, arar, qsqs;
                typename swizzler_wrapper<3, 0, 3, 1>::type wxwy, arag, qsqt;
                typename swizzler_wrapper<3, 0, 3, 2>::type wxwz, arab, qsqp;
                typename swizzler_wrapper<3, 0, 3, 3>::type wxww, araa, qsqq;
                typename swizzler_wrapper<3, 1, 0, 0>::type wyxx, agrr, qtss;
                typename swizzler_wrapper<3, 1, 0, 1>::type wyxy, agrg, qtst;
                typename swizzler_wrapper<3, 1, 0, 2>::type wyxz, agrb, qtsp;
                typename swizzler_wrapper<3, 1, 0, 3>::type wyxw, agra, qtsq;
                typename swizzler_wrapper<3, 1, 1, 0>::type wyyx, aggr, qtts;
                typename swizzler_wrapper<3, 1, 1, 1>::type wyyy, aggg, qttt;
                typename swizzler_wrapper<3, 1, 1, 2>::type wyyz, aggb, qttp;
                typename swizzler_wrapper<3, 1, 1, 3>::type wyyw, agga, qttq;
                typename swizzler_wrapper<3, 1, 2, 0>::type wyzx, agbr, qtps;
                typename swizzler_wrapper<3, 1, 2, 1>::type wyzy, agbg, qtpt;
                typename swizzler_wrapper<3, 1, 2, 2>::type wyzz, agbb, qtpp;
                typename swizzler_wrapper<3, 1, 2, 3>::type wyzw, agba, qtpq;
                typename swizzler_wrapper<3, 1, 3, 0>::type wywx, agar, qtqs;
                typename swizzler_wrapper<3, 1, 3, 1>::type wywy, agag, qtqt;
                typename swizzler_wrapper<3, 1, 3, 2>::type wywz, agab, qtqp;
                typename swizzler_wrapper<3, 1, 3, 3>::type wyww, agaa, qtqq;
                typename swizzler_wrapper<3, 2, 0, 0>::type wzxx, abrr, qpss;
                typename swizzler_wrapper<3, 2, 0, 1>::type wzxy, abrg, qpst;
                typename swizzler_wrapper<3, 2, 0, 2>::type wzxz, abrb, qpsp;
                typename swizzler_wrapper<3, 2, 0, 3>::type wzxw, abra, qpsq;
                typename swizzler_wrapper<3, 2, 1, 0>::type wzyx, abgr, qpts;
                typename swizzler_wrapper<3, 2, 1, 1>::type wzyy, abgg, qptt;
                typename swizzler_wrapper<3, 2, 1, 2>::type wzyz, abgb, qptp;
                typename swizzler_wrapper<3, 2, 1, 3>::type wzyw, abga, qptq;
                typename swizzler_wrapper<3, 2, 2, 0>::type wzzx, abbr, qpps;
                typename swizzler_wrapper<3, 2, 2, 1>::type wzzy, abbg, qppt;
                typename swizzler_wrapper<3, 2, 2, 2>::type wzzz, abbb, qppp;
                typename swizzler_wrapper<3, 2, 2, 3>::type wzzw, abba, qppq;
                typename swizzler_wrapper<3, 2, 3, 0>::type wzwx, abar, qpqs;
                typename swizzler_wrapper<3, 2, 3, 1>::type wzwy, abag, qpqt;
                typename swizzler_wrapper<3, 2, 3, 2>::type wzwz, abab, qpqp;
                typename swizzler_wrapper<3, 2, 3, 3>::type wzww, abaa, qpqq;
                typename swizzler_wrapper<3, 3, 0, 0>::type wwxx, aarr, qqss;
                typename swizzler_wrapper<3, 3, 0, 1>::type wwxy, aarg, qqst;
                typename swizzler_wrapper<3, 3, 0, 2>::type wwxz, aarb, qqsp;
                typename swizzler_wrapper<3, 3, 0, 3>::type wwxw, aara, qqsq;
                typename swizzler_wrapper<3, 3, 1, 0>::type wwyx, aagr, qqts;
                typename swizzler_wrapper<3, 3, 1, 1>::type wwyy, aagg, qqtt;
                typename swizzler_wrapper<3, 3, 1, 2>::type wwyz, aagb, qqtp;
                typename swizzler_wrapper<3, 3, 1, 3>::type wwyw, aaga, qqtq;
                typename swizzler_wrapper<3, 3, 2, 0>::type wwzx, aabr, qqps;
                typename swizzler_wrapper<3, 3, 2, 1>::type wwzy, aabg, qqpt;
                typename swizzler_wrapper<3, 3, 2, 2>::type wwzz, aabb, qqpp;
                typename swizzler_wrapper<3, 3, 2, 3>::type wwzw, aaba, qqpq;
                typename swizzler_wrapper<3, 3, 3, 0>::type wwwx, aaar, qqqs;
                typename swizzler_wrapper<3, 3, 3, 1>::type wwwy, aaag, qqqt;
                typename swizzler_wrapper<3, 3, 3, 2>::type wwwz, aaab, qqqp;
                typename swizzler_wrapper<3, 3, 3, 3>::type wwww, aaaa, qqqq;
            };
        };

    }// namespace detail

#ifdef _MSC_VER
#define _MSC_FIX_EBO __declspec(empty_bases)// https://blogs.msdn.microsoft.com/vcblog/2016/03/30/optimizing-the-layout-of-empty-base-classes-in-vs2015-update-2-3/
#else
#define _MSC_FIX_EBO
#endif

    template<typename T, size_t... Ns>
    struct _MSC_FIX_EBO vector : public detail::vector_base_selector<T, Ns...>::base_type,
                                 public detail::builtin_func_lib<vector, T, Ns...>,
                                 public std::conditional<sizeof...(Ns) != 1,// no binary ops for promoted scalar
                                                         detail::binary_vec_ops<vector<T, Ns...>, T>, detail::nothing>::type
    {
        static constexpr auto num_components = sizeof...(Ns);

        using scalar_type = T;
        using vector_type = vector<T, Ns...>;
        using base_type = typename detail::vector_base_selector<T, Ns...>::base_type;
        using decay_type = typename std::conditional<num_components == 1, scalar_type, vector_type>::type;

        // bring in scope the union member
        using base_type::data;

        vector() {
            ((data[Ns] = 0), ...);
        }

        vector(typename std::conditional<num_components == 1, scalar_type, detail::nothing>::type s) {
            data[0] = s;
        }

        explicit vector(typename std::conditional<num_components != 1, scalar_type, detail::nothing>::type s) {
            ((data[Ns] = s), ...);
        }

        template<typename A0, typename... Args,
                 class = typename std::enable_if<
                         ((sizeof...(Args) >= 1) ||
                          ((sizeof...(Args) == 0) && !std::is_scalar<A0>::value))>::type>
        explicit vector(A0 &&a0, Args &&...args) {
            static_assert((sizeof...(args) < num_components), "too many arguments");

#define CTOR_FOLD
#ifdef CTOR_FOLD
            size_t i = 0;

            construct_at_index(i, detail::decay(std::forward<A0>(a0)));
            (construct_at_index(i, detail::decay(std::forward<Args>(args))), ...);
#else
            static_recurse<0>(std::forward<A0>(a0), std::forward<Args>(args)...);
#endif
        }

        scalar_type const operator[](size_t i) const {
            return data[i];
        }

        scalar_type &operator[](size_t i) {
            return data[i];
        }

        decay_type decay() const {
            return static_cast<const decay_type &>(*this);
        }

        operator typename std::conditional<num_components == 1, scalar_type, detail::nothing>::type() const {
            return data[0];
        }

        using self_type = vector_type;
        using other_type = self_type;
#define Is Ns
#define HAS_UNARY_MUL
        // must define `self_type` and `other_type`
        // must define `Is` as the param pack expansion of the indices

#define DEF_OP_UNARY_SCALAR(op)             \
    self_type &operator op(scalar_type s) { \
        ((data[Is] op s), ...);             \
        return *this;                       \
    }

        DEF_OP_UNARY_SCALAR(+=)
        DEF_OP_UNARY_SCALAR(-=)
        DEF_OP_UNARY_SCALAR(*=)
        DEF_OP_UNARY_SCALAR(/=)

#define DEF_OP_UNARY_VECTOR(op)                   \
    self_type &operator op(const other_type &v) { \
        ((data[Is] op v.data[Is]), ...);          \
        return *this;                             \
    }

        DEF_OP_UNARY_VECTOR(+=)
        DEF_OP_UNARY_VECTOR(-=)
#ifdef HAS_UNARY_MUL
        DEF_OP_UNARY_VECTOR(*=)
#undef HAS_UNARY_MUL
#endif
        DEF_OP_UNARY_VECTOR(/=)

#ifndef OMIT_NEG_OP
        self_type operator-() const {
            return self_type((-data[Is])...);
        }
#endif// !OMIT_NEG_OP

        //TODO: add  ==, !=

#undef DEF_OP_UNARY_SCALAR
#undef DEF_OP_UNARY_VECTOR
#undef Is

        //TODO: add matrix multiply

    private:
#ifdef CTOR_FOLD
        void construct_at_index(size_t &i, scalar_type arg) {
            data[i++] = arg;
        }

        template<typename Other, size_t... Other_Ns>
        void construct_at_index(size_t &i, const vector<Other, Other_Ns...> &arg) {
            constexpr auto other_num = vector<Other, Other_Ns...>::num_components;
            constexpr auto count = num_components <= other_num ? num_components : other_num;

            detail::static_for<0, count>()([&](size_t j) {
                data[i++] = arg.data[j];
            });
        }
#else
        template<size_t i>
        void construct_at_index(scalar_type arg) {
            data[i] = arg;
        }

        template<size_t i, typename Other, size_t... Other_Ns>
        void construct_at_index(const vector<Other, Other_Ns...> &arg) {
            constexpr auto other_num = vector<Other, Other_Ns...>::num_components;
            constexpr auto count = (i + other_num) > num_components ? num_components : (i + other_num);
            detail::static_for<i, count>()([&](size_t j) {
                data[j] = arg.data[j - i];
            });
        }

        template<size_t I, typename Arg0, typename... Args>
        void static_recurse(Arg0 &&a0, Args &&...args) {
            construct_at_index<I>(detail::decay(std::forward<Arg0>(a0)));
            static_recurse<I + detail::get_size<Arg0>()>(std::forward<Args>(args)...);
        }

        template<size_t I>
        void static_recurse() {}
#endif
    };

    namespace traits {

        // get the common type between 2 vectors (including scalar promotions aka vec1's)
        // rules:
        //   - their underlying scalar types must have same common type
        //   - if vector of different dimensions, at least one must be vec1 (promoted scalar)
        // output:
        //   the higher dimension vector is chosen
        template<
                class vector_type_1, class scalar_type_1, size_t size_1,
                class vector_type_2, class scalar_type_2, size_t size_2>
        struct common_vec_type_impl
        {
            using scalar_common_type = typename std::common_type<scalar_type_1, scalar_type_2>::type;
            static_assert(std::is_same<scalar_common_type, scalar_type_1>::value ||
                                  std::is_same<scalar_common_type, scalar_type_2>::value,
                          "invalid vector common scalar type");
            static_assert(size_1 == size_2 || size_1 == 1 || size_2 == 1,
                          "vector sizes must be equal or at least one needs to be a promoted scalar");

            using type = typename std::conditional<
                    // if same dimensions, choose the one with the common scalar
                    size_1 == size_2,
                    typename std::conditional<std::is_same<scalar_common_type, scalar_type_1>::value,
                                              vector_type_1, vector_type_2>::type,
                    // else (diff dimensions) choose the bigger one
                    typename std::conditional<size_1 == 1,
                                              vector_type_2, vector_type_1>::type>::type;
        };

        template<class V1, class V2>
        struct common_vec_type : common_vec_type_impl<
                                         V1, typename V1::scalar_type, V1::num_components,
                                         V2, typename V2::scalar_type, V2::num_components>
        {
        };

        // get the equivalent vec type doing necesary promotion (scalar to vec1 for ex)
        // this is the general case operating on a list of types
        template<class T, class... Ts>
        struct promote_to_vec : common_vec_type<
                                        typename promote_to_vec<T>::type,
                                        typename promote_to_vec<Ts...>::type>
        {
        };

        // implementation class, will get specialized
        template<class T>
        struct promote_to_vec_impl
        {
        };

        // specialization for promotion of a single type, defers to impl class
        template<class T>
        struct promote_to_vec<T> : promote_to_vec_impl<typename MetaEngine::Math::detail::remove_cvref<T>::type>
        {
        };

        // specialization: vector just returns itself
        template<typename T, size_t... Ns>
        struct promote_to_vec_impl<MetaEngine::Math::vector<T, Ns...>>
        {
            using type = MetaEngine::Math::vector<T, Ns...>;
        };

        // specialization: swizzlers return their equivalent vec (.xx -> vec2)
        template<typename vector_type, typename T, size_t N, size_t... indices>
        struct promote_to_vec_impl<MetaEngine::Math::detail::swizzler<vector_type, T, N, indices...>>
        {
            using type = vector_type;
        };

        // specialization: aritmethic scalars are promoted to vec1
        template<typename T>
        struct scalar_to_vector
        {
            using type = MetaEngine::Math::vector<T, 0>;
        };
        template<>
        struct promote_to_vec_impl<bool> : scalar_to_vector<bool>
        {
        };
        template<>
        struct promote_to_vec_impl<int> : scalar_to_vector<int>
        {
        };
        template<>
        struct promote_to_vec_impl<long int> : scalar_to_vector<long int>
        {
        };
        template<>
        struct promote_to_vec_impl<float> : scalar_to_vector<float>
        {
        };
        template<>
        struct promote_to_vec_impl<double> : scalar_to_vector<double>
        {
        };

        constexpr int vec_traits_test() {
            using vec1 = MetaEngine::Math::vector<float, 0>;
            using dvec1 = MetaEngine::Math::vector<double, 0>;
            using vec3 = MetaEngine::Math::vector<float, 0, 1, 2>;

            static_assert(std::is_convertible<vec1, float>::value, "mismatch");

            static_assert(std::is_same<promote_to_vec<float>::type, vec1>::value, "mismatch");
            static_assert(std::is_same<promote_to_vec<vec3>::type, vec3>::value, "mismatch");
            static_assert(std::is_same<promote_to_vec<decltype(vec3().xyz)>::type, vec3>::value, "mismatch");

            static_assert(std::is_same<promote_to_vec<float, float>::type, vec1>::value, "mismatch");
            static_assert(std::is_same<promote_to_vec<float, double>::type, dvec1>::value, "mismatch");
            static_assert(std::is_same<promote_to_vec<vec3, float>::type, vec3>::value, "mismatch");
            static_assert(std::is_same<promote_to_vec<double, vec3>::type, vec3>::value, "mismatch");
            static_assert(std::is_same<promote_to_vec<vec3, float, double>::type, vec3>::value, "mismatch");

            return 0;
        }

        static constexpr auto vec_traits_unit_test = vec_traits_test();

    }// namespace traits
}// namespace MetaEngine::Math


#define MAKE_LIB_FUNC(name)                                                                                                                                                         \
    template<class... Args>                                                                                                                                                         \
    inline auto name(Args &&...args)->decltype(MetaEngine::Math::detail::decay(MetaEngine::Math::traits::promote_to_vec<Args...>::type::lib_##name(std::forward<Args>(args)...))) { \
        return MetaEngine::Math::traits::promote_to_vec<Args...>::type::                                                                                                            \
                lib_##name(std::forward<Args>(args)...);                                                                                                                            \
    }

MAKE_LIB_FUNC(radians)
MAKE_LIB_FUNC(degrees)
MAKE_LIB_FUNC(sin)
MAKE_LIB_FUNC(cos)
MAKE_LIB_FUNC(tan)
MAKE_LIB_FUNC(asin)
MAKE_LIB_FUNC(acos)
MAKE_LIB_FUNC(atan)

MAKE_LIB_FUNC(pow)
MAKE_LIB_FUNC(exp)
MAKE_LIB_FUNC(log)
MAKE_LIB_FUNC(exp2)
MAKE_LIB_FUNC(log2)
MAKE_LIB_FUNC(sqrt)
MAKE_LIB_FUNC(inversesqrt)

MAKE_LIB_FUNC(abs)
MAKE_LIB_FUNC(sign)
MAKE_LIB_FUNC(floor)
MAKE_LIB_FUNC(trunc)
MAKE_LIB_FUNC(ceil)
MAKE_LIB_FUNC(fract)
MAKE_LIB_FUNC(mod)
MAKE_LIB_FUNC(min)
MAKE_LIB_FUNC(max)
MAKE_LIB_FUNC(clamp)
MAKE_LIB_FUNC(mix)
MAKE_LIB_FUNC(step)
MAKE_LIB_FUNC(smoothstep)

MAKE_LIB_FUNC(length)
MAKE_LIB_FUNC(distance)
MAKE_LIB_FUNC(normalize)
MAKE_LIB_FUNC(dot)
MAKE_LIB_FUNC(cross)
MAKE_LIB_FUNC(faceforward)
MAKE_LIB_FUNC(reflect)
MAKE_LIB_FUNC(refract)

MAKE_LIB_FUNC(lessThan)
MAKE_LIB_FUNC(lessThanEqual)
MAKE_LIB_FUNC(greaterThan)
MAKE_LIB_FUNC(greaterThanEqual)
MAKE_LIB_FUNC(equal)
MAKE_LIB_FUNC(notEqual)
MAKE_LIB_FUNC(any)
MAKE_LIB_FUNC(all)
MAKE_LIB_FUNC(_not)

MAKE_LIB_FUNC(dFdx)
MAKE_LIB_FUNC(dFdy)
MAKE_LIB_FUNC(fwidth)

#undef MAKE_LIB_FUNC