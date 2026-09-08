module;

#include <nlohmann/json.hpp>

export module external.nlohmann;

export {
    using namespace ::nlohmann;
    using ::nlohmann::json;
}
