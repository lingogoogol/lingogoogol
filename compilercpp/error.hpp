#ifndef COMPILERCPP_ERROR
#define COMPILERCPP_ERROR

#include <exception>
#include <cstdint>
#include <string>
#include <stacktrace>

class error: public std::exception {
public:
    using type_t = std::uint8_t;

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
        return "compiler error: " + std::to_string(m_type);
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

#endif