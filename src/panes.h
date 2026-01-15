#ifndef CRYSTAL_PANES
#define CRYSTAL_PANES

void add_new_pane(GtkWidget* paned_frame, GtkWidget* pane_content);
void remove_last_pane(GtkWidget* paned_frame);

void resize_panes(GtkWidget* paned_frame, int num_panes);

#endif
