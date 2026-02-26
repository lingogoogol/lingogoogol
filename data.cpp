#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "version_f.h"
#include "data.h"
#include "surface.h"
#include "debug_f.h"
#include "graphic.h"

Data_pv::Data_pv() :
	text_resolution{ 256 }, main_window{}, logfile{ new std::ofstream{} }, current_time{}, last_time{},
	frame_period{}, shader{ new Shader{} }, font_file{}, character{}, text{}, state{}, button{}, message_window{},
	view{}, projection{}, main_window_width{}, main_window_height{} {
	logfile->open(".\\logfile\\" + get_time() + ".txt");
	glfwInit();
	main_window = create_window("a fun game", constant::main_window_width, constant::main_window_height, false, this);
	glfwSetWindowUserPointer(main_window, this);
	main_window_width = constant::main_window_width;
	main_window_height = constant::main_window_height;
	shader->text = create_shader(constant::text_vertex_shader, constant::text_fragment_shader, this);
	shader->button = create_shader(constant::button_vertex_shader, constant::button_fragment_shader, this);
	shader->transparent_cell = create_shader(constant::transparent_cell_vertex_shader, constant::transparent_cell_fragment_shader, this);
	//shader->transparent_object = create_shader(constant::transparent_object_vertex_shader, constant::transparent_object_fragment_shader, this);
	shader->tile_ambient = create_shader(constant::tile_vertex_shader, constant::tile_ambient_fragment_shader, this);
	//shader->tile_dir = create_shader(constant::tile_vertex_shader, constant::tile_dir_fragment_shader, this);
	shader->tile_point = create_shader(constant::tile_vertex_shader, constant::tile_point_fragment_shader, this);
	//shader->tile_spot = create_shader(constant::tile_vertex_shader, constant::tile_spot_fragment_shader, this);
	//shader->opaque_cell = create_shader(constant::opaque_cell_vertex_shader, constant::opaque_cell_fragment_shader, this);
	//shader->opaque_object = create_shader(constant::opaque_object_vertex_shader, constant::opaque_object_fragment_shader, this);
	FT_Library ft{};
	if (FT_Init_FreeType(&ft))
		handle_error(U"error: Failed to initialize FreeType.\n", this);
	if (FT_New_Face(ft, "C:\\Windows\\Fonts\\msjh.ttc", 0, &font_file))
		handle_error(U"Failed to create new FreeType face.\n", this);
	FT_Set_Pixel_Sizes(font_file, 0, text_resolution);
	glActiveTexture(GL_TEXTURE0);
	glUseProgram(shader->text);
	glUniform1i(glGetUniformLocation(shader->text, "tex"), 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	std::fstream data_file{ ".\\data.nop",std::ios::out | std::ios::app };
	data_file.close();
	data_file.open(".\\data.nop", std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	if (data_file.tellp() == 0) {
		data_file << constant::current_version;
	}
	else {
		Version<5> data_version{};
		data_file.seekg(0, std::ios::beg);
		data_file >> data_version;
		if (data_version < constant::last_necessary_version)
			handle_error(U"請先啟動版本" + constant::last_necessary_version.to_string32(), this);
		else {
			data_file.seekg(0, std::ios::beg);
			data_file << constant::current_version;
		}
	}
	check_GL_error(this);
	return;
}

Data_pv::~Data_pv() {
	return;
}

void Home_data::setup() {
	return;
}

void Environment_data::setup() {
	return;
}

void World_data::setup() {
	first_cursor = true;
	acceleration = 3.0f;
	max_speed = 2.5f;
	angle_of_view = 1.0f;
	friction = 0.5f;
	cursor_sensitivity = 0.001f;
	ambient_color = { 0.2f,0.2f,0.2f };
	last_cursor_pos_x = {};
	last_cursor_pos_y = {};
	camera_pos = { 0.0f,0.0f,0.0f };
	camera_dir = { 0.0f,0.0f,1.0f };
	camera_up = { 0.0f,1.0f,0.0f };
	camera_speed = { 0.0f,0.0f,0.0f };
	yaw = constant::pi_floor / 2.0f;
	pitch = 0.0f;
	tiles = {};
	blocks = {};
	pos_shininess = create_tex((float*)nullptr, GL_RGBA, main_window_width, main_window_height, this);
	normal_specular_strength = create_tex((float*)nullptr, GL_RGBA, main_window_width, main_window_height, this);
	color = create_tex((float*)nullptr, GL_RGBA, main_window_width, main_window_height, this);
	parent = create_tex((int*)nullptr, GL_RGBA, main_window_width, main_window_height, this);
	return;
}