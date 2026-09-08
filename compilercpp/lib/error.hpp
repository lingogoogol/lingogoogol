#ifndef COMPILERCPP_LIB_ERROR
#define COMPILERCPP_LIB_ERROR

#include <exception>
#include <cstdint>
#include <string>
#include <stacktrace>
#include <utility>

#include "common.hpp"

class error_t: public std::exception {
private:
    std::string m_message{};
    std::string m_what{};
public:
    error_t(std::string message): m_message{ std::move(message) }, m_what{ "error: " + m_message } {}

    ~error_t() override = default;

    auto message() const noexcept -> const std::string& {
        return m_message;
    }

    auto what() const noexcept -> const char* override {
        return m_what.c_str();
    }
};

class internal_error_t: public error_t {
private:
    std::stacktrace m_stack{};
    std::string m_what{};
public:
    internal_error_t(const std::string& description, std::stacktrace stack = std::stacktrace::current())
    : error_t{ description }, m_stack{ stack }
    , m_what{ "internal error: " + description + "\nstack trace: " + std::to_string(m_stack) } {}

    ~internal_error_t() override = default;

    auto what() const noexcept -> const char* override {
        return m_what.c_str();
    }
};

#endif
