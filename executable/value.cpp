#include "value.h"
#include "surface.h"

Data_pv::Data_pv():
	text_resolution{ 256 }, main_window{}, logfile{ new std::ofstream{} }, current_time{}, last_time{},
	frame_period{}, shader{}, font_file{}, character{}, text{}, state{}, button{}, message_window{},
	view{}, projection{}, main_window_width{}, main_window_height{} {
	return;
}

Data_pv::~Data_pv() {
	return;
}

void Init_data::setup() {
	start_width = 800;
	start_height = 600;
	return;
}

void Home_data::setup() {
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