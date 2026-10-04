export module lgo.GUI.object.depth_tracker;

import std;

import lgo.math.vec;

export namespace lgo
{
    class depth_tracker_t
    {
    private:
        struct depth_data
        {
            pos_2D m_pos{};
            size_2D m_size{};
            float m_depth{};
            bool m_opaque{};
        };

        std::map<std::uint64_t, depth_data> m_depth_data{};
        std::uint64_t m_current_id{};
    public:
        depth_tracker_t() = default;

        auto add(pos_2D pos, size_2D size, float depth, bool opaque) -> std::uint64_t;
        auto remove(std::uint64_t id) -> void;
        auto top(pos_2D pos, std::uint64_t id) -> bool;
    };
}
