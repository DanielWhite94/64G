#ifndef ENGINE_UTIL_H
#define ENGINE_UTIL_H

#include <cstddef>

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
		static bool isFile(const char *path);
		static size_t getFileSize(const char *path);
		static bool makeDir(const char *path);
		static bool unlinkFile(const char *path);
		static bool unlinkDir(const char *path);

		static bool isImageWhite(const char *path); // returns true if given path represents an image with all pixels white

		typedef long long int TimeMs;
		static TimeMs getTimeMs(void);
		static void printTime(TimeMs timeMs);
		static void sprintTime(char *str, TimeMs timeMs); // str should have space for at least 16 characters (null byte included)

		static TimeMs calculateTimeRemaining(double progress, TimeMs elapsedTimeMs); // progress in range [0,1], if cannot estimate then returns 0
	};

	void utilProgressFunctorString(double progress, Util::TimeMs elapsedTimeMs, void *userData); // where userData points to a null terminated string
};

#endif
