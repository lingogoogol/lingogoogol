#ifndef INIT_F_H
#define INIT_F_H

enum class Data_pointer;

void init_logfile(Init_data* data);
void init_window(Init_data* data);
void init_shader(Init_data* data);
void init_font(Init_data* data);
void init_data_file(Init_data* data);

#endif