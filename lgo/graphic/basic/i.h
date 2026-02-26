#ifndef LGO_GRAPHIC_I_H
#define LGO_GRAPHIC_I_H

#include "d.h"
#include "../lib/glad.h"
#include "../debug/f.h"
#include "../math/i.h"
#include "../constant/.h"
#include "../data/f.h"
#include "../mat/s/i.h"

namespace lgo {
    inline void message_window_pv::destruct() {
        destruct1();
    }

    template<message_window_type T1>
    message_window<T1>::message_window(std::string title, std::u32string message_param) :
        m_window{ create_window(title,800,300,false) }, m_message{ message_param,50.0f,
        {0.0f,0.0f,0.0f}, ortho(0.0f,800.0f,300.0f,0.0f,0.0f,1.0f), }, m_text_shader{
        create_shader(impl::text_vertex_shader, impl::text_fragment_shader) } {
        m_message.set_pos({ 400.0f,150.0f,1.0f });
    }

    template<message_window_type T1>
    void message_window<T1>::process(int index) {
        glfwMakeContextCurrent(m_window);
        if (!glfwWindowShouldClose(m_window)) {
            if constexpr (T1 == message_window_type::error) {
                while (!glfwWindowShouldClose(m_window))
                    process1();
                for (int i{ 0 }; i < impl::get_data()->message_windows->size(); ++i)
                    (*impl::get_data()->message_windows)[i].~Copy_holder();
                glfwTerminate();
                std::exit(0);
            }
            else
                process1();
        }
        else
            (*impl::get_data()->message_windows)[index].~Copy_holder();
    }

    template<message_window_type T1>
    void message_window<T1>::process1() {
        glfwPollEvents();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_message.render(m_text_shader);
        glfwSwapBuffers(m_window);
    }

    template<message_window_type T1>
    void message_window<T1>::destruct1() {
        glfwMakeContextCurrent(*impl::get_data()->main_window);
        glfwDestroyWindow(m_window);
        for (std::vector<Copy_holder<message_window_pv*>>::iterator
            i{ impl::get_data()->message_windows->begin() };
            i != impl::get_data()->message_windows->end(); ++i) {
            if (sc<message_window<T1>*>(**i) == this) {
                impl::get_data()->message_windows->erase(i);
                break;
            }
        }
    }

