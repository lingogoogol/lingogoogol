class a {
protected:
    template<typename...>
    constexpr static bool cpt{ true };
};

class b: public a {
private:
    using base = a;
public:
    template<typename... u>
    auto f() -> void requires base::template cpt<u...> {}
};

int main() {
    b{}.f<char, bool>();
    return 0;
}