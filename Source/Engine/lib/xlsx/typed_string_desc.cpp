#include "typed_string_desc.h"

namespace
{
	using namespace spiritsaway::container;
	std::string value_type_to_str(basic_value_type in_type)
	{
		switch (in_type)
		{
		case basic_value_type::str:
			return "str";
			break;
		case basic_value_type::number_bool:
			return "bool";
			break;
		case basic_value_type::number_uint:
			return "uint";
			break;
		case basic_value_type::number_int:
			return "int";
			break;
		case basic_value_type::number_float:
			return "float";
			break;
		case basic_value_type::tuple:
			break;
		case basic_value_type::list:
			break;
		default:
			break;
		}
		return "";
	}
	std::optional<basic_value_type> str_to_value_type(std::string_view input)
	{
		static const std::unordered_map<std::string_view, basic_value_type> look_map = {
			{"str", basic_value_type::str},
			{"bool", basic_value_type::number_bool},
			{"int", basic_value_type::number_int},
			{"uint", basic_value_type::number_uint},
			{"float", basic_value_type::number_float},
		};
		auto cur_iter = look_map.find(input);
		if (cur_iter == look_map.end())
		{
			return {};
		}
		else
		{
			return cur_iter->second;
		}
	}

	bool check_value_match_type(basic_value_type cur_type, const json& cur_value)
	{
		switch (cur_type)
		{
		case basic_value_type::str:
		{
			return cur_value.is_string();
		}
		case basic_value_type::number_bool:
		{
			return cur_value.is_boolean();
		}
		case basic_value_type::number_uint:
		{
			return cur_value.is_number_unsigned();
		}
		case basic_value_type::number_int:
		{
			return cur_value.is_number_integer();
		}
		case basic_value_type::number_float:
		{
			return cur_value.is_number();
		}
		default:
			return false;
		}
	}
	
	
}
namespace spiritsaway::container
{
	typed_string_desc::typed_string_desc(basic_value_type in_type, const std::vector<json>& in_choice_values)
		: m_type(in_type)
		, m_choice_values(in_choice_values)
		, m_list_sz(0)
	{

	}

	typed_string_desc::typed_string_desc(const std::vector<std::shared_ptr<const typed_string_desc>>& tuple_detail)
		: m_type(basic_value_type::tuple)
		, m_type_detail(tuple_detail)
		, m_list_sz(0)
	{

	}
	typed_string_desc::typed_string_desc(std::shared_ptr < const typed_string_desc> list_detail, std::uint32_t in_list_sz)
		: m_type(basic_value_type::list)
		, m_type_detail(1, list_detail )
		, m_list_sz(in_list_sz)
	{

	}

	json typed_string_desc::encode() const
	{
		json result;
		if (int(m_type) >= int(basic_value_type::str) && int(m_type) <= int(basic_value_type::number_float))
		{
			auto cur_type_name = value_type_to_str(m_type);
			if (m_choice_values.empty())
			{
				result = cur_type_name;
			}
			else
			{
				result[cur_type_name] = m_choice_values;
			}
			return result;
		}
		else
		{
			if (m_type == basic_value_type::tuple)
			{
				for (const auto& one_ele : m_type_detail)
				{
					result.push_back(one_ele->encode());
				}
				return result;
			}
			else
			{
				result.push_back(m_type_detail[0]->encode());
				result.push_back(m_list_sz);
				return result;
			}
		}
	}
	std::ostream& operator<<(std::ostream& output_stream, const typed_string_desc& cur_node)
	{
		auto cur_json = cur_node.encode();
		output_stream << cur_json;
		return output_stream;
	}
	bool operator==(const typed_string_desc& cur, const typed_string_desc& other)
	{
		if (cur.m_type != other.m_type)
		{
			return false;
		}
		if (cur.m_choice_values != other.m_choice_values)
		{
			return false;
		}
		if (cur.m_list_sz != other.m_list_sz)
		{
			return false;
		}
		if (cur.m_type_detail.size() != other.m_type_detail.size())
		{
			return false;
		}
		for (int i = 0; i < cur.m_type_detail.size(); i++)
		{
			if (*cur.m_type_detail[i] != *other.m_type_detail[i])
			{
				return false;
			}
		}
		return true;
	}
	bool operator!=(const typed_string_desc& cur, const typed_string_desc& other)
	{
		return !(cur == other);
	}
	std::shared_ptr <const typed_string_desc> typed_string_desc::get_type_from_str(std::string_view type_string)
	{
		auto cur_basic_type = str_to_value_type(type_string);
		if (cur_basic_type)
		{
			return std::make_shared<typed_string_desc>(cur_basic_type.value(), std::vector<json>{});
		}
		if (!json::accept(type_string))
		{
			return {};
		}
		auto cur_type_json = json::parse(type_string);
		return get_type_from_json(cur_type_json);
	}

