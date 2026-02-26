template<const auto&>
class c{};

template<typename>
constexpr static int g{0};

template<typename t>
auto f(t) -> c<g<t>>;

int main() {
    decltype(f(int{})) a{};
    a;
    return 0;
}