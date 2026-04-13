#ifndef CRYSTAL_COMPILER_WIDGET
#define CRYSTAL_COMPILER_WIDGET

#include <adwaita.h>

#include "detect_compilers.h"

void send_compiler_infos(const struct compiler_info* compiler_info, unsigned int count);
void free_compiler_strings();

int get_compiler_index(GtkWidget* compiler_widget);

GtkWidget* create_compiler_widget();
void set_compiler_widget_compiling(GtkWidget* compiler_widget, bool compiling);

#endif
