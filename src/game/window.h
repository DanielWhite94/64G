class Window {
	public:
	Window();
	~Window();

	void displayHandler(void);
	void reshapeHandler(int w, int h);

	private:
		int id;
		int width, height;
};
