#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <stacktrace>
#include <chrono>

auto get_string(std::istream& istream) -> std::string {
    std::string out{};
    std::uint8_t len{ static_cast<std::uint8_t>(istream.get()) };
    for (std::uint8_t i{ 0 }; i < len; i++) {
        out.push_back(static_cast<char>(istream.get()));
    }
    return out;
}

template<typename t_in>
auto pad_integer(std::ostream& ostream, t_in in) -> void {
    ostream.write(reinterpret_cast<const char*>(&in), sizeof in);
    return;
}

template<typename t_in>
auto fill_integer(std::ostream& ostream, std::streampos pos, t_in in) -> void {
    const std::streampos current_pos{ ostream.tellp() };
    ostream.seekp(pos);
    pad_integer(ostream, in);
    ostream.seekp(current_pos);
    return;
}

auto pad_string(std::ostream& ostream, const std::string& in) -> void {
    for (std::uint8_t i{ 0 }; i < in.size(); ++i) {
        ostream.put(in[i]);
    }
    return;
}

class error: public std::exception {
public:
    using type_t = std::int8_t;

    enum: type_t {
        internal_error,
        command_line_argument,
        eof,
        global_type
    };
private:
    type_t m_type{};
public:
    error(type_t type): m_type{ type } {}

    virtual ~error() {}

    auto type() -> type_t {
        return m_type;
    }

    virtual auto what() -> std::string {
        return "compiler error: " + std::string{ m_type };
    }
};

class internal_error: public error {
private:
    std::string m_description{};
    std::stacktrace m_stack{};
public:
    internal_error(const std::string& description, std::stacktrace stack = std::stacktrace::current())
    : error{ error::internal_error }, m_description{ description }, m_stack{ stack } {}

    virtual ~internal_error() {}

    virtual auto what() -> std::string {
        return "internal error: " + m_description + "\n, stack trace: " + std::to_string(m_stack);
    }
};

class global_pv {
public:
    virtual ~global_pv() = default;
};

class fun: public global_pv {
private:
    std::string m_name{};
public:
    fun(std::istream& istream) {
        m_name = get_string(istream);
    }

    virtual ~fun() = default;

    auto name() const -> const std::string& {
        return m_name;
    }

    auto compile(std::string& ostream) const -> void {}
};

class global {
public:
    using type_t = std::uint8_t;

    enum: type_t {
        fun,
        var,
        data_type
    };
private:
    type_t m_type{};
    global_pv* m_data{};
public:
    global(std::istream& istream) {
        int type{};
        try {
            type = istream.get();
        }
        catch (std::ios_base::failure&) {
            if (istream.eof()) {
                throw error{ error::eof };
            }
            else {
                throw internal_error{ "source reading" };
            }
        }

        switch (type) {
        case fun:
            m_data = new ::fun{ istream };
            break;
        default:
            throw error{ error::global_type };
        }
    }

    auto type() const -> type_t {
        return m_type;
    }

    auto to_fun() -> ::fun& {
        return *dynamic_cast<::fun*>(m_data);
    }
};

struct data_directory {
    std::uint32_t virtual_address{ 0 };
    std::uint32_t size{ 0 };
};

auto pad_data_directory(std::ostream& ostream, const data_directory& in) -> void {
    pad_integer(ostream, in.virtual_address);
    pad_integer(ostream, in.size);
    return;
}

struct section_header {
    char name[8 + 1]{};
    std::uint32_t virtual_size{};
    std::uint32_t virtual_address{};
    std::uint32_t size_of_raw_data{};
    std::uint32_t pointer_to_raw_data{};
    std::uint32_t characteristics{};
};

auto pad_section_header(std::ostream& ostream, const section_header& in) -> void {
    ostream.write(in.name, 8);
    pad_integer(ostream, in.virtual_size);
    pad_integer(ostream, in.virtual_address);
    pad_integer(ostream, in.size_of_raw_data);
    pad_integer(ostream, in.pointer_to_raw_data);
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint16_t{ 0 });
    pad_integer(ostream, std::uint16_t{ 0 });
    pad_integer(ostream, in.characteristics);
    return;
}

