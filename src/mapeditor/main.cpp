#include "main.h"

int main(int argc, char **argv) {
	MapEditor m;
	return 0;
}

namespace MapEditor {
	Main() {
		mainWindow=new MainWindow();
	}

	~Main() {
		delete mainWindow;
	}
};
