module lgo.GUI.object.depth_tracker;

import std;

import lgo.math.vec;
import lgo.math.op;

namespace lgo
{
    auto depth_tracker_t::add(pos_2D pos, size_2D size, float depth, bool opaque) -> std::uint64_t
    {
        m_depth_data.emplace(m_current_id, depth_data{ pos, size, depth, opaque });
        return m_current_id++;
    }

    auto depth_tracker_t::remove(std::uint64_t id) -> void
    {
        m_depth_data.erase(id);
        return;
    }

    auto depth_tracker_t::update(std::uint64_t id, pos_2D pos, size_2D size) -> void
    {
        auto& data{ m_depth_data.at(id) };
        data.m_pos = pos;
        data.m_size = size;
    }

    auto depth_tracker_t::top(pos_2D pos, std::uint64_t id) -> bool
    {
        float depth{ m_depth_data[id].m_depth };
        for (auto i{ m_depth_data.begin() }; i != m_depth_data.end(); ++i)
        {
            if
            (
                inside(i->second.m_pos, i->second.m_size, pos) &&
                i->second.m_depth < depth &&
                i->second.m_opaque
            )
            {
                return false;
            }
        }
        return true;
    }
}
