#ifndef LGO_BASIC_IO_OPT_UNTIL_IGNORE_DEF
#define LGO_BASIC_IO_OPT_UNTIL_IGNORE_DEF

#include "../../../seq/io/cpt/.hpp"

LGO_EXPORT namespace lgo {
    template<is_seqio_cpt t_type>
    class io_until_ignore: public lgo {
    public:
        io_until_ignore(t_type delim);

        auto delim() const -> const t_type&;
    private:
        t_type m_delim{};
    };
}

#endif