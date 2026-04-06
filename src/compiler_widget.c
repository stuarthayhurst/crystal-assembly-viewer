#include <adwaita.h>

#include "compiler_widget.h"

GtkWidget* create_compiler_widget() {
  //Vertical box for the config and output
  GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

  //Horizontal box for the compiler config
  GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_box_append(GTK_BOX(vbox), hbox);

  //Create a text entry for the compiler arguments
  GtkWidget* compiler_arguments = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(compiler_arguments), "Compiler arguments, e.g. -O2");

  //Put the entry in the horizontal box, and pad it
  gtk_box_append(GTK_BOX(hbox), compiler_arguments);
  gtk_widget_set_margin_start(compiler_arguments, 4);
  gtk_widget_set_margin_end(compiler_arguments, 4);
  gtk_widget_set_margin_top(compiler_arguments, 4);

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
