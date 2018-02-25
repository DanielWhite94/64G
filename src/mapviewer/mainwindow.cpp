#include <iostream>

#include "mainwindow.h"

MainWindow::MainWindow() : m_button("Hello World") {
	set_border_width(10);

	m_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_button_clicked));
	add(m_button);

	m_button.show();
}

MainWindow::~MainWindow() {
}

void MainWindow::on_button_clicked() {
  std::cout << "Hello World" << std::endl;
}
