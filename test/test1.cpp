#include <memory>
#include <map>

struct state_t{
    std::map<int, std::shared_ptr<state_t>> m{};
};
int main() {
    std::map<int, std::shared_ptr<state_t>> map{};
    map.erase(1);
    return 0;
}