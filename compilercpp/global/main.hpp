#ifndef COMPILERCPP_GLOBAL_MAIN
#define COMPILERCPP_GLOBAL_MAIN

#include <cstdint>
#include <iostream>

#include "../lib/.hpp"
#include "../expr/.hpp"
#include "../data_type/.hpp"

#include "pv.hpp"
#include "fun.hpp"

class global {
public:
    using type_t = std::uint8_t;

    enum: type_t {
        data_type,
        var,
        fun
    };
private:
    type_t m_type{};
    global_pv* m_data{};
public:
    global(std::istream& src) {
        try {
            m_type = static_cast<type_t>(src.get());
        }
        catch (std::ios_base::failure&) {
            if (src.eof()) {
                throw error_t{ "全域物件結束" };
            }
            else {
                throw internal_error_t{ "讀取原始碼檔案時發生錯誤" };
            }
        }

        switch (m_type) {
        case data_type:
            m_data = new ::data_type{ src };
            break;
        case fun:
            m_data = new ::fun{ src };
            break;
        default:
            throw error_t{ "未知的全域物件種類" };
        }
    }

    auto type() const -> type_t {
        return m_type;
    }

    auto to_fun() -> ::fun& {
        return *dynamic_cast<::fun*>(m_data);
    }

    auto save(std::ostream& dest) -> void {
        dest.put(m_type);
        m_data->save(dest);
        return;
    }
};

#endif