	std::shared_ptr <const typed_string_desc> typed_string_desc::get_type_from_json(const json& cur_type_json)
	{
		std::optional< basic_value_type> cur_basic_type;
		if (cur_type_json.is_string())
		{
			auto cur_str = cur_type_json.get<std::string>();
			cur_basic_type = str_to_value_type(cur_str);
			if (!cur_basic_type)
			{
				return {};
			}
			return std::make_shared<typed_string_desc>(cur_basic_type.value(), std::vector<json>{});
		}
		if (cur_type_json.is_object())
		{
			if (cur_type_json.size() != 1)
			{
				return {};
			}
			auto cur_pair = cur_type_json.cbegin();
			cur_basic_type = str_to_value_type(cur_pair.key());
			if (!cur_basic_type)
			{
				return {};
			}
			if (!cur_pair.value().is_array() || cur_pair.value().empty())
			{
				return {};
			}
			std::vector<nlohmann::json> cur_choices = cur_pair.value().get<json::array_t>();
			for (const auto& one_value : cur_choices)
			{
				if (!check_value_match_type(cur_basic_type.value(), one_value))
				{
					return {};
				}
			}
			return std::make_shared< typed_string_desc>(cur_basic_type.value(), cur_choices);
		}
		if (cur_type_json.is_array() && !cur_type_json.empty())
		{
			if (cur_type_json.back().is_number_unsigned())
			{
				if (cur_type_json.size() != 2)
				{
					return {};
				}
				auto cur_list_desc = get_type_from_json(cur_type_json[0]);
				if (!cur_list_desc)
				{
					return {};
				}
				return std::make_shared< typed_string_desc>(cur_list_desc, cur_type_json.back().get<std::uint32_t>());
			}
			else
			{
				std::vector<std::shared_ptr<const typed_string_desc>> cur_desc_vec;
				cur_desc_vec.reserve(cur_type_json.size());
				for (const auto& one_ele : cur_type_json)
				{
					auto temp_type = get_type_from_json(one_ele);
					if (!temp_type)
					{
						return {};
					}
					cur_desc_vec.push_back(temp_type);
				}
				return std::make_shared<typed_string_desc>(cur_desc_vec);

			}
		}
		return {};
	}
	bool typed_string_desc::validate(const json& cur_value_json) const
	{
		if (m_type == basic_value_type::list)
		{
			if (!cur_value_json.is_array())
			{
				return false;
			}
			if (m_list_sz && cur_value_json.size() != m_list_sz)
			{
				return false;
			}
			for (const auto& one_ele : cur_value_json)
			{
				if (!m_type_detail[0]->validate(one_ele))
				{
					return false;
				}
			}
			return true;
		}
		else if (m_type == basic_value_type::tuple)
		{
			if (!cur_value_json.is_array())
			{
				return false;
			}
			if (cur_value_json.size() != m_type_detail.size())
			{
				return false;
			}
			for (int i = 0; i < m_type_detail.size(); i++)
			{
				if (!m_type_detail[i]->validate(cur_value_json[i]))
				{
					return false;
				}
			}
			return true;
		}
		return check_value_match_type(m_type, cur_value_json);
	}

	typed_string_desc::~typed_string_desc()
	{
		return;
	}
}