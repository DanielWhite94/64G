#include "util.h"

namespace MapEditor {
	bool utilPromptForYesNo(const char *title, const char *prompt, GtkWidget *parentWindow) {
		int result=utilPromptCustom(title, prompt, parentWindow, "Yes", "No", NULL);
		return (result==0);
	}

	void utilPromptForOk(const char *title, const char *prompt, GtkWidget *parentWindow) {
		utilPromptCustom(title, prompt, parentWindow, "OK", NULL);
	}

	int utilPromptCustom(const char *title, const char *prompt, GtkWidget *parentWindow, const char *firstButton, ...) {
		// Create dialog.
		GtkWidget *dialog=gtk_dialog_new_with_buttons(title, GTK_WINDOW(parentWindow), (GtkDialogFlags)(GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT), firstButton, 0, NULL);
		if (dialog==NULL)
			return false;

		// Add buttons.
		unsigned num=1;
		va_list ap;
		va_start(ap, firstButton);
		const char *text;
		while((text=va_arg(ap, const char *))!=NULL)
			gtk_dialog_add_button(GTK_DIALOG(dialog), text, num++); // TODO: Check return.

		// Add prompt.
		if (prompt!=NULL) {
			GtkWidget *contentArea=gtk_dialog_get_content_area(GTK_DIALOG(dialog));
			GtkWidget *label=gtk_label_new(prompt);
			gtk_container_add(GTK_CONTAINER(contentArea), label);
		}
		gtk_widget_show_all(dialog);

		// Run dialog and grab result.
		int result=gtk_dialog_run(GTK_DIALOG(dialog));
		if (result<0)
			result=-1;

		gtk_widget_destroy(dialog);

		return result;
	}

	void utilDoGtkEvents(void) {
		for(unsigned i=0; i<4 && gtk_events_pending(); ++i)
			gtk_main_iteration_do(false);
	}
};
