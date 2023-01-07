#ifndef MAPEDITOR_PROGRESSDIALOGUE_H
#define MAPEDITOR_PROGRESSDIALOGUE_H

#include "../engine/util.h"

#include <gtk/gtk.h>

namespace MapEditor {

	// Progress functor which can be passed to Gen functions such as modifyTiles to update a ProgressDialogue passed in via userData.
	// Handles updating progress bar percentage and dialogue text to show progress and timings.
	// Also handles cancel button if enabled.
	bool progressDialogueProgressFunctor(double progress, Engine::Util::TimeMs elapsedTimeMs, void *userData);

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

		bool progressFunctor(double progress, Engine::Util::TimeMs elapsedTimeMs);
	private:
		GtkWidget *window;
		GtkWidget *progressBar;

		guint pulseTimer;

		GtkWidget *cancelButton;
		bool cancelled;

		char *userText;
	};

};

#endif
