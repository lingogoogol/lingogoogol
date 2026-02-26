#ifndef COMPILERCPP_LIB_ERROR
#define COMPILERCPP_LIB_ERROR

#include <exception>
#include <cstdint>
#include <string>
#include <stacktrace>

#include "common.hpp"

class error_t: public std::exception {
private:
    std::string m_message{};
public:
    error_t(std::string message): m_message{ message } {}

    virtual ~error_t() = default;

    auto message() -> std::string {
        return m_message;
    }

    virtual auto what() -> std::string {
        return "error: " + m_message;
    }
};

class internal_error_t: public error_t {
private:
    std::stacktrace m_stack{};
public:
    internal_error_t(const std::string& description, std::stacktrace stack = std::stacktrace::current())
    : error_t{ description }, m_stack{ stack } {}

    virtual ~internal_error_t() = default;

    auto what() -> std::string override {
        return "internal error: " + error_t::what() + "\n, stack trace: " + std::to_string(m_stack);
    }
};

#endif