auto compile(std::istream& source, std::ostream& exe) -> void {
    std::string entrance{ get_string(source) };

    std::vector<fun> funs{};
    try {
        while (true) {
            global get{ source };
            switch (get.type()) {
            case global::fun:
                funs.push_back(get.to_fun());
                break;
            default:
                throw internal_error{ "global type" };
            }
        }
    }
    catch (error& e) {
        if (e.type() != error::eof) {
            throw e;
        }
    }

    std::string text{};
    auto entrance_fun{ std::find_if(funs.begin(), funs.end(), [&entrance] (const fun& in) {
        return in.name() == entrance;
    }) };
    entrance_fun->compile(text);
    funs.erase(entrance_fun);
    for (std::int8_t i{ 0 }; i < funs.size(); ++i) {
        funs[i].compile(text);
    }

    pad_integer(exe, std::uint16_t{ 0x5A4D });
    for (std::uint8_t i{ 0 }; i < 29; ++i) {
        pad_integer(exe, std::uint16_t{ 0x0 });
    }
    pad_integer(exe, std::uint32_t{ 0x40 });

    pad_integer(exe, std::uint32_t{ 0x00004550 });
    pad_integer(exe, std::uint16_t{ 0x8664 });
    const std::streampos number_of_sections{ exe.tellp() };
    pad_integer(exe, std::uint16_t{});
    pad_integer(exe, static_cast<std::uint32_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())));
    pad_integer(exe, std::uint32_t{ 0 });
    pad_integer(exe, std::uint32_t{ 0 });
    const std::streampos size_of_optional_header{ exe.tellp() };
    pad_integer(exe, std::uint16_t{});
    pad_integer(exe, std::uint16_t{ 0x0022 });
    
    const std::streamoff start_of_optional_header{ exe.tellp() };

    pad_integer(exe, std::uint16_t{ 0x20B });
    pad_integer(exe, std::uint8_t{ 0 });
    pad_integer(exe, std::uint8_t{ 0 });
    const std::streampos size_of_code{ exe.tellp() };
    pad_integer(exe, std::uint32_t{});
    const std::streampos size_of_initialized_data{ exe.tellp() };
    pad_integer(exe, std::uint32_t{});
    const std::streampos size_of_uninitialized_data{ exe.tellp() };
    pad_integer(exe, std::uint32_t{});
    const std::streampos address_of_entry_point{ exe.tellp() };
    pad_integer(exe, std::uint32_t{});
    const std::streampos base_of_code{ exe.tellp() };
    pad_integer(exe, std::uint32_t{});

    pad_integer(exe, std::uint64_t{ 0x400000 });
    pad_integer(exe, std::uint32_t{ 0x1000 });
    pad_integer(exe, std::uint32_t{ 0x200 });
    pad_integer(exe, std::uint16_t{ 6 });
    pad_integer(exe, std::uint16_t{ 0 });
    pad_integer(exe, std::uint16_t{ 0 });
    pad_integer(exe, std::uint16_t{ 0 });
    pad_integer(exe, std::uint16_t{ 6 });
    pad_integer(exe, std::uint16_t{ 0 });
    pad_integer(exe, std::uint32_t{ 0 });
    const std::streampos size_of_image{ exe.tellp() };
    pad_integer(exe, std::uint32_t{});
    const std::streampos size_of_headers{ exe.tellp() };
    pad_integer(exe, std::uint32_t{});
    pad_integer(exe, std::uint32_t{ 0 });
    pad_integer(exe, std::uint16_t{ 3 });
    pad_integer(exe, std::uint16_t{ 0x8160 });
    pad_integer(exe, std::uint64_t{ 0x100000 });
    pad_integer(exe, std::uint64_t{ 0x1000 });
    pad_integer(exe, std::uint64_t{ 0x100000 });
    pad_integer(exe, std::uint64_t{ 0x1000 });
    pad_integer(exe, std::uint32_t{ 0 });
    pad_integer(exe, std::uint32_t{ 0x10 });

    const std::streampos export_table{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos import_table{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos resource_table{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos exception_table{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos certificate_table{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos base_relocation_table{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos debug{ exe.tellp() };
    pad_data_directory(exe, {});
    pad_data_directory(exe, { 0, 0 });
    const std::streampos global_ptr{ exe.tellp() };
    pad_data_directory(exe, { 0, 0 });
    const std::streampos tls_table{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos load_config_table{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos bound_import{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos IAT{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos delay_import_descriptor{ exe.tellp() };
    pad_data_directory(exe, {});
    const std::streampos CLR_runtime_header{ exe.tellp() };
    pad_data_directory(exe, {});
    pad_data_directory(exe, { 0, 0 });

    fill_integer(exe, size_of_optional_header, static_cast<std::uint16_t>(static_cast<std::streamoff>(exe.tellp()) - start_of_optional_header));

    constexpr std::uint32_t IMAGE_SCN_CNT_CODE{ 0x00000020 };
    constexpr std::uint32_t IMAGE_SCN_CNT_INITIALIZED_DATA{ 0x00000040 };
    constexpr std::uint32_t IMAGE_SCN_CNT_UNINITIALIZED_DATA{ 0x00000080 };
    constexpr std::uint32_t IMAGE_SCN_GPREL{ 0x00008000 };
    constexpr std::uint32_t IMAGE_SCN_LNK_NRELOC_OVFL{ 0x01000000 };
    constexpr std::uint32_t IMAGE_SCN_MEM_DISCARDABLE{ 0x02000000 };
    constexpr std::uint32_t IMAGE_SCN_MEM_NOT_CACHED{ 0x04000000 };
    constexpr std::uint32_t IMAGE_SCN_MEM_NOT_PAGED{ 0x08000000 };
    constexpr std::uint32_t IMAGE_SCN_MEM_SHARED{ 0x10000000 };
    constexpr std::uint32_t IMAGE_SCN_MEM_EXECUTE{ 0x20000000 };
    constexpr std::uint32_t IMAGE_SCN_MEM_READ{ 0x40000000 };
    constexpr std::uint32_t IMAGE_SCN_MEM_WRITE{ 0x80000000 };

    std::vector<section_header> section_headers{
        { ".bss", 0x200, 0x1000, 0, 0, IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE },
        { ".data", 0x200, 0x2000, 0x200, 0x400, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE },
        { ".rdata", 0x200, 0x3000, 0x200, 0x600, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ },
        { ".tls", 0x200, 0x4000, 0x200, 0x800, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE },
        { ".text", 0x3, 0x5000, 0x200, 0xA00, IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ },
        { ".idata", 0x200, 0x6000, 0x200, 0xC00, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE },
        { ".reloc", 0x200, 0x7000, 0x200, 0xE00, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE }
    };
    fill_integer(exe, number_of_sections, static_cast<std::uint16_t>(section_headers.size()));
    fill_integer(exe, size_of_code, 0x200);
    fill_integer(exe, size_of_initialized_data, 0xA00);
    fill_integer(exe, size_of_uninitialized_data, 0x200);
    fill_integer(exe, address_of_entry_point, 0x4000);
    fill_integer(exe, base_of_code, 0x4000);
    fill_integer(exe, size_of_image, 0x8000);
    fill_integer(exe, size_of_headers, 0x400);
    for (std::uint8_t i{ 0 }; i < section_headers.size(); ++i) {
        pad_section_header(exe, section_headers[i]);
    }

    while (static_cast<std::streamoff>(exe.tellp()) < 0x1000) {
        pad_integer(exe, std::uint8_t{ 0 });
    }
    return;
}

auto main(int argc, char** argv) -> int {
    try {
        if (argc != 3) {
            throw error{ error::command_line_argument };
        }
        
        constexpr std::ios_base::iostate exception_mask{ std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit };
        std::ifstream source{};
        source.open(argv[1], std::ios_base::in | std::ios_base::binary);
        source.exceptions(exception_mask);
        std::ofstream exe{};
        exe.open(argv[2], std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        exe.exceptions(exception_mask);

        compile(source, exe);

        source.close();
        exe.close();
    }
    catch (std::exception& e) {
        std::cout << "an error occurred: " << e.what() << '\n';
    }
    return 0;
}