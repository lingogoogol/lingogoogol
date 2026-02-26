#ifndef LGO_IO_CMD_DEF
#define LGO_IO_CMD_DEF

LGO_EXPORT namespace lgo {
    class cmd_type: public lgo {
    public:
        cmd_type() = delete;
        cmd_type(std::istream& is, std::ostream& os);
        cmd_type(const cmd_type&) = delete;

        auto operator=(const cmd_type&) = delete;

        template<typename u_self, typename u_in>
        auto pad(this u_self&& self, const u_in& in) -> u_self&&;
    private:
        const ptrs<std::istream> m_is{};
        const ptrs<std::ostream> m_os{};
    };
}

#endif