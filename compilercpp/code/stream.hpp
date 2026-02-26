#ifndef COMPILERCPP_CODE_STREAM
#define COMPILERCPP_CODE_STREAM

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <map>

#include "../out/setting.hpp"

#include "../lib/.hpp"

class code_stream_def_index_counter_t {
private:
    std::map<std::string, std::uint64_t> m_index{};
    std::map<std::string, std::uint64_t> m_internal_index{};
public:
    auto get_index(const std::string& name) -> std::uint64_t {
        return m_index[name]++;
    }

    auto get_internal_index(const std::string& name) -> std::uint64_t {
        return m_internal_index[name]++;
    }
};

class code_stream {
public:
    struct address_t {
        std::uint64_t m_raw_ptr{};
        std::uint64_t m_RVA{};
    };

    using type_t = std::uint8_t;
    using get_address_t = std::function<std::uint64_t(std::string, type_t)>;
    using resolver_t = std::function<void(const get_address_t&, std::vector<unsigned char>&, address_t, const std::string&)>;

    enum: type_t {
        raw_ptr,
        VA,
        RVA
    };
private:
    using address_def_t = std::map<std::string, address_t>;

    struct address_ref_data_t {
        std::string m_name{};
        resolver_t m_resolver{};
        address_t m_address{};
    };
    using address_ref_t = std::vector<address_ref_data_t>;

    std::uint64_t m_base_VA{};
    std::uint64_t m_RVA{};
    std::uint64_t m_raw_ptr{};
    std::vector<unsigned char> m_plain{};
    address_def_t m_def{};
    address_ref_t m_ref{};
    
    auto get_address_impl(address_t, type_t type) const -> std::uint64_t;
public:
    auto plain_size() const -> std::uint64_t;

    auto pad_byte(unsigned char in) -> void;
    template<typename t_in>
    auto pad_int(t_in in) -> void;
    auto pad_str(std::string in) -> void;
    auto pad_address_def(std::string name) -> void;
    auto pad_address_def_index(std::string name, code_stream_def_index_counter_t& index_counter) -> void;
    auto pad_address_ref(std::string name, resolver_t resolver) -> std::uint64_t;
    auto resolver_set(std::uint64_t id, resolver_t resolver) -> void;
    
    auto get_address_current(type_t type) const -> std::uint64_t;
    auto get_address(std::string name, type_t type) const -> std::uint64_t;
    auto get_def() const -> const address_def_t&;
    auto get_ref() const -> const address_ref_t&;

    auto merge(const code_stream& in) -> void;
    auto align() -> void;
    auto resolve() -> void;
    auto out(std::ostream& dest) -> void;
private:
    static auto overwrite_impl(std::vector<unsigned char>& plain, std::uint64_t pos, std::size_t size, std::uint64_t val) -> void;
    static auto resolver_overwrite_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
    , address_t address, const std::string name, std::function<std::uint64_t(const get_address_t&, const std::string)> val_fun, std::size_t size) -> void;
    static auto resolver_overwrite_address_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
    , address_t address, const std::string name, type_t type, std::size_t size) -> void;
    static auto resolver_overwrite_address_if_exist_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
    , address_t address, const std::string name, type_t type, std::size_t size) -> void;
    static auto resolver_overwrite_size_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
    , address_t address, const std::string name, type_t type, std::size_t size) -> void;
    static auto resolver_overwrite_size_if_exist_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
    , address_t address, const std::string name, type_t type, std::size_t size) -> void;
    static auto resolver_overwrite_size_sum_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
    , address_t address, const std::string name, type_t type, std::size_t size) -> void;
public:
    static auto resolver_default() -> resolver_t;
    static auto resolver_overwrite(std::function<std::int64_t(const get_address_t&, const std::string)> val_fun, std::size_t size) -> resolver_t;
    static auto resolver_overwrite_address(type_t type, std::size_t size) -> resolver_t;
    static auto resolver_overwrite_address_if_exist(type_t type, std::size_t size) -> resolver_t;
    static auto resolver_overwrite_size(type_t type, std::size_t size) -> resolver_t;
    static auto resolver_overwrite_size_if_exist(type_t type, std::size_t size) -> resolver_t;
    static auto resolver_overwrite_size_sum(type_t type, std::size_t size) -> resolver_t;
};

