export module lgo.dev.error;

import std;

export namespace lgo {
    class error_t: public std::exception {
    private:
        std::string m_message{};
    public:
        error_t(std::string message);
        virtual ~error_t() = default;

        auto message() -> std::string;
        virtual auto what() -> std::string;
    };

    class internal_error_t: public error_t {
    private:
        std::stacktrace m_stack{};
    public:
        internal_error_t(const std::string& description, std::stacktrace stack = std::stacktrace::current());
        virtual ~internal_error_t() = default;

        auto what() -> std::string override;
    };
}
