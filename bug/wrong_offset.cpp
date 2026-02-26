struct ax
{
    int x;
};

struct ay
{
    int y;
};

constexpr auto may = &ay::y;

struct b : ax, ay { };

constexpr auto mby = &b::y;

static_assert(may == mby);

union test
{
    struct { int skip; int member; } mmm;
    b parent;
};

constexpr test tt{};

static_assert(&tt.mmm.member == &tt.parent.y);

static_assert(&tt.parent.y == &(tt.parent.*may));
static_assert(&tt.parent.y == &(tt.parent.*mby));
static_assert(&tt.mmm.member == &(tt.parent.*mby));
static_assert(&tt.mmm.member == &(tt.parent.*may));