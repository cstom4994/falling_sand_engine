#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <ostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace spiritsaway::container
{

	enum class basic_value_type
	{
		str,
		number_bool,//BOOL
		number_uint,//UINT
		number_int,//INT
		number_float,//float
		// complicated types

		tuple,// ["float", "int"]
		list,// ["float", 3]

	};
	
	class typed_string_desc
	{
	public:
		const basic_value_type m_type;
		const std::vector<nlohmann::json> m_choice_values;
		const std::vector<std::shared_ptr<const typed_string_desc>> m_type_detail;
		const std::uint32_t m_list_sz;
	public:
		typed_string_desc() = delete;
	public:
		typed_string_desc(basic_value_type in_type, const std::vector<json>& in_choice_values);
		typed_string_desc(const std::vector<std::shared_ptr<const typed_string_desc>>& tuple_detail);
		typed_string_desc(std::shared_ptr < const typed_string_desc> list_detail, std::uint32_t in_list_sz);
	public:
		json encode() const;
		friend std::ostream& operator<<(std::ostream& output_stream, const typed_string_desc& cur_node);
		static std::shared_ptr <const typed_string_desc> get_type_from_str(std::string_view type_string);
		static std::shared_ptr <const typed_string_desc> get_type_from_json(const json& cur_type_json);
		bool validate(const json& cur_value_json) const;
		friend bool operator==(const typed_string_desc& cur, const typed_string_desc& other);
		friend bool operator!=(const typed_string_desc& cur, const typed_string_desc& other);
		
		typed_string_desc(const typed_string_desc& other) = delete;
		typed_string_desc& operator=(const typed_string_desc& other) = delete;
		~typed_string_desc();
		
	};
}