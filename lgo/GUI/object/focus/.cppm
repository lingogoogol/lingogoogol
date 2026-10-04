export module lgo.GUI.object.focus;

import std;

export namespace lgo
{
    class focus_t
    {
    private:
        std::set<std::function<void(void)>*> m_on{};
        std::set<std::function<void(void)>*> m_pending{};
        std::uint64_t m_depth{ 0 };
    public:
        focus_t() = default;

        auto clear() -> void;
        auto exclusive(const std::function<void(void)>& lose) -> void;
        auto share(const std::function<void(void)>& lose) -> void;
    };
}
