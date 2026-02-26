#ifndef LGO_JSON_I_H
#define LGO_JSON_I_H

#include "d.h"

namespace lgo {
	inline json_pv::json_pv() {}

	inline json_pv::~json_pv() {}

	inline json_pv* json_pv::operator[](intf64 index) const {
		auto casted{ dc<const json_arr*>(this) };
		if (casted) {
			return (*casted)[index];
		}
		else {
			throw U"";
		}
	}

	inline intf64 json_pv::len() const {
		auto casted{ dc<const json_arr*>(this) };
		if (casted) {
			return casted->len();
		}
		else {
			throw U"";
		}
	}

	inline strd json_pv::get_str() const {
		auto casted{ dc<const json_str*>(this) };
		if (casted) {
			return casted->get_str();
		}
		else {
			throw U"";
		}
	}

	inline json_pv* json_pv::get_val(const strd& key) const {
		auto casted{ dc<const json_obj*>(this) };
		if (casted) {
			return casted->get_val(key);
		}
		else {
			throw U"";
		}
	}

	inline json_num::json_num(const strd& in) :json_pv{}, m_data{ in } {}

	inline json_num::~json_num() {}

	inline json_num::operator strd() const {
		return m_data;
	}

	inline json_str::json_str(const strd& in) : json_pv{}, m_data{ in } {
		if (m_data[0] != '\"' || m_data[m_data.len() - 1] != '\"') {
			throw U"";
		}
		m_data.replace(0, 1, "").replace(m_data.len() - 1, m_data.len(), "");
		for (intf64 i{ 0 }; i < m_data.len(); ++i) {
			if (m_data[i] == '\\') {
				switch (m_data[i + 1]) {
				case 'b':
				case 'f':
				case 'n':
				case 't':
				case '\"':
				case '\\':
					m_data.replace(i, i + 2, m_data[i + 1]);
					break;
				default:
					throw U"";
				}
			}
		}
	}

	inline json_str::~json_str() {}

	inline json_str::operator strd() const {
		strd out{ "\"" };
		for (int i{ 0 }; i < m_data.len(); ++i) {
			switch (m_data[i]) {
			case '\b':
				out += "\\b";
				break;
			case '\f':
				out += "\\f";
				break;
			case '\n':
				out += "\\n";
				break;
			case '\t':
				out += "\\t";
				break;
			case '\"':
				out += "\\\"";
				break;
			case '\\':
				out += "\\\\";
				break;
			default:
				out.push(m_data[i]);
				break;
			}
		}
		out.push('\"');
		return out;
	}

	inline strd json_str::get_str() const {
		return m_data;
	}

	inline json_bol::json_bol(const strd& in) :json_pv{}, m_data{} {
		if (in == "true")
			m_data = true;
		else if (in == "false")
			m_data = false;
		else
			throw U"";
	}

	inline json_bol::~json_bol() {}

	inline json_bol::operator strd() const {
		if (m_data)
			return "true";
		else
			return "false";
	}

	inline json_arr::json_arr(const strd& in) :json_pv{}, m_data{} {
		if (in[0] != '[' || in[in.len() - 1] != ']')
			throw U"";
		intf64 i{ 1 };
		m_data.push(make_json(in, i, &i));
		for (; i < in.len();)
			if (formatting(in[i]))
				++i;
			else if (in[i] == ',')
				m_data.push(make_json(in, i + 1, &i));
			else if (i == in.len() - 1)
				return;
			else
				throw U"";
	}

	inline json_arr::~json_arr() {}

	inline json_arr::operator strd() const {
		strd out{ "[" };
		for (intf64 i{ 0 }; i < len(); ++i) {
			out += sc<strd>(*(*this)[i]);
			if (i < len() - 1)
				out.push(',');
		}
		out.push(']');
		return out;
	}

	inline json_pv* json_arr::operator[](intf64 index) const {
		return m_data[index];
	}

	inline intf64 json_arr::len() const {
		return m_data.len();
	}

