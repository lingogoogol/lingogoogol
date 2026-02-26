#ifndef LGO_JSON_D_H
#define LGO_JSON_D_H

#include <map>

#include "f.h"
#include "../arr/.h"
#include "../num/.h"

namespace lgo {
	enum class json_type {
		num,
		str,
		bol,
		arr,
		obj,
		nul
	};

	class json_pv {
	public:
		virtual ~json_pv();
		virtual operator strd() const = 0;
		json_pv* operator[](intf64 index) const;
		strd get_str() const;
		intf64 len() const;
		json_pv* get_val(const strd& key) const;
	protected:
		json_pv();
	};

	class json_num :public json_pv {
	public:
		json_num(const strd& in);
		virtual ~json_num();
		virtual operator strd() const;
	private:
		dnum<uint64> m_data{};
	};

	class json_str :public json_pv {
	public:
		json_str(const strd& in);
		virtual ~json_str();
		virtual operator strd() const;
		strd get_str() const;
	private:
		strd m_data{};
	};

	class json_bol :public json_pv {
	public:
		json_bol(const strd& in);
		virtual ~json_bol();
		virtual operator strd() const;
	private:
		bool m_data{};
	};

	class json_arr :public json_pv {
	public:
		json_arr(const strd& in);
		virtual ~json_arr();
		virtual operator strd() const;
		json_pv* operator[](intf64 index) const;
		intf64 len() const;
	private:
		arrd<json_pv*> m_data{};
	};

	class json_obj :public json_pv {
	public:
		json_obj(const strd& in);
		virtual ~json_obj();
		virtual operator strd() const;
		json_pv* get_val(const strd& key) const;
	private:
		std::map<strd, json_pv*> m_data{};
	};

	class json_nul :public json_pv {
	public:
		json_nul(const strd& in);
		virtual ~json_nul();
		virtual operator strd() const;
	};
}

#endif