#ifndef LGO_DEBUG_I_H
#define LGO_DEBUG_I_H

#include <functional>

#include "f.h"
#include "../lib/glad.h"
#include "../graphic/d.h"
#include "../math/f.h"
#include "../holder_i.h"
#include "../data/d.h"
#include "../str/f.h"

namespace lgo {
    namespace impl {
        inline void handle_error1(const char* file, int line, std::u32string message) {
            get_data()->logfile->push("錯誤：\n").push("  檔案：").push(file)
                .push("\n  行數：").push(line).push("\n  訊息：")
                .push(to_string8(message.data())).push("\n");
        }

        inline void check_gl_error1(const char* file, int line) {
            GLenum code{};
            while ((code = glGetError()) != GL_NO_ERROR) {
                handle_error1(file, line, std::u32string{ code });
            }
        }
    }
}

#endif