auto code_stream::get_address_impl(address_t address, type_t type) const -> std::uint64_t {
    switch (type) {
        case raw_ptr: {
            return address.m_raw_ptr;
        }
        case VA: {
            return m_base_VA + address.m_RVA;
        }
        case RVA: {
            return address.m_RVA;
        }
        default: {
            throw error_t{ "code stream resolve type" };
        }
    }
}

auto code_stream::plain_size() const -> std::uint64_t {
    return m_plain.size();
}

auto code_stream::pad_byte(unsigned char in) -> void {
    m_plain.push_back(in);
    ++m_RVA;
    ++m_raw_ptr;
    return;
}

auto code_stream::pad_str(std::string in) -> void {
    for (std::size_t i{ 0 }; i < in.size(); ++i) {
        pad_byte(in[i]);
    }
    return;
}

template<typename t_in>
auto code_stream::pad_int(t_in in) -> void {
    if constexpr (std::is_same_v<t_in, uint128_t>) {
        pad_int(in.m_significant_least);
        pad_int(in.m_significant_most);
    }
    else {
        for (int i{ 0 }; i < sizeof(t_in); ++i) {
            m_plain.push_back(static_cast<unsigned char>(in));
            in >>= 8;
        }
        m_RVA += sizeof(t_in);
        m_raw_ptr += sizeof(t_in);
    }
    return;
}

auto code_stream::pad_address_def(std::string name) -> void {
    m_def.emplace(name, address_t{ m_raw_ptr, m_RVA });
    return;
}

auto code_stream::pad_address_def_index(std::string name, code_stream_def_index_counter_t& index_counter) -> void {
    pad_address_def(name + std::to_string(index_counter.get_index(name)));
    return;
}

auto code_stream::pad_address_ref(std::string name, resolver_t resolver) -> std::uint64_t {
    m_ref.push_back(address_ref_data_t{ name, resolver, address_t{ m_raw_ptr, m_RVA } });
    return m_ref.size() - 1;
}

auto code_stream::resolver_set(std::uint64_t id, resolver_t resolver) -> void {
    m_ref[id].m_resolver = resolver;
    return;
}

auto code_stream::merge(const code_stream& in) -> void {
    m_plain.insert(m_plain.end(), in.m_plain.begin(), in.m_plain.end());
    for (auto i{ in.m_def.begin() }; i != in.m_def.end(); ++i) {
        m_def[i->first] = address_t{ m_raw_ptr + i->second.m_raw_ptr, m_RVA + i->second.m_RVA };
    }
    for (auto i{ 0 }; i < in.m_ref.size(); ++i) {
        address_ref_data_t current{ in.m_ref[i] };
        current.m_address.m_raw_ptr += m_raw_ptr;
        current.m_address.m_RVA += m_RVA;
        m_ref.push_back(current);
    }
    m_raw_ptr += in.m_raw_ptr;
    m_RVA += in.m_RVA;
    return;
}

auto code_stream::align() -> void {
    std::uint64_t raw_ptr_new{ roundup(m_raw_ptr, static_cast<std::uint64_t>(setting::file_alignment)) };
    std::uint64_t count{ raw_ptr_new - m_raw_ptr };
    for (std::uint32_t i{ 0 }; i < count; ++i) {
        pad_byte(0);
    }
    m_RVA = roundup(m_RVA, static_cast<std::uint64_t>(setting::virtual_alignment));
    return;
}

auto code_stream::get_address_current(type_t type) const -> std::uint64_t {
    return get_address_impl(address_t{ m_raw_ptr, m_RVA }, type);
}

auto code_stream::get_address(std::string name, type_t type) const -> std::uint64_t {
    const address_t* address{};
    try {
        address = &m_def.at(name);
    }
    catch (std::out_of_range) {
        throw error_t{ "找不到符號：\"" + name + "\"" };
    }
    return get_address_impl(*address, type);
}

auto code_stream::get_def() const -> const address_def_t& {
    return m_def;
}

auto code_stream::get_ref() const -> const address_ref_t& {
    return m_ref;
}

