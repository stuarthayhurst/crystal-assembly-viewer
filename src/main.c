#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <adwaita.h>

#include "compiler_widget.h"
#include "detect_compilers.h"
#include "panes.h"
#include "run_compiler.h"

#define MAX_NUM_PANES 4

static const char file_button_icon[] = "document-open-symbolic";
static const char recompile_button_icon[] = "view-refresh-symbolic";
static const char add_pane_button_icon[] = "list-add-symbolic";
static const char remove_pane_button_icon[] = "list-remove-symbolic";

static const char file_label_text[] = "No file selected";

static const char file_button_tooltip[] = "Open a file";
static const char recompile_button_tooltip[] = "Recompile the selected file";
static const char add_pane_button_tooltip[] = "Add a new compiler";
static const char remove_pane_button_tooltip[] = "Remove last compiler";

static const char app_name[] = "Crystal";
static const char app_id[] = "io.github.stuarthayhurst.Crystal";

static GtkWidget* window;
static GtkWidget* paned_frame;
static GtkWidget* add_pane_button;
static GtkWidget* remove_pane_button;
static GtkWidget* file_button;
static GtkWidget* file_label;
static GtkWidget* recompile_button;

static unsigned int num_panes = 0;
static bool compiling = false;
static GFile* opened_file = NULL;
static char* binary_path = NULL;

static GtkWidget* compiler_widgets[MAX_NUM_PANES];
static char* compiler_widget_strings[MAX_NUM_PANES];
static struct compiler_info* compiler_infos;

static void set_pane_button_sensitivity();

//Return the base path of the binary, must be freed by the caller
static char* detect_binary_path() {
  int buffer_size = sizeof(char) * 256;
  char* binary_path = NULL;

  //Fetch the path to the binary
  while (true) {
    binary_path = realloc(binary_path, buffer_size);

    int count = readlink("/proc/self/exe", binary_path, buffer_size);
    if (count < 0) {
      fprintf(stderr, "Failed to determine binary path (%d)\n", errno);

      free(binary_path);
      return NULL;
    } else if (count == buffer_size) {
      //Didn't allocate enough space, try again
      buffer_size *= 2;
      break;
    }

    binary_path[count] = '\0';
    break;
  }

  //Find the base path's length
  const int length = strlen(binary_path);
  int base_length = length;
  for (int i = length - 1; i >= 0; i--) {
    if (binary_path[i] != '/') {
      base_length--;
    } else {
      break;
    }
  }

  //Copy the base path and return it
  char* base_binary_path = strndup(binary_path, base_length);
  free(binary_path);
  return base_binary_path;
}

static void free_binary_path(char* binary_path) {
  free(binary_path);
}

static void update_text_dark_mode(AdwStyleManager* style_manager) {
  for (unsigned int i = 0; i < num_panes; i++) {
    set_compiler_widget_dark(compiler_widgets[i], adw_style_manager_get_dark(style_manager));
  }
}

static GtkWidget* add_compiler_widget() {
  num_panes++;

  //Create and return a new widget
  compiler_widgets[num_panes - 1] = create_compiler_widget();
  update_text_dark_mode(adw_style_manager_get_default());
  return compiler_widgets[num_panes - 1];
}

static void free_compiler_widget_text(unsigned int widget_index) {
  if (compiler_widget_strings[widget_index] != NULL) {
    free(compiler_widget_strings[widget_index]);
    compiler_widget_strings[widget_index] = NULL;
  }
}

static void remove_last_compiler_widget() {
  free_compiler_widget_text(num_panes - 1);
  num_panes--;
}

static void replace_compiler_widget_text(unsigned int widget_index, char* new_text) {
  display_compiler_widget_text_content(compiler_widgets[widget_index], new_text);

  free_compiler_widget_text(widget_index);
  compiler_widget_strings[widget_index] = new_text;
}

