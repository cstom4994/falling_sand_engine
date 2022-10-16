#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <array>
#include <string>
#include <list>
#include <stack>
#include <deque>
#include <forward_list>
#include <utility>
#include <type_traits>
#include <set>

#include <optional>
#include <variant>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace spiritsaway::serialize
{
	template <typename T1, typename T2 = void>
	struct encodable : std::false_type
	{

	};

	template <typename... args> struct all_encode_able : std::false_type
	{

	};

	template <>
	struct all_encode_able<> : std::true_type
	{

	};

	template <typename T1, typename... args>
	struct all_encode_able<T1, args...> : std::integral_constant<bool, encodable<T1, void>::value && all_encode_able<args...>::value>
	{

	};



#define encodable_int(T) template <>\
struct encodable<T, void>: std::true_type\
{\
}\

	encodable_int(int8_t);
	encodable_int(int16_t);
	encodable_int(int32_t);
	encodable_int(int64_t);
	encodable_int(uint8_t);
	encodable_int(uint16_t);
	encodable_int(uint32_t);
	encodable_int(uint64_t);
	encodable_int(bool);

	template <>
	struct encodable<json, void> : std::true_type
	{

	};
	template <>
	struct encodable<float, void> : std::true_type
	{

	};
	template <>
	struct encodable<double, void> : std::true_type
	{

	};
	template <>
	struct encodable<std::string, void> : std::true_type
	{

	};

	template<typename T>
	struct encodable<std::optional<T>, std::void_t<typename std::enable_if<encodable<T>::value>::type>> : std::true_type
	{

	};

	template<typename T1, typename T2>
	struct encodable<std::pair<T1, T2>, std::void_t<typename std::enable_if<encodable<T1>::value && encodable<T2>::value>::type>> : std::true_type
	{

	};
	template<typename... args>
	struct encodable<std::tuple<args...>, std::void_t<typename std::enable_if<all_encode_able<args...>::value>::type>> :std::true_type
	{

	};
	template<typename... args>
	struct encodable<std::variant<args...>, std::void_t<typename std::enable_if<all_encode_able<args...>::value>::type>> :std::true_type
	{

	};
	template<typename T>
	struct encodable < std::vector<T>, std::void_t<typename std::enable_if<encodable<T>::value>::type>> : std::true_type
	{

	};
	template<typename T>
	struct encodable < std::list<T>, std::void_t<typename std::enable_if<encodable<T>::value>::type>> : std::true_type
	{

	};
	template<typename T>
	struct encodable < std::deque<T>, std::void_t<typename std::enable_if<encodable<T>::value>::type>> : std::true_type
	{

	};
	template<typename T>
	struct encodable < std::forward_list<T>, std::void_t<typename std::enable_if<encodable<T>::value>::type>> : std::true_type
	{

	};
	template <typename T1, std::size_t T2>
	struct encodable<std::array<T1, T2>, std::void_t<typename std::enable_if<encodable<T1>::value>::type>> : std::true_type
	{

	};
	template <typename T1, typename T2>
	struct encodable<std::map<T1, T2>, std::void_t<typename std::enable_if<encodable<T1>::value && encodable<T2>::value>::type>> : std::true_type
	{

	};
	template <typename T1, typename T2>
	struct encodable<std::unordered_map<T1, T2>, std::void_t<typename std::enable_if<encodable<T1>::value && encodable<T2>::value>::type>> : std::true_type
	{

	};
	template <typename T1>
	struct encodable<std::set<T1>, std::void_t<typename std::enable_if<encodable<T1>::value>::type>> : std::true_type
	{

	};
	template <typename T1>
	struct encodable<std::unordered_set<T1>, std::void_t<typename std::enable_if<encodable<T1>::value>::type>> : std::true_type
	{

	};
	template <typename T1, typename T2>
	struct encodable<std::multimap<T1, T2>, std::void_t<typename std::enable_if<encodable<T1>::value && encodable<T2>::value>::type>> : std::true_type
	{

	};
	template <typename T1, typename T2>
	struct encodable<std::unordered_multimap<T1, T2>, std::void_t<typename std::enable_if<encodable<T1>::value && encodable<T2>::value>::type>> : std::true_type
	{

	};
	template <typename T1>
	struct encodable<std::multiset<T1>, std::void_t<typename std::enable_if<encodable<T1>::value>::type>> : std::true_type
	{

	};
	template <typename T1>
	struct encodable<std::unordered_multiset<T1>, std::void_t<typename std::enable_if<encodable<T1>::value>::type>> : std::true_type
	{

	};
	template <typename T, typename B = void>
	struct has_encode_func
	{
		using result_type = void;

	};
	template <typename T>
	struct has_encode_func<T, std::void_t<decltype(std::declval<T>().encode())>>
	{
		using result_type = decltype(std::declval<T>().encode());
	};
	template <typename T>
	struct encodable<T,
		std::void_t<typename
		std::enable_if<
		encodable<decltype(std::declval<T>().encode())>::value
		>::type
		>
	> : std::true_type
	{

	};




#define encode_int(T) static T encode(const T& data)\
{\
	return data;\
}\

	encode_int(int8_t);
	encode_int(int16_t);
	encode_int(int32_t);
	encode_int(int64_t);
	encode_int(uint8_t);
	encode_int(uint16_t);
	encode_int(uint32_t);
	encode_int(uint64_t);

	//template <typename T>
	//inline typename std::enable_if<std::is_integral<T>::value, T>::type
	//	encode(const T& data)
	//{
	//	return data;
	//}
	static bool encode(bool data)
	{
		return data;
	}
	static double encode(const float& data)
	{
		return double(data);
	}
	static double encode(const double& data)
	{
		return data;
	}
	static std::string encode(const std::string& data)
	{
		return data;
	}
	static json encode(const json& data)
	{
		return data;
	}
	// forward declare
	template <typename T>
	json encode(const std::optional<T>& data);
	template <typename T1, typename T2>
	json encode(const std::pair<T1, T2>& data);
	template <typename T1, std::size_t T2>
	json encode(const std::array<T1, T2>& data);
	template <typename... args>
	json encode(const std::tuple<args...>& data);
	template <typename... Args>
	json encode(const std::variant<Args...>& data);
	template <typename T1, typename T2>
	json encode(const std::map<T1, T2>& data);
	template <typename T1, typename T2>
	json encode(const std::unordered_map<T1, T2>& data);
	template <typename T>
	json encode(const std::unordered_map<std::string, T>& data);
	template <typename T1, typename T2>
	json encode(const std::multimap<T1, T2>& data);
	template <typename T1, typename T2>
	json encode(const std::unordered_multimap<T1, T2>& data);
	template <typename T1>
	json encode(const std::set<T1>& data);
	template <typename T1>
	json encode(const std::unordered_set<T1>& data);
	template <typename T1>
	json encode(const std::multiset<T1>& data);
	template <typename T1>
	json encode(const std::unordered_multiset<T1>& data);
	template <typename T>
	inline typename std::enable_if<encodable<decltype(std::declval<T>().encode())>::value, json>::type encode(const T& data);
	template <typename T>
	json encode(const std::optional<T>& data)
	{
		if (data)
		{
			return encode(data.value());
		}
		else
		{
			return nullptr;
		}
	}
	template <typename T1, typename T2>
	json encode(const std::pair<T1, T2>& data)
	{

		json::array_t cur_array;
		cur_array.push_back(encode(data.first));
		cur_array.push_back(encode(data.second));
		return cur_array;
	}
	template <typename T1, std::size_t T2>
	json encode(const std::array<T1, T2>& data)
	{
		auto cur_array = json::array_t();
		for (std::size_t i = 0; i < T2; i++)
		{
			cur_array.push_back(encode(data[i]));
		}
		return cur_array;
	}

	template <typename tuple_type, std::size_t... index>
	json encode_tuple(const tuple_type& data, std::index_sequence<index...>)
	{
		auto cur_array = json::array_t({ encode(std::get<index>(data))... });
		return cur_array;
	}

	template <typename... args>
	json encode(const std::tuple<args...>& data)
	{
		return encode_tuple(data, std::index_sequence_for<args...>{});
	}
	template <typename... Args>
	json encode(const std::variant<Args...>& data)
	{
		if (data.index() == std::variant_npos)
		{
			return nullptr;
		}
		else
		{
			auto visitor = [](auto&& value) -> json
			{
				return encode(value);
			};
			return std::visit(visitor, data);
		}
	}

	template <typename T1, typename T2>
	json encode(const std::map<T1, T2>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(std::make_pair<json, json>(encode(i.first), encode(i.second)));
		}
		return cur_array;
	}

	template <typename T>
	json encode(const std::map<std::string, T>& data)
	{
		json::object_t result;
		for (const auto one_item : data)
		{
			result[one_item.first] = encode(one_item.second);
		}
		return result;
	}

	template <typename T1, typename T2>
	json encode(const std::unordered_map<T1, T2>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(std::make_pair<json, json>(encode(i.first), encode(i.second)));
		}
		return cur_array;
	}
	template <typename T>
	json encode(const std::unordered_map<std::string, T>& data)
	{
		json::object_t result;
		for (const auto one_item : data)
		{
			result[one_item.first] = encode(one_item.second);
		}
		return result;
	}

	template <typename T1, typename T2>
	json encode(const std::multimap<T1, T2>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(std::make_pair<json, json>(encode(i.first), encode(i.second)));
		}
		return cur_array;
	}
	template <typename T1, typename T2>
	json encode(const std::unordered_multimap<T1, T2>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(std::make_pair<json, json>(encode(i.first), encode(i.second)));
		}
		return cur_array;
	}

	template <typename T1>
	json encode(const std::set<T1>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(encode(i));
		}
		return cur_array;
	}
	template <typename T1>
	json encode(const std::unordered_set<T1>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(encode(i));
		}
		return cur_array;
	}
	template <typename T1>
	json encode(const std::multiset<T1>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(encode(i));
		}
		return cur_array;
	}
	template <typename T1>
	json encode(const std::unordered_multiset<T1>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(encode(i));
		}
		return cur_array;
	}
	template <typename T1, std::size_t T2>
	json encode(const T1(&data)[T2])
	{
		auto cur_array = json::array_t();
		for (std::size_t i = 0; i < T2; i++)
		{
			cur_array.push_back(encode(data[i]));
		}
		return cur_array;
	}
	template <typename T>
	json encode(const std::list<T>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(encode(i));
		}
		return cur_array;
	}
	template <typename T>
	json encode(const std::forward_list<T>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(encode(i));
		}
		return cur_array;
	}
	template <typename T>
	json encode(const std::vector<T>& data)
	{
		auto cur_array = json::array_t();
		for (const auto& i : data)
		{
			cur_array.push_back(encode(i));
		}
		return cur_array;
	}
	template <typename T>
	inline typename std::enable_if<encodable<decltype(std::declval<T>().encode())>::value, json>::type encode(const T& data)
	{
		return encode(data.encode());
	}
	template <typename... Args>
	json encode_multi(const Args&... args)
	{
		auto cur_array = json::array_t();
		(cur_array.push_back(encode(args)), ...);
		return cur_array;

	}
}