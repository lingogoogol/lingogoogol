module lgo.dev.error;

import std;

namespace lgo {
    error_t::error_t(std::string message, std::string what)
    :
        m_message{ message },
        m_what{ what }
    {}

    error_t::error_t(std::string message)
    :
        error_t{ message, "error: " + message }
    {}

    auto error_t::message() -> std::string {
        return m_message;
    }

    auto error_t::what() const noexcept -> const char* {
        return m_what.c_str();
    }

    internal_error_t::internal_error_t(const std::string& message)
    :
        error_t{ message, "internal error: " + message }
    {}
}