	inline json_obj::json_obj(const strd& in) :json_pv{}, m_data{} {
		if (in[0] != '{' || in[in.len() - 1] != '}')
			throw U"";
		for (intf64 i{ 1 }; i < in.len() - 1;) {
			while (formatting(in[i]))
				++i;
			strd key{};
			if (in[i] == '\"') {
				for (++i; i < in.len() && in[i] != '\"'; ++i)
					key.push(in[i]);
				++i;
			}
			else
				for (; i < in.len() && in[i] != ':' && !formatting(in[i]); ++i)
					key.push(in[i]);
			while (formatting(in[i]))
				++i;
			if (in[i] != ':')
				throw U"";
			m_data[key] = make_json(in, i + 1, &i);
			while (formatting(in[i]))
				++i;
			if (i == in.len() - 1)
				break;
			if (in[i] != ',')
				throw U"";
			++i;
		}
	}

	inline json_obj::~json_obj() {}

	inline json_obj::operator strd() const {
		strd out{ "{" };
		for (auto i{ m_data.begin() }; i != m_data.end(); ++i)
			out += strd{ "\"" } + i->first + "\":" + sc<strd>(*i->second) + ",";
		out.pop();
		out.push('}');
		return out;
	}

	inline json_pv* json_obj::get_val(const strd& key) const {
		auto i{ m_data.find(key) };
		if (i == m_data.end())
			throw U"";
		else
			return i->second;
	}

	inline json_nul::json_nul(const strd& in) :json_pv{} {
		if (in != "null")
			throw U"";
	}

	inline json_nul::~json_nul() {}

	inline json_nul::operator strd() const {
		return "null";
	}

	inline json_pv* make_json(const strd& in, intf64 begin, intf64* end) {
		intf64 end_v{};
		const auto num_op{ [&in, &end_v](intf64 begin)->json_pv* {
			for (intf64 i{ begin + 1 }; i < in.len(); ++i)
				if (!numchar(in[i])) {
					end_v = i;
					break;
				}
			if (!end_v)
				end_v = in.len();
			return new json_num{ in.substr(begin, end_v) };
		} };
		const auto str_op{ [&in, &end_v](intf64 begin)->json_pv* {
			const auto even_backslashes{ [&in](intf64 index)->bool {
				const auto impl{ [&in](intf64 index, auto& impl)->bool {
					if (in[index] == '\\')
						return !impl(index - 1, impl);
					else
						return true;
				} };
				return impl(index, impl);
			} };
			for (intf64 i{ begin + 1 }; i < in.len(); ++i)
				if (in[i] == '\"' && even_backslashes(i - 1)) {
					end_v = i + 1;
					return new json_str{ in.substr(begin, end_v) };
				}
			throw U"";
		} };
		const auto boolnull_op{ [&in, &end_v](intf64 begin, strd expected)->json_pv* {
			end_v = begin + expected.len();
			if (in.substr(begin, end_v) != expected)
				throw U"";
			if (expected == "null")
				return new json_nul{ expected };
			else
				return new json_bol{ expected };
		} };
		const auto arrobj_op{ [&in, &end_v](intf64 begin, char open, char close)->json_pv* {
			intf64 level{ 1 };
			for (intf64 i{ begin + 1 }; i < in.len() && end_v == 0; ++i)
				if (in[i] == open)
					level++;
				else if (in[i] == close) {
					if (level == 1) {
						end_v = i + 1;
						strd param{ in.substr(begin, end_v) };
						if (open == '[')
							return new json_arr{ param };
						else
							return new json_obj{ param };
					}
					level--;
				}
			throw U"";
		} };
		json_pv* out{};
		intf64 i{ begin };
		for (; i < in.len(); ++i)
			if (!formatting(in[i]))
				break;
		if (i == in.len() - 1)
			throw U"";
		if (numchar(in[i]))
			out = num_op(i);
		else
			switch (in[i]) {
			case '\"':
				out = str_op(i);
				break;
			case 't':
				out = boolnull_op(i, "true");
				break;
			case 'f':
				out = boolnull_op(i, "false");
				break;
			case '[':
				out = arrobj_op(i, '[', ']');
				break;
			case '{':
				out = arrobj_op(i, '{', '}');
				break;
			case 'n':
				out = boolnull_op(i, "null");
				break;
			default:
				throw U"";
			}
		if (end)
			*end = end_v;
		return out;
	}
}

#endif