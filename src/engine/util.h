#ifndef ENGINE_UTIL_H
#define ENGINE_UTIL_H

namespace Engine {
	class Util {
	public:
		static int floordiv(int n, int d);

		static double angleFromXYToXY(double x1, double y1, double x2, double y2);

		static void clearConsoleLine();

		static bool randBool(void);
		static long long randIntInInterval(long long min, long long max);
		static double randFloatInInterval(double min, double max);

		static unsigned chooseWithProb(const double *probabilities, size_t count);

		static bool isDir(const char *path);

		typedef unsigned long long int TimeMs;
		static TimeMs getTimeMs(void);
	};
};

#endif
