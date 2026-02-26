#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "value.h"
#include "window_f.h"
#include "text.h"
#include "holder.h"

enum class Message_window_type {
	Error
};

class Message_window_pv {
public:
	virtual void process(int index) = 0;
	void destruct();
private:
	virtual void destruct1() = 0;
};

template<Message_window_type T1>
class Message_window final :public Message_window_pv {
public:
	Message_window(std::string title, std::u32string message);
	virtual void process(int index);
private:
	GLFWwindow* window{};
	Text message{};
	void process1();
	virtual void destruct1();
};

template<Message_window_type T1>
Message_window<T1>::Message_window(std::string title, std::u32string message_param) :
	window{ create_window(title,800,300,false) }, message{ message_param,50.0f,glm::vec3{0.0f,0.0f,0.0f},
	glm::ortho(0.0f,800.0f,300.0f,0.0f) }{
	message.set_pos(glm::vec3{ 400.0f,150.0f,1.0f });
	return;
}

template<Message_window_type T1>
void Message_window<T1>::process(int index) {
	glfwMakeContextCurrent(window);
	if (!glfwWindowShouldClose(window)) {
		if constexpr (T1 == Message_window_type::Error) {
			while (!glfwWindowShouldClose(window))
				process1();
			for (int i{ 0 }; i < object::message_window.size(); i++)
				object::message_window[i].~Copy_holder();
			glfwTerminate();
			std::exit(0);
		}
		else
			process1();
	}
	else
		object::message_window[index].~Copy_holder();
	return;
}

template<Message_window_type T1>
void Message_window<T1>::process1() {
	glfwPollEvents();
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	message.print();
	glfwSwapBuffers(window);
	return;
}

template<Message_window_type T1>
void Message_window<T1>::destruct1() {
	glfwMakeContextCurrent(object::main_window);
	glfwDestroyWindow(window);
	for (std::vector<Copy_holder<Message_window_pv*>>::iterator i{ object::message_window.begin() }; i != object::message_window.end(); i++)
		if (static_cast<Message_window<T1>*>(**i) == this) {
			object::message_window.erase(i);
			break;
		}
	return;
}

#endif