template<typename t_in1, typename... t_in2>
concept is_constructible_cpt = requires(t_in2... in2) {
    { t_in1{ in2... } };
};

template<typename... t_type>
class tuple;
template<>
class tuple<>;

template<typename... t_type>
class tuple {
private:
    template<typename t_first, typename... t_next>
    struct decompose {
        using first = t_first;
        using next = tuple<t_next...>;
    };
    using first = decompose<t_type...>::first;
    using next = decompose<t_type...>::next;
public:
    template<typename t_first, typename... t_other>
    constexpr tuple(t_first, t_other...)
    requires is_constructible_cpt<next, t_other...>;
};

template<typename t_first, typename... t_other>
tuple(t_first, t_other...) -> tuple<t_first, t_other...>;

template<typename... t_type>
template<typename t_first, typename... t_other>
constexpr tuple<t_type...>::tuple(t_first, t_other...)
requires is_constructible_cpt<next, t_other...> {}

template<>
class tuple<> {};

int main() {
    tuple a{ 0 };
	return 0;
}