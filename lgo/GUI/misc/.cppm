export module lgo.GUI.misc;

import std;

export namespace lgo
{
    struct motion_limits_t
    {
        double acceleration_max{12000.}; // Logical pixels per second squared.
        double speed_max{1600.};        // Logical pixels per second.
        auto validate() const -> void
        {
            if (!std::isfinite(acceleration_max) || acceleration_max <= 0. ||
                !std::isfinite(speed_max) || speed_max <= 0.)
                throw std::invalid_argument{"Motion limits must be finite and positive."};
        }
    };

    // Time-based, bounded-acceleration motion shared by wheel scrolling and div
    // layout. Integrate exactly to acceleration/cruise/braking transitions so
    // stopping distance v*v/(2*a) remains correct even at low frame rates.
    struct motion_axis_t
    {
        double position{}, target{}, velocity{};
        auto snap(double value) -> void { position = target = value; velocity = 0.; }
        auto shift(double offset) -> void { position += offset; target += offset; }
        auto active() const -> bool { return position != target || velocity != 0.; }
        auto clamp(double maximum) -> void
        {
            target = std::clamp(target, 0., maximum);
            const auto bounded{std::clamp(position, 0., maximum)};
            if (position != bounded || (position <= 0. && velocity < 0.) ||
                (position >= maximum && velocity > 0.)) velocity = 0.;
            position = bounded;
        }
        auto advance(double seconds, const motion_limits_t& limits) -> void
        {
            // A minimized window or slow frame must not cause one giant jump.
            double remaining{std::clamp(seconds, 0., .05)};
            const auto a{limits.acceleration_max}, speed{limits.speed_max};
            for (unsigned event{}; remaining > 1e-9 && active() && event < 16; ++event)
            {
                const auto delta{target - position}, distance{std::abs(delta)};
                if (distance < 1e-6 && std::abs(velocity) < 1e-3) { snap(target); break; }
                const auto direction{delta < 0. ? -1. : delta > 0. ? 1. : velocity < 0. ? -1. : 1.};
                const auto v{velocity * direction};
                double acceleration{}, duration{remaining};
                if (v < 0.) { acceleration = a; duration = std::min(duration, -v/a); }
                else if (v > 0. && (v*v/(2.*a) >= distance - 1e-8 || v > speed))
                {
                    acceleration = -a;
                    duration = std::min(duration, v > speed ? (v-speed)/a : v/a);
                }
                else if (v >= speed)
                    duration = std::min(duration, (distance - v*v/(2.*a))/v);
                else
                {
                    acceleration = a;
                    const auto brake_at{(std::sqrt(v*v/2. + a*distance)-v)/a};
                    duration = std::min({duration, (speed-v)/a, brake_at});
                }
                if (duration <= 1e-9) break;
                position += direction * (v*duration + .5*acceleration*duration*duration);
                velocity = direction * (v + acceleration*duration);
                if (std::abs(velocity) < 1e-8) velocity = 0.;
                remaining -= duration;
            }
        }
    };

    struct depth_range_t
    {
        float near{}, far{};
    };

    struct color_t
    {
        float R{}, G{}, B{};
    };

    enum class alignment_x
    {
        left,
        center,
        right,
        count
    };

    enum class alignment_y
    {
        top,
        center,
        bottom,
        count
    };

    struct alignment_2D
    {
        alignment_x x{};
        alignment_y y{};
    };

    auto to_string(depth_range_t depth_range) -> std::string;
    auto to_string(color_t color) -> std::string;
}
