export module lgo.dev.error;

import std;

export namespace lgo {
    class error_t: public std::exception {
    private:
        std::string m_message{};
        std::string m_what{};
    public:
        error_t(std::string message);
        ~error_t() override = default;

        auto message() const noexcept -> const std::string&;
        auto what() const noexcept -> const char* override;
    };

    class internal_error_t: public error_t {
    private:
        std::stacktrace m_stack{};
        std::string m_what{};
    public:
        internal_error_t(const std::string& description, std::stacktrace stack = std::stacktrace::current());
        ~internal_error_t() override = default;

        auto what() const noexcept -> const char* override;
    };
}
