class int_class {
public:
    constexpr int_class() = default;
    constexpr int_class(const int_class&) = delete;
    constexpr ~int_class() = default;
};

class intp_class: public int_class {
public:
    constexpr intp_class() = default;
    constexpr intp_class(const intp_class&) {}
    constexpr intp_class(intp_class&&) {}
    constexpr ~intp_class() = default;
};

consteval auto f() -> intp_class {
    return intp_class{};
}

template<typename t_in>
constexpr void divdw(t_in) {}

int main() {
    divdw(f());
	while (true) {}
	return 0;
}