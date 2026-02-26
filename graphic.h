#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <string>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "data.h"
#include "graphic_f.h"
#include "text.h"
#include "holder.h"

enum class Message_window_type {
	Error
};

class Message_window_pv {
public:
	virtual ~Message_window_pv() = default;
	virtual void process(int index, Data_pv* data) = 0;
	void destruct(Data_pv* data);
private:
	virtual void destruct1(Data_pv* data) = 0;
};

template<Message_window_type T1>
class Message_window final :public Message_window_pv {
public:
	Message_window(std::string title, std::u32string message, Data_pv* data);
	virtual ~Message_window() = default;
	virtual void process(int index, Data_pv* data);
private:
	GLFWwindow* window{};
	Text message{};
	unsigned int text_shader{};
	void process1(Data_pv* data);
	virtual void destruct1(Data_pv* data);
};

template<Message_window_type T1>
Message_window<T1>::Message_window(std::string title, std::u32string message_param, Data_pv* data) :
	window{ create_window(title,800,300,false,data) }, message{ message_param,50.0f,
	glm::vec3{0.0f,0.0f,0.0f}, glm::ortho(0.0f,800.0f,300.0f,0.0f), data }, text_shader{
	create_shader(constant::text_vertex_shader, constant::text_fragment_shader, data) } {
	message.set_pos(glm::vec3{ 400.0f,150.0f,1.0f }, data);
	return;
}

template<Message_window_type T1>
void Message_window<T1>::process(int index, Data_pv* data) {
	glfwMakeContextCurrent(window);
	if (!glfwWindowShouldClose(window)) {
		if constexpr (T1 == Message_window_type::Error) {
			while (!glfwWindowShouldClose(window))
				process1(data);
			for (int i{ 0 }; i < data->message_window.size(); i++)
				data->message_window[i].~Copy_holder();
			glfwTerminate();
			std::exit(0);
		}
		else
			process1(data);
	}
	else
		data->message_window[index].~Copy_holder();
	return;
}

template<Message_window_type T1>
void Message_window<T1>::process1(Data_pv* data) {
	glfwPollEvents();
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	message.render(text_shader, data);
	glfwSwapBuffers(window);
	return;
}

template<Message_window_type T1>
void Message_window<T1>::destruct1(Data_pv* data) {
	glfwMakeContextCurrent(data->main_window);
	glfwDestroyWindow(window);
	for (std::vector<Copy_holder<Message_window_pv*>>::iterator
		i{ data->message_window.begin() }; i != data->message_window.end(); i++)
		if (static_cast<Message_window<T1>*>(**i) == this) {
			data->message_window.erase(i);
			break;
		}
	return;
}

#endif