    inline GLFWwindow* create_window(std::string title, unsigned int width,
        unsigned int height, bool resizeable) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MAJOR);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MINOR);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, resizeable);
        GLFWwindow* window{ glfwCreateWindow(width, height, title.data(), nullptr, nullptr) };
        if (!window) {
            handle_error(U"error: Failed to open window.\n");
        }
        glfwMakeContextCurrent(window);
        if (!gladLoadGL(rc<GLADloadfunc>(glfwGetProcAddress))) {
            handle_error(U"error: Failed to initialize GLAD.\n");
        }
        glViewport(0, 0, width, height);
        return window;
    }

    template<typename... T1>
    unsigned int create_fbo(T1... attachments) {
        unsigned int fbo{ impl::create_fbo1(sizeof...(T1) - 1,attachments...) };
        arrs<unsigned int, sizeof...(T1)> buffers{};
        for (int i{ 0 }; i < sizeof...(T1) - 1; ++i)
            buffers[i] = GL_COLOR_ATTACHMENT0 + i;
        glDrawBuffers(sizeof...(T1) - 1, buffers.data());
        return fbo;
    }

    inline unsigned int create_shader(std::string vertex, std::string fragment) {
        unsigned int vertex_shader{ glCreateShader(GL_VERTEX_SHADER) };
        const char* shader_code{ &(vertex[0]) };
        glShaderSource(vertex_shader, 1, &shader_code, NULL);
        glCompileShader(vertex_shader);
        int success{};
        char error_message[impl::error_message_max_size]{};
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex_shader, impl::error_message_max_size, NULL, error_message);
            handle_error(to_string32(error_message));
        }
        unsigned int fragment_shader{ glCreateShader(GL_FRAGMENT_SHADER) };
        shader_code = &(fragment[0]);
        glShaderSource(fragment_shader, 1, &shader_code, NULL);
        glCompileShader(fragment_shader);
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment_shader, impl::error_message_max_size, NULL, error_message);
            handle_error(to_string32(error_message));
        }
        unsigned int shader{ glCreateProgram() };
        glAttachShader(shader, vertex_shader);
        glAttachShader(shader, fragment_shader);
        glLinkProgram(shader);
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, impl::error_message_max_size, NULL, error_message);
            handle_error(to_string32(error_message));
        }
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return shader;
    }

    template<typename T1>
    unsigned int create_tex(T1* tex_data, GLenum format,
        unsigned int width, unsigned int height) {
        unsigned int tex{};
        glGenTextures(1, &tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GLenum type{};
        if constexpr (std::is_same_v<T1, unsigned char>) {
            type = GL_UNSIGNED_BYTE;
        }
        else if constexpr (std::is_same_v<T1, int>) {
            type = GL_INT;
        }
        else if constexpr (std::is_same_v<T1, unsigned int>) {
            type = GL_UNSIGNED_INT;
        }
        else if constexpr (std::is_same_v<T1, float>) {
            type = GL_FLOAT;
        }
        else {
            handle_error(U"");
        }
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, type, tex_data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    inline int get_window_width(GLFWwindow* window) {
        int width{};
        glfwGetWindowSize(window, &width, nullptr);
        return width;
    }

    inline int get_window_height(GLFWwindow* window) {
        int height{};
        glfwGetWindowSize(window, nullptr, &height);
        return height;
    }

    inline int get_framebuffer_width(GLFWwindow* window) {
        int width{};
        glfwGetFramebufferSize(window, &width, nullptr);
        return width;
    }

    inline int get_framebuffer_height(GLFWwindow* window) {
        int height{};
        glfwGetFramebufferSize(window, nullptr, &height);
        return height;
    }

    inline double get_cursor_x(GLFWwindow* window) {
        double x{};
        glfwGetCursorPos(window, &x, nullptr);
        return x;
    }

    inline double get_cursor_y(GLFWwindow* window) {
        double y{};
        glfwGetCursorPos(window, nullptr, &y);
        return sc<double>(get_window_height(window)) - y;
    }

    inline void set_window_width(GLFWwindow* window, int width) {
        glfwSetWindowSize(window, width, get_window_height(window));
    }

    inline void set_window_height(GLFWwindow* window, int height) {
        glfwSetWindowSize(window, get_window_width(window), height);
    }

    inline void set_cursor_x(GLFWwindow* window, double x) {
        glfwSetCursorPos(window, x, get_window_height(window) - get_cursor_y(window));
    }

    inline void set_cursor_y(GLFWwindow* window, double y) {
        glfwSetCursorPos(window, get_cursor_x(window), get_window_height(window) - y);
    }

    inline void set_callback(GLFWwindow* window, GLFWkeyfun key_callback,
        GLFWcharfun char_callback, GLFWcursorposfun cursor_pos_callback,
        GLFWmousebuttonfun mouse_button_callback, GLFWscrollfun scroll_callback) {
        glfwSetKeyCallback(window, key_callback);
        glfwSetCharCallback(window, char_callback);
        glfwSetCursorPosCallback(window, cursor_pos_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetScrollCallback(window, scroll_callback);
    }

    namespace impl {
        template<typename... T1>
        unsigned int create_fbo1(int color_attachment_num,
            unsigned int attachment, T1... attachments) {
            if constexpr (sizeof...(T1) == 0) {
                unsigned int fbo{};
                glGenFramebuffers(1, &fbo);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    GL_TEXTURE_2D, attachment, 0);
                return fbo;
            }
            else {
                unsigned int fbo{ create_fbo1(color_attachment_num, attachments...) };
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 +
                    (color_attachment_num - sizeof...(T1)), GL_TEXTURE_2D, attachment, 0);
                return fbo;
            }
        }
    }
}

#endif