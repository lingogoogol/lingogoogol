#ifndef COMPILERCPP_GUI_OBJECT_DEPTH_TRACKER
#define COMPILERCPP_GUI_OBJECT_DEPTH_TRACKER

#include <map>

#include "../../lib/.hpp"

class depth_tracker_t {
private:
    struct depth_data {
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

auto depth_tracker_t::add(pos_2D pos, size_2D size, float depth, bool opaque) -> std::uint64_t {
    m_depth_data.emplace(m_current_id, depth_data{ pos, size, depth, opaque });
    return m_current_id++;
}

auto depth_tracker_t::remove(std::uint64_t id) -> void {
    m_depth_data.erase(id);
    return;
}

auto depth_tracker_t::top(pos_2D pos, std::uint64_t id) -> bool {
    float depth{ m_depth_data[id].m_depth };
    for (auto i{ m_depth_data.begin() }; i != m_depth_data.end(); ++i) {
        if (inside(i->second.m_pos, i->second.m_size, pos) && i->second.m_depth < depth && i->second.m_opaque) {
            return false;
        }
    }
    return true;
}

#endif