auto code_stream::resolve() -> void {
    for (std::size_t i{ 0 }; i < m_ref.size(); ++i) {
        m_ref[i].m_resolver(std::bind(&code_stream::get_address, this, std::placeholders::_1, std::placeholders::_2)
        , m_plain, m_ref[i].m_address, m_ref[i].m_name);
    }
    return;
}

auto code_stream::out(std::ostream& dest) -> void {
    for (std::size_t i{ 0 }; i < m_plain.size(); ++i) {
        dest.put(m_plain[i]);
    }
    return;
}

auto code_stream::overwrite_impl(std::vector<unsigned char>& plain, std::uint64_t pos, std::size_t size, std::uint64_t val) -> void {
    for (std::size_t i{ 0 }; i < std::min(std::size_t{ 8 }, size); ++i) {
        plain[pos + i] = static_cast<unsigned char>(val);
        val >>= 8;
    }
    for (std::size_t i{ 8 }; i < size; ++i) {
        plain[pos + i] = 0;
    }
    return;
}

auto code_stream::resolver_overwrite_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
, address_t address, const std::string name, std::function<std::uint64_t(const get_address_t&, const std::string)> val_fun, std::size_t size) -> void {
    overwrite_impl(plain, address.m_raw_ptr, size, val_fun(get_address, name));
    return;
}

auto code_stream::resolver_overwrite_address_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
, address_t address, const std::string name, type_t type, std::size_t size) -> void {
    overwrite_impl(plain, address.m_raw_ptr, size, get_address(name, type));
    return;
}

auto code_stream::resolver_overwrite_address_if_exist_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
, address_t address, const std::string name, type_t type, std::size_t size) -> void {
    try {
        resolver_overwrite_address_impl(get_address, plain, address, name, type, size);
    }
    catch (error_t) {}
    return;
}

auto code_stream::resolver_overwrite_size_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
, address_t address, const std::string name, type_t type, std::size_t size) -> void {
    overwrite_impl(plain, address.m_raw_ptr, size, get_address(name + "_end", type) - get_address(name + "_begin", type));
    return;
}

auto code_stream::resolver_overwrite_size_if_exist_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
, address_t address, const std::string name, type_t type, std::size_t size) -> void {
    try {
        resolver_overwrite_size_impl(get_address, plain, address, name, type, size);
    }
    catch (error_t) {}
    return;
}

auto code_stream::resolver_overwrite_size_sum_impl(const get_address_t& get_address, std::vector<unsigned char>& plain
, address_t address, const std::string name, type_t type, std::size_t size) -> void {
    std::uint64_t size_sum{ 0 };
    try {
        std::uint64_t index{ 0 };
        while (true) {
            size_sum += get_address(name + "_end" + std::to_string(index), type) - get_address(name + "_begin" + std::to_string(index), type);
            ++index;
        }
    }
    catch (error_t) {}
    overwrite_impl(plain, address.m_raw_ptr, size, size_sum);
    return;
}

auto code_stream::resolver_default() -> resolver_t {
    return [] (const get_address_t&, std::vector<unsigned char>&, address_t, const std::string&) -> void {
        throw internal_error_t{ "resolver default called" };
    };
}

auto code_stream::resolver_overwrite(std::function<std::int64_t(const get_address_t&, const std::string)> val_fun, std::size_t size) -> resolver_t {
    return std::bind(resolver_overwrite_impl, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, val_fun, size);
}

auto code_stream::resolver_overwrite_address(type_t type, std::size_t size) -> resolver_t {
    return std::bind(resolver_overwrite_address_impl, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, type, size);
}

auto code_stream::resolver_overwrite_address_if_exist(type_t type, std::size_t size) -> resolver_t {
    return std::bind(resolver_overwrite_address_if_exist_impl, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, type, size);
}

auto code_stream::resolver_overwrite_size(type_t type, std::size_t size) -> resolver_t {
    return std::bind(resolver_overwrite_size_impl, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, type, size);
}

auto code_stream::resolver_overwrite_size_if_exist(type_t type, std::size_t size) -> resolver_t {
    return std::bind(resolver_overwrite_size_if_exist_impl, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, type, size);
}

auto code_stream::resolver_overwrite_size_sum(type_t type, std::size_t size) -> resolver_t {
    return std::bind(resolver_overwrite_size_sum_impl, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, type, size);
}

#endif