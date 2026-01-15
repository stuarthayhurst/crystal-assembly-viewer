#include <adwaita.h>

#include "panes.h"

static GtkWidget* paned_frame;
static GtkWidget* add_pane_button;
static GtkWidget* remove_pane_button;
static int num_panes;

static GtkWidget* create_pane_content() {
  GtkWidget* label = gtk_label_new("Hi there");
  gtk_widget_set_margin_start(label, 4);
  gtk_widget_set_margin_end(label, 4);
  gtk_widget_set_margin_top(label, 4);
  gtk_widget_set_margin_bottom(label, 4);
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_vexpand(label, TRUE);

  return label;
}

static void set_pane_button_sensitivity() {
  //Deactivate the add button when 4+ panes exist
  if (num_panes < 4) {
    gtk_widget_set_sensitive(add_pane_button, true);
  } else {
    gtk_widget_set_sensitive(add_pane_button, false);
  }

  //Deactivate the remove button when only 1 pane exists
  if (num_panes > 1) {
    gtk_widget_set_sensitive(remove_pane_button, true);
  } else {
    gtk_widget_set_sensitive(remove_pane_button, false);
  }
}

static void add_button_clicked_callback() {
  //Add a new pane
  add_new_pane(paned_frame, create_pane_content());
  num_panes++;

  //Resize and update buttons
  resize_panes(paned_frame, num_panes);
  set_pane_button_sensitivity();
}

static void remove_button_clicked_callback() {
  //Remove the last pane
  remove_last_pane(paned_frame);
  num_panes--;

  //Resize and update buttons
  resize_panes(paned_frame, num_panes);
  set_pane_button_sensitivity();
}

static void setup_content(GtkWidget* window) {
  //Create a vbox for the window content
  GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(vbox, 8);
  gtk_widget_set_margin_end(vbox, 8);
  gtk_widget_set_margin_top(vbox, 8);
  gtk_widget_set_margin_bottom(vbox, 8);
  gtk_window_set_child(GTK_WINDOW(window), vbox);

  //Create an hbox for the buttons
  GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_hexpand(button_box, false);
  gtk_box_append(GTK_BOX(vbox), button_box);

  //Create the add pane button
  add_pane_button = gtk_button_new_from_icon_name("list-add-symbolic");
  g_signal_connect(add_pane_button, "clicked", G_CALLBACK(add_button_clicked_callback), NULL);
  gtk_box_append(GTK_BOX(button_box), add_pane_button);

  //Create the remove pane button
  remove_pane_button = gtk_button_new_from_icon_name("list-remove-symbolic");
  g_signal_connect(remove_pane_button, "clicked", G_CALLBACK(remove_button_clicked_callback), NULL);
  gtk_box_append(GTK_BOX(button_box), remove_pane_button);

  //Create a frame for the panes
  paned_frame = gtk_frame_new(NULL);
  gtk_box_append(GTK_BOX(vbox), paned_frame);

  //Create the first pane and add to the frame
  GtkWidget* first_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_frame_set_child(GTK_FRAME(paned_frame), first_paned);
  num_panes = 1;

  //Create the initial pane content
  gtk_paned_set_start_child(GTK_PANED(first_paned), create_pane_content());

  set_pane_button_sensitivity();
}

static void activate_callback(GtkApplication* app) {
  GtkWidget* window = gtk_application_window_new(app);

  gtk_window_set_title(GTK_WINDOW(window), "Crystal");
  gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
  gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

  setup_content(window);

  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char* argv[]) {
  AdwApplication* app;

  app = adw_application_new("io.github.stuarthayhurst.Crystal", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate_callback), NULL);
  int result = g_application_run(G_APPLICATION(app), argc, argv);

  g_object_unref(app);
  return result;
}