static void set_compiling(bool new_compiling) {
  compiling = new_compiling;

  //Toggle controls availability
  gtk_widget_set_sensitive(file_button, !compiling);
  gtk_widget_set_sensitive(recompile_button, !compiling);

  set_pane_button_sensitivity();

  for (unsigned int i = 0; i < num_panes; i++) {
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

  //Give up early if no file has been provided
  if (opened_file == NULL) {
    return;
  }

  //Disable recompilation button and widgets until it's done
  set_compiling(true);

  //Fetch information and run each compiler
  char* input_path = g_file_get_path(opened_file);
  for (unsigned int i = 0; i < num_panes; i++) {
    int index = get_compiler_index(compiler_widgets[i]);
    if (index == -1) {
      continue;
    }

    //Fetch information required for compilation
    char* user_compiler_arguments = get_user_compiler_arguments(compiler_widgets[i]);

    //Compile the file
    bool success = false;
    char* compiler_output = run_compiler(compiler_infos, index, user_compiler_arguments,
                                         input_path, &success);
    if (compiler_output != NULL) {
      replace_compiler_widget_text(i, compiler_output);
      set_compiler_widget_syntax_highlighting(compiler_widgets[i], success);
    }

    free(user_compiler_arguments);
  }

  free(input_path);
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
  file_button = gtk_button_new_from_icon_name(file_button_icon);
  g_signal_connect(file_button, "clicked", G_CALLBACK(file_button_clicked_callback), NULL);
  gtk_box_append(GTK_BOX(file_box), file_button);
  gtk_widget_set_tooltip_text(file_button, file_button_tooltip);

  //Create a label for the open file
  file_label = gtk_label_new(file_label_text);
  gtk_box_append(GTK_BOX(file_box), file_label);

  //Create a button to recompile
  recompile_button = gtk_button_new_from_icon_name(recompile_button_icon);
  g_signal_connect(recompile_button, "clicked", G_CALLBACK(recompile_button_clicked_callback), NULL);
  gtk_box_append(GTK_BOX(file_box), recompile_button);
  gtk_widget_set_tooltip_text(recompile_button, recompile_button_tooltip);

  //Create an hbox for the panel boxes
  GtkWidget* panel_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(vbox), panel_box);
  gtk_box_append(GTK_BOX(panel_box), file_box);
  gtk_box_append(GTK_BOX(panel_box), button_box);

  //Create the add pane button
  add_pane_button = gtk_button_new_from_icon_name(add_pane_button_icon);
  g_signal_connect(add_pane_button, "clicked", G_CALLBACK(add_button_clicked_callback), NULL);
  gtk_box_append(GTK_BOX(button_box), add_pane_button);
  gtk_widget_set_tooltip_text(add_pane_button, add_pane_button_tooltip);

  //Create the remove pane button
  remove_pane_button = gtk_button_new_from_icon_name(remove_pane_button_icon);
  g_signal_connect(remove_pane_button, "clicked", G_CALLBACK(remove_button_clicked_callback), NULL);
  gtk_box_append(GTK_BOX(button_box), remove_pane_button);
  gtk_widget_set_tooltip_text(remove_pane_button, remove_pane_button_tooltip);

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

  gtk_window_set_title(GTK_WINDOW(window), app_name);
  gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
  gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

  append_language_path(binary_path);
  setup_content(window);

  //Sync the text view's style
  AdwStyleManager* style_manager = adw_style_manager_get_default();
  g_signal_connect(style_manager, "notify::dark", G_CALLBACK(update_text_dark_mode), NULL);

  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char* argv[]) {
  AdwApplication* app;
  binary_path = detect_binary_path();

  //Initialise the widgets and strings
  for (unsigned int i = 0; i < MAX_NUM_PANES; i++) {
    compiler_widgets[i] = NULL;
    compiler_widget_strings[i] = NULL;
  }

  //Detect compilers
  unsigned int compiler_info_count = 0;
  compiler_infos = detect_unique_compilers(&compiler_info_count);
  send_compiler_infos(compiler_infos, compiler_info_count);

  //Log detected compilers
  for (unsigned int i = 0; i < compiler_info_count; i++) {
    printf("Found '%s'\n", compiler_infos[i].path);
  }

  app = adw_application_new(app_id, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate_callback), NULL);
  int result = g_application_run(G_APPLICATION(app), argc, argv);

  free_opened_file();
  g_object_unref(app);

  for (unsigned int i = 0; i < MAX_NUM_PANES; i++) {
    free_compiler_widget_text(i);
  }

  free_compiler_strings();
  free_compiler_array(compiler_infos, compiler_info_count);
  free_binary_path(binary_path);

  return result;
}
