#ifndef COMPILERCPP_SETTING_MEMBER
#define COMPILERCPP_SETTING_MEMBER

class setting_member_t {
public:
    virtual ~setting_member_t() = default;
    virtual auto pad(std::ostream& dest) -> void = 0;
};

#endif