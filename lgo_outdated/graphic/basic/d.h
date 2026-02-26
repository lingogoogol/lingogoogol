#ifndef LGO_GRAPHIC_D_H
#define LGO_GRAPHIC_D_H

#include <string>
#include <cstdlib>

#include "f.h"
#include "../lib/glad.h"
#include "../lib/glfw.h"
#include "../text/d.h"
#include "../holder_d.h"
#include "../data/d.h"
#include "../constant/.h"

namespace lgo {
    enum class message_window_type {
        error
    };

    class message_window_pv {
    public:
        virtual ~message_window_pv() = default;
        virtual void process(int index) = 0;
        void destruct();
    private:
        virtual void destruct1() = 0;
    };

    template<message_window_type T1>
    class message_window final :public message_window_pv {
    public:
        message_window(std::string title, std::u32string message_param);
        virtual ~message_window() = default;
        virtual void process(int index);
    private:
        GLFWwindow* m_window{};
        text m_message{};
        unsigned int m_text_shader{};
        void process1();
        virtual void destruct1();
    };
}

#endif