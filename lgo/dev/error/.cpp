module lgo.dev.error;

import std;

namespace lgo {
    error_t::error_t(std::string message): m_message{ message } {}

    auto error_t::message() -> std::string {
        return m_message;
    }

    auto error_t::what() -> std::string {
        return "error: " + m_message;
    }

    internal_error_t::internal_error_t(const std::string& description, std::stacktrace stack = std::stacktrace::current())
    : error_t{ description }, m_stack{ stack } {}

    auto internal_error_t::what() -> std::string override {
        return "internal error: " + error_t::what() + "\n, stack trace: " + std::to_string(m_stack);
    }
}
