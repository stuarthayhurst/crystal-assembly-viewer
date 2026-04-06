#include <stdlib.h>
#include <string.h>

#include <adwaita.h>

#include "compiler_widget.h"
#include "panes.h"

#define MAX_NUM_PANES 4

static GtkWidget* window;
static GtkWidget* paned_frame;
static GtkWidget* add_pane_button;
static GtkWidget* remove_pane_button;
static GtkWidget* file_button;
static GtkWidget* file_label;
static GtkWidget* recompile_button;

static int num_panes = 0;
static bool compiling = false;
static GFile* opened_file = NULL;

static GtkWidget* compiler_widgets[MAX_NUM_PANES];

static void set_pane_button_sensitivity();

static GtkWidget* add_compiler_widget() {
  num_panes++;

  //Create and return a new widgets
  compiler_widgets[num_panes - 1] = create_compiler_widget();
  return compiler_widgets[num_panes - 1];
}

static void remove_last_compiler_widget() {
  num_panes--;
}

static void set_compiling(bool new_compiling) {
  compiling = new_compiling;

  //Toggle controls availability
  gtk_widget_set_sensitive(file_button, !compiling);
  gtk_widget_set_sensitive(recompile_button, !compiling);

  set_pane_button_sensitivity();

  for (int i = 0; i < num_panes; i++) {
    set_compiler_widget_compiling(compiler_widgets[i], compiling);
  }
}

static void compile_done() {
  set_compiling(false);
}

static void compile_start() {
  //Guard against file changes while compiling, not designed to prevent races
  if (compiling) {
    return;
  }

  //Disable recompilation button and widgets until it's done
  set_compiling(true);

  //Load the file content
  char* data;
  gsize size;
  if (!opened_file || !g_file_load_contents(opened_file, NULL, &data, &size, NULL, NULL)) {
    set_compiling(false);
    return;
  }

  g_message(data);
  g_free(data);

  compile_done();
}

static void set_pane_button_sensitivity() {
  //Deactivate both buttons when compiling
  if (compiling) {
    gtk_widget_set_sensitive(add_pane_button, false);
    gtk_widget_set_sensitive(remove_pane_button, false);
  }

  //Deactivate the add button when the maximum number of panes exists
  if (num_panes < MAX_NUM_PANES) {
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
  //Add a new pane with content
  add_new_pane(paned_frame, add_compiler_widget());

  //Resize and update buttons
  resize_panes(paned_frame, num_panes);
  set_pane_button_sensitivity();
}

static void remove_button_clicked_callback() {
  //Remove the last pane and content
  remove_last_compiler_widget();
  remove_last_pane(paned_frame);

  //Resize and update buttons
  resize_panes(paned_frame, num_panes);
  set_pane_button_sensitivity();
}

//Free the open file, if one is open
static void free_opened_file() {
  if (opened_file) {
    g_object_unref(opened_file);
    opened_file = NULL;
  }
}

static void file_opened(GObject* source, GAsyncResult* result, void*) {
  GFile* selected_file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, NULL);
  if (!selected_file) {
    return;
  }

  //Free any existing file and track the new one
  free_opened_file();
  opened_file = selected_file;

  //Set the file label to the file name
  char* file_path = g_file_get_basename(opened_file);
  gtk_label_set_text(GTK_LABEL(file_label), file_path);
  g_free(file_path);

  //Initial compile
  compile_start();
}

static void file_button_clicked_callback() {
  //Open the file picker, hand off to file_opened when chosen
  GtkFileDialog* file_dialog = gtk_file_dialog_new();
  gtk_file_dialog_open(file_dialog, GTK_WINDOW(window), NULL, file_opened, NULL);

  g_object_unref(file_dialog);
}

static void recompile_button_clicked_callback() {
  compile_start();
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
  GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(button_box, true);
  gtk_widget_set_halign(button_box, GTK_ALIGN_END);
  gtk_box_set_spacing(GTK_BOX(button_box), 8);

  //Create an hbox for the file elements
  GtkWidget* file_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(file_box, true);
  gtk_widget_set_halign(file_box, GTK_ALIGN_START);
  gtk_box_set_spacing(GTK_BOX(file_box), 8);

  //Create a button to open a file
  file_button = gtk_button_new_from_icon_name("document-open-symbolic");
  g_signal_connect(file_button, "clicked", G_CALLBACK(file_button_clicked_callback), NULL);
  gtk_box_append(GTK_BOX(file_box), file_button);

  //Create a label for the open file
  file_label = gtk_label_new("No file selected");
  gtk_box_append(GTK_BOX(file_box), file_label);

  //Create a button to recompile
  recompile_button = gtk_button_new_from_icon_name("view-refresh-symbolic");
  g_signal_connect(recompile_button, "clicked", G_CALLBACK(recompile_button_clicked_callback), NULL);
  gtk_box_append(GTK_BOX(file_box), recompile_button);

  //Create an hbox for the panel boxes
  GtkWidget* panel_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(vbox), panel_box);
  gtk_box_append(GTK_BOX(panel_box), file_box);
  gtk_box_append(GTK_BOX(panel_box), button_box);

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

  //Create the initial pane content
  gtk_paned_set_start_child(GTK_PANED(first_paned), add_compiler_widget());

  set_pane_button_sensitivity();
}

static void activate_callback(GtkApplication* app) {
  window = gtk_application_window_new(app);

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

  free_opened_file();
  g_object_unref(app);
  return result;
}
