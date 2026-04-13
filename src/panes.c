#include <adwaita.h>

#include "panes.h"

void add_new_pane(GtkWidget* paned_frame, GtkWidget* pane_content) {
  //Find the final pane
  GtkWidget* last_paned = NULL;
  GtkWidget* next_paned = gtk_frame_get_child(GTK_FRAME(paned_frame));
  while (next_paned != NULL) {
    last_paned = next_paned;
    next_paned = gtk_paned_get_end_child(GTK_PANED(last_paned));
  }

  //Add a new pane to the final pane
  GtkWidget* new_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_end_child(GTK_PANED(last_paned), new_paned);

  //Create the pane's content
  gtk_paned_set_start_child(GTK_PANED(new_paned), pane_content);
}

void remove_last_pane(GtkWidget* paned_frame) {
  //Find the final pane
  GtkWidget* last_paned = NULL;
  GtkWidget* next_paned = gtk_frame_get_child(GTK_FRAME(paned_frame));
  while (next_paned != NULL) {
    last_paned = next_paned;
    next_paned = gtk_paned_get_end_child(GTK_PANED(last_paned));
  }

  //Destroy the contents of the last pane
  gtk_paned_set_start_child(GTK_PANED(last_paned), NULL);

  //Destroy the last pane
  GtkWidget* last_paned_parent = gtk_widget_get_parent(last_paned);
  gtk_paned_set_end_child(GTK_PANED(last_paned_parent), NULL);
}

void resize_panes(GtkWidget* paned_frame, unsigned int num_panes) {
  int width = gtk_widget_get_width(paned_frame);
  int position = width / num_panes;

  //Resize each pane
  GtkWidget* paned = gtk_frame_get_child(GTK_FRAME(paned_frame));
  while (paned != NULL) {
    gtk_paned_set_position(GTK_PANED(paned), position);
    paned = gtk_paned_get_end_child(GTK_PANED(paned));
  }
}
