#ifndef COMPILERCPP_LIB_GUI_OBJECT
#define COMPILERCPP_LIB_GUI_OBJECT

class GUI_object {
public:
    virtual ~GUI_object() = default;

    virtual auto show() -> void = 0;
    virtual auto hide() -> void = 0;
};

#endif