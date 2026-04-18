#ifndef CRYSTAL_COMPILER_WIDGET
#define CRYSTAL_COMPILER_WIDGET

#include <adwaita.h>

#include "detect_compilers.h"

void send_compiler_infos(const struct compiler_info* compiler_info, unsigned int count);
void free_compiler_strings();

int get_compiler_index(GtkWidget* compiler_widget);
char* get_user_compiler_arguments(GtkWidget* compiler_widget);

void display_compiler_widget_text_content(GtkWidget* compiler_widget, const char* text);

GtkWidget* create_compiler_widget();
void set_compiler_widget_compiling(GtkWidget* compiler_widget, bool compiling);
void set_compiler_widget_dark(GtkWidget* compiler_widget, bool dark);
void set_compiler_widget_syntax_highlighting(GtkWidget* compiler_widget, bool syntax_highlighting);

void append_language_path(const char* path);

#endif
