#ifndef ENGINE_UTIL_H
#define ENGINE_UTIL_H

namespace Engine {
	class Util {
	public:
		static int floordiv(int n, int d);

		static double angleFromXYToXY(double x1, double y1, double x2, double y2);
	};
};

#endif
