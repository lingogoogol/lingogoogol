#ifndef LIBRARY_GRAPHIC_H
#define LIBRARY_GRAPHIC_H

#include <string>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "graphic_f.h"
#include "text.h"
#include "holder.h"
#include "data.h"
#include "const_f.h"

enum class Message_window_type {
	Error
};

class Message_window_pv {
public:
	virtual ~Message_window_pv() = default;
	virtual void process(int index) = 0;
	void destruct();
private:
	virtual void destruct1() = 0;
};

template<Message_window_type T1>
class Message_window final :public Message_window_pv {
public:
	Message_window(std::string title, std::u32string message_param);
	virtual ~Message_window() = default;
	virtual void process(int index);
private:
	GLFWwindow* window{};
	Text message{};
	unsigned int text_shader{};
	void process1();
	virtual void destruct1();
};

template<Message_window_type T1>
Message_window<T1>::Message_window(std::string title, std::u32string message_param) :
	window{ create_window(title,800,300,false) }, message{ message_param,50.0f,
	{0.0f,0.0f,0.0f}, glm::ortho(0.0f,800.0f,300.0f,0.0f), }, text_shader{
	create_shader(implement::text_vertex_shader, implement::text_fragment_shader) } {
	message.set_pos({ 400.0f,150.0f,1.0f });
	return;
}

template<Message_window_type T1>
void Message_window<T1>::process(int index) {
	glfwMakeContextCurrent(window);
	if (!glfwWindowShouldClose(window)) {
		if constexpr (T1 == Message_window_type::Error) {
			while (!glfwWindowShouldClose(window))
				process1();
			for (int i{ 0 }; i < implement::get_data()->message_windows->size(); i++)
				(*implement::get_data()->message_windows)[i].~Copy_holder();
			glfwTerminate();
			std::exit(0);
		}
		else
			process1();
	}
	else
		(*implement::get_data()->message_windows)[index].~Copy_holder();
	return;
}

template<Message_window_type T1>
void Message_window<T1>::process1() {
	glfwPollEvents();
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	message.render(text_shader);
	glfwSwapBuffers(window);
	return;
}

template<Message_window_type T1>
void Message_window<T1>::destruct1() {
	glfwMakeContextCurrent(*implement::get_data()->main_window);
	glfwDestroyWindow(window);
	for (std::vector<Copy_holder<Message_window_pv*>>::iterator
		i{ implement::get_data()->message_windows->begin() }; i != implement::get_data()->message_windows->end(); i++)
		if (static_cast<Message_window<T1>*>(**i) == this) {
			implement::get_data()->message_windows->erase(i);
			break;
		}
	return;
}

#endif