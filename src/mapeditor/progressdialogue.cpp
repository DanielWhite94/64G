#include <cassert>
#include <cstring>

#include "progressdialogue.h"
#include "util.h"
#include "../engine/util.h"

using namespace MapEditor;

gint progressDialogueProgressBarPulseTimer(gpointer userData);
gboolean progressDialogueCancelButtonClicked(GtkWidget *widget, gpointer userData);

namespace MapEditor {
	void progressDialogueProgressFunctor(double progress, Engine::Util::TimeMs elapsedTimeMs, void *userData) {
		assert(progress>=0.0 && progress<=1.0);
		assert(userData!=NULL);

		ProgressDialogue *prog=(ProgressDialogue *)userData;

		// This is simply a wrapper
		prog->progressFunctor(progress, elapsedTimeMs);
	}

	ProgressDialogue::ProgressDialogue(const char *text, GtkWidget *parentWindow) {
		// Init
		window=NULL;
		progressBar=NULL;
		cancelled=false;
		userText=NULL;

		// Create dialogue and widgets
		window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(window), "Working...");
		gtk_window_set_modal(GTK_WINDOW(window), true);
		gtk_window_set_decorated(GTK_WINDOW(window), false);
		gtk_window_set_resizable(GTK_WINDOW(window), false);
		if (parentWindow!=NULL) {
			gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);
			gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parentWindow));
		} else
			gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

		GtkWidget *box=gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_container_add(GTK_CONTAINER(window), box);

		progressBar=gtk_progress_bar_new();
		gtk_widget_set_margin_start(progressBar, 30);
		gtk_widget_set_margin_end(progressBar, 30);
		gtk_widget_set_margin_top(progressBar, 10);
		gtk_widget_set_margin_bottom(progressBar, 10);
		gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progressBar), true);
		gtk_box_pack_start(GTK_BOX(box), progressBar, true, true, 0);

		cancelButton=gtk_button_new_with_label("Cancel");
		g_signal_connect(cancelButton, "clicked", G_CALLBACK(progressDialogueCancelButtonClicked), this);
		gtk_box_pack_start(GTK_BOX(box), cancelButton, true, true, 0);

		// Make a copy of the user's text
		userText=(char *)malloc(strlen(text)+1); // TODO: check return
		strcpy(userText, text);

		// Set progress bar text
		setText(text);

		// Add a timer to regularly pulse the progress bar
		pulseTimer=g_timeout_add(150, &progressDialogueProgressBarPulseTimer, this);

		// Show the window
		gtk_widget_show_all(window);
		gtk_widget_set_visible(cancelButton, false);
	}

	ProgressDialogue::~ProgressDialogue() {
		// Close window and destroy all widgets
		gtk_widget_destroy(window);

		// Disconnect pulse timer
		if (pulseTimer>0) {
			g_source_remove(pulseTimer);
			pulseTimer=0;
		}

		// Free memory
		free(userText);
	}

	bool ProgressDialogue::getCancelled(void) {
		return cancelled;
	}

	void ProgressDialogue::setShowCancelButton(bool visible) {
		// Show/hide button
		gtk_widget_set_visible(cancelButton, visible);
	}

	void ProgressDialogue::setProgress(double value) {
		// Update widget
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), value);

		// Disconnect pulse timer
		if (pulseTimer>0) {
			g_source_remove(pulseTimer);
			pulseTimer=0;
		}
	}

	void ProgressDialogue::setText(const char *str) {
		assert(str!=NULL);

		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), str);
	}

	gint ProgressDialogue::progressBarPulseTimer() {
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressBar));

		return G_SOURCE_CONTINUE;
	}

	gboolean ProgressDialogue::cancelButtonClicked(GtkWidget *widget) {
		// Set cancelled flag
		cancelled=true;

		return TRUE;
	}

	void ProgressDialogue::progressFunctor(double progress, Engine::Util::TimeMs elapsedTimeMs) {
		// Calculate some stuff
		char elapsedTimeStr[16];
		Engine::Util::sprintTime(elapsedTimeStr, elapsedTimeMs);

		Engine::Util::TimeMs remainingTimeMs=Engine::Util::calculateTimeRemaining(progress, elapsedTimeMs);

		// Generate string and update dialogue text
		char str[1024]; // TODO: this better
		if (remainingTimeMs!=0) {
			char remainingTimeStr[16];
			Engine::Util::sprintTime(remainingTimeStr, remainingTimeMs);
			sprintf(str, "%s %.3f%% %s (~%s remaining)", userText, progress*100.0, elapsedTimeStr, remainingTimeStr);
		} else
			sprintf(str, "%s %.3f%% %s", userText, progress*100.0, elapsedTimeStr);
		setText(str);

		// Update progress bar
		setProgress(progress);

		// Ensure progress dialogue can actually update
		utilDoGtkEvents();
	}
};

gint progressDialogueProgressBarPulseTimer(gpointer userData) {
	assert(userData!=NULL);

	ProgressDialogue *pd=(ProgressDialogue *)userData;

	return pd->progressBarPulseTimer();
}

gboolean progressDialogueCancelButtonClicked(GtkWidget *widget, gpointer userData) {
	assert(userData!=NULL);

	ProgressDialogue *pd=(ProgressDialogue *)userData;

	return pd->cancelButtonClicked(widget);
}
