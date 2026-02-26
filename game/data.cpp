#include <lgo/lib/glad.h>
#include <lgo/lib/glfw.h>
#include <lgo/data/.h>

#include "block_d.h"
#include "data_d.h"
#include "surface_d.h"

Data::Data() :
	text_resolution{ 256 }, main_window{},
	logfile{ new lgo::file<char>{ ".\\logfile\\" + lgo::get_time() + ".txt" } }, current_time{}, last_time{},
	last_second{ 0 }, frame_count{}, frame_period{}, shader{ new Shader{} }, font_file{}, character{},
	text{}, state{}, button{}, message_window{}, view{}, projection{} {
	lgo::impl::get_data()->text_resolution = text_resolution;
	lgo::impl::get_data()->logfile = logfile;
	lgo::impl::get_data()->message_windows = &message_window;
	lgo::impl::get_data()->font_file = &font_file;
	lgo::impl::get_data()->chars = &character;
	lgo::impl::get_data()->main_window = &main_window;
	glfwInit();
	main_window = lgo::create_window("Game", constant::main_window_width, constant::main_window_height, false);
	shader->text = lgo::create_shader(lgo::impl::text_vertex_shader, lgo::impl::text_fragment_shader);
	shader->button = lgo::create_shader(constant::button_vertex_shader, constant::button_fragment_shader);
	shader->transparent_cell = lgo::create_shader(constant::transparent_cell_vertex_shader, constant::transparent_cell_fragment_shader);
	//...shader->transparent_object = lgo::create_shader(constant::transparent_object_vertex_shader, constant::transparent_object_fragment_shader);
	shader->tile_ambient = lgo::create_shader(constant::tile_vertex_shader, constant::tile_ambient_fragment_shader);
	//...shader->tile_dir = lgo::create_shader(constant::tile_vertex_shader, constant::tile_dir_fragment_shader);
	shader->tile_point = lgo::create_shader(constant::tile_vertex_shader, constant::tile_point_fragment_shader);
	//...shader->tile_spot = lgo::create_shader(constant::tile_vertex_shader, constant::tile_spot_fragment_shader);
	//...shader->opaque_cell = lgo::create_shader(constant::opaque_cell_vertex_shader, constant::opaque_cell_fragment_shader);
	//...shader->opaque_object = lgo::create_shader(constant::opaque_object_vertex_shader, constant::opaque_object_fragment_shader);
	FT_Library ft{};
	if (FT_Init_FreeType(&ft))
		handle_error(U"Failed to initialize FreeType.\n");
	if (FT_New_Face(ft, "C:\\Windows\\Fonts\\msjh.ttc", 0, &font_file))
		handle_error(U"Failed to create new FreeType face.\n");
	FT_Set_Pixel_Sizes(font_file, 0, text_resolution);
	glActiveTexture(GL_TEXTURE0);
	glUseProgram(shader->text);
	glUniform1i(glGetUniformLocation(shader->text, "tex"), 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	lgo::file<char> data_file{ ".\\data.nop" };
	/*if (data_file.tellp() == 0)
		data_file << constant::current_version;
	else {
		Version<5> data_version{};
		data_file.seekg(0, std::ios::beg);
		data_file >> data_version;
		if (data_version < constant::last_necessary_version)
			handle_error(U"請先啟動版本" + constant::last_necessary_version.to_string32());
		else {
			data_file.seekg(0, std::ios::beg);
			data_file << constant::current_version;
		}
	}*///...
}

Home_data::Home_data() {}

Home_data::~Home_data() {
	get_home_data();
}

Environment_data::Environment_data() {}

Environment_data::~Environment_data() {
	get_environment_data();
}

World_data::World_data() :
	first_cursor{ true }, acceleration{ 3.0f }, max_speed{ 2.5f }, angle_of_view{ 60.0f },
	friction{ 0.5f }, cursor_sensitivity{ 0.05f }, ambient_color{ 0.2f, 0.2f, 0.2f },
	select_length{ 5.0f }, selected_surface{}, selected_pos{}, last_cursor_pos_x{}, last_cursor_pos_y{},
	camera_pos{ 0.0f, 0.0f, 0.0f }, camera_dir{ 0.0f, 0.0f, 1.0f }, camera_up{ 0.0f, 1.0f, 0.0f },
	camera_speed{ 0.0f, 0.0f, 0.0f }, yaw{ 0.0f }, pitch{ 0.0f }, tiles{}, blocks{},
	pos_shininess{ lgo::create_tex(lgo::sc<float*>(nullptr), GL_RGBA, lgo::get_framebuffer_width(
		get_data()->main_window), lgo::get_framebuffer_height(get_data()->main_window)) },
	normal_specular_strength{ lgo::create_tex(lgo::sc<float*>(nullptr), GL_RGBA, lgo::get_framebuffer_width(
		get_data()->main_window), lgo::get_framebuffer_height(get_data()->main_window)) },
	color{ lgo::create_tex(lgo::sc<float*>(nullptr), GL_RGBA, lgo::get_framebuffer_width(
		get_data()->main_window), lgo::get_framebuffer_height(get_data()->main_window)) },
	parent{ lgo::create_tex(lgo::sc<int*>(nullptr), GL_RGBA, lgo::get_framebuffer_width(
		get_data()->main_window), lgo::get_framebuffer_height(get_data()->main_window)) } {}

World_data::~World_data() {
	get_world_data();
}

Data* get_data() {
	static auto data{ new Data{} };
	return data;
}

Home_data* get_home_data() {
	static Home_data* home_data{};
	static bool in{ false };
	Data* data{ get_data() };
	if (data->state == State::Home || data->state == State::Wait) {
		if (!in) {
			home_data = new Home_data{};
			in = true;
		}
	}
	else
		in = false;
	return home_data;
}

Environment_data* get_environment_data() {
	static Environment_data* environment_data{};
	static bool in{ false };
	Data* data{ get_data() };
	if (data->state == State::Environment || data->state == State::Wait) {
		if (!in) {
			environment_data = new Environment_data{};
			in = true;
		}
	}
	else
		in = false;
	return environment_data;
}

World_data* get_world_data() {
	static World_data* world_data{};
	static bool in{ false };
	Data* data{ get_data() };
	if (data->state == State::World || data->state == State::World_init || data->state == State::Wait) {
		if (!in) {
			world_data = new World_data{};
			in = true;
		}
	}
	else
		in = false;
	return world_data;
}