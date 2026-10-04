module lgo.GUI.object.div;

import std;

import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.state;
import lgo.GUI.object.depth_tracker;

namespace lgo
{
    GUI_div_t::GUI_div_t(engine_t* engine, depth_tracker_t* depth_tracker)
    : state_t{ engine, depth_tracker } {}
}
