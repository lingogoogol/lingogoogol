#ifndef DATA_F_H
#define DATA_F_H

enum class State;

struct Shader;

class Data;
class Home_data;
class Environment_data;
class World_data;

Data* get_data();
Home_data* get_home_data();
Environment_data* get_environment_data();
World_data* get_world_data();

#endif