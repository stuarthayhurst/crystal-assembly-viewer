#include <adwaita.h>

#include "compiler_widget.h"

/* TODO:
 - Compiler arguments
 - Compiler selection
 - Output section
 - Disable input
 - Add sensitive toggle
*/

GtkWidget* create_compiler_widget() {
  GtkWidget* label = gtk_label_new("Hi there");
  gtk_widget_set_margin_start(label, 4);
  gtk_widget_set_margin_end(label, 4);
  gtk_widget_set_margin_top(label, 4);
  gtk_widget_set_margin_bottom(label, 4);
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_vexpand(label, TRUE);

  return label;
}

void set_compiler_widget_compiling(GtkWidget* compiler_widget, bool compiling) {
  gtk_widget_set_sensitive(compiler_widget, !compiling);
}
