#ifndef MAPEDITOR_PROGRESSDIALOGUE_H
#define MAPEDITOR_PROGRESSDIALOGUE_H value

#include <gtk/gtk.h>

namespace MapEditor {

	class ProgressDialogue {
	public:
		ProgressDialogue(const char *text, GtkWidget *parentWindow);
		~ProgressDialogue();

		bool getCancelled(void);
		void setShowCancelButton(bool visible);

		void setProgress(double value);
		void setText(const char *str);

		gint progressBarPulseTimer();
		gboolean cancelButtonClicked(GtkWidget *widget);
	private:
		GtkWidget *window;
		GtkWidget *progressBar;

		guint pulseTimer;

		GtkWidget *cancelButton;
		bool cancelled;
	};

};

#endif
