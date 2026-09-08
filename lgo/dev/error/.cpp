module lgo.dev.error;

import std;

namespace lgo {
    error_t::error_t(std::string message)
    : m_message{ std::move(message) }, m_what{ "error: " + m_message } {}

    auto error_t::message() const noexcept -> const std::string& {
        return m_message;
    }

    auto error_t::what() const noexcept -> const char* {
        return m_what.c_str();
    }

    internal_error_t::internal_error_t(const std::string& description, std::stacktrace stack)
    : error_t{ description }, m_stack{ stack }
    , m_what{ "internal error: " + description + "\nstack trace: " + std::to_string(m_stack) } {}

    auto internal_error_t::what() const noexcept -> const char* {
        return m_what.c_str();
    }
}
