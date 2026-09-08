export module lgo.dev.error;

import std;

export namespace lgo {
    class error_t: public std::exception {
    private:
        std::string m_message{};
        std::string m_what{};
    protected:
        error_t(std::string message, std::string what);
    public:
        error_t(std::string message);
        virtual ~error_t() = default;

        auto message() -> std::string;
        virtual auto what() const noexcept -> const char* override;
    };

    class internal_error_t: public error_t {
    public:
        internal_error_t(const std::string& message);
        virtual ~internal_error_t() = default;

        virtual auto what() const noexcept -> const char* override;
    };
}
