#include <stdlib.h>

#include <adwaita.h>

#include "compiler_widget.h"

#include "detect_compilers.h"

static const char** compiler_paths = nullptr;
static unsigned int compiler_count = 0;

void send_compiler_infos(const struct compiler_info* infos, unsigned int count) {
  compiler_count = count;

  //Allocate the path array and copy the string points
  compiler_paths = malloc(sizeof(char*) * (compiler_count + 1));
  compiler_paths[compiler_count] = NULL;
  for (unsigned int i = 0; i < compiler_count; i++) {
    compiler_paths[i] = infos[i].path;
  }
}

void free_compiler_strings() {
  free(compiler_paths);
}

//Return the index of the selected compiler, or -1 if unselected
int get_compiler_index(GtkWidget* compiler_widget) {
  //GtkWidget* vbox = gtk_widget_get_first_child(compiler_widget);
  GtkWidget* hbox = gtk_widget_get_first_child(compiler_widget);
  GtkWidget* compiler_selector = gtk_widget_get_last_child(hbox);

  guint selected_index = gtk_drop_down_get_selected(GTK_DROP_DOWN(compiler_selector));
  if (selected_index == GTK_INVALID_LIST_POSITION) {
    return -1;
  }

  return selected_index;
}

GtkWidget* create_compiler_widget() {
  //Vertical box for the config and output
  GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

  //Horizontal box for the compiler config
  GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(vbox), hbox);

  //Create a text entry for the compiler arguments
  GtkWidget* compiler_arguments = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(compiler_arguments), "Compiler arguments, e.g. -O2");

  //Put the entry in the horizontal box, and pad it
  gtk_box_append(GTK_BOX(hbox), compiler_arguments);
  gtk_widget_set_hexpand(compiler_arguments, TRUE);
  gtk_widget_set_margin_start(compiler_arguments, 4);
  gtk_widget_set_margin_top(compiler_arguments, 4);

  //Create a compiler selection dropdown
  GtkWidget* compiler_selector = gtk_drop_down_new_from_strings(compiler_paths);
  gtk_box_append(GTK_BOX(hbox), compiler_selector);
  gtk_widget_set_margin_end(compiler_selector, 4);
  gtk_widget_set_margin_top(compiler_selector, 4);

  //Create a frame for the output
  GtkWidget* text_frame = gtk_frame_new(NULL);
  gtk_box_append(GTK_BOX(vbox), text_frame);
  gtk_widget_set_margin_start(text_frame, 4);
  gtk_widget_set_margin_end(text_frame, 4);
  gtk_widget_set_margin_bottom(text_frame, 4);

  //Add a read-only text area to the frame
  GtkWidget* text_view = gtk_text_view_new();
  gtk_frame_set_child(GTK_FRAME(text_frame), text_view);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), false);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), false);

  //Fill the area with the text box
  gtk_widget_set_hexpand(text_view, TRUE);
  gtk_widget_set_vexpand(text_view, TRUE);

  //Set the text buffer to a placeholder
  GtkTextBuffer* text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_set_text(text_buffer, "...", -1);

  return vbox;
}

void set_compiler_widget_compiling(GtkWidget* compiler_widget, bool compiling) {
  gtk_widget_set_sensitive(compiler_widget, !compiling);
}
