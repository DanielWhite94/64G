#ifndef ENGINE_UTIL_H
#define ENGINE_UTIL_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace Engine {
	class Util {
	public:
		typedef long long int TimeMs;

		typedef bool (ProgressFunctor)(double progress, Util::TimeMs elapsedTimeMs, void *userData); // return true to continue, false to cancel the operation

		struct ProgressFunctorScaledData {
			double progressOffset;
			double progressMultiplier;

			TimeMs startTimeMs;

			ProgressFunctor *progressFunctor;
			void *progressUserData;
		};

		static int floordiv(int n, int d);

		static double angleFromXYToXY(double x1, double y1, double x2, double y2);

		static void clearConsoleLine();

		static bool randBool(void);
		static long long randIntInInterval(long long min, long long max);
		static double randFloatInInterval(double min, double max);
		static uint16_t rand16(void);
		static uint32_t rand32(void);
		static uint64_t rand64(void);

		static unsigned chooseWithProb(const double *probabilities, size_t count);

		static bool isDir(const char *path);
		static bool isFile(const char *path);
		static size_t getFileSize(const char *path);
		static bool makeDir(const char *path);
		static bool unlinkFile(const char *path);
		static bool unlinkDir(const char *path);

		static bool isImageWhite(const char *path); // returns true if given path represents an image with all pixels white

		static TimeMs getTimeMs(void);
		static void printTime(TimeMs timeMs);
		static void sprintTime(char *str, TimeMs timeMs); // str should have space for at least 16 characters (null byte included)

		static TimeMs calculateTimeRemaining(double progress, TimeMs elapsedTimeMs); // progress in range [0,1], if cannot estimate then returns -1

		// Calculate horizontal/vertical distances between points including wrapping around the edges as an option.
		// The wrappingDist function is the sum of x and y distances (e.g. manhattan/taxicab distance).
		static unsigned wrappingDistX(unsigned x1, unsigned x2, unsigned mapW) {
			assert(x1<mapW);
			assert(x2<mapW);

			// Note: could try something clever like std::min((x1-x2)&(mapW-1), (x2-x1)&(mapW-1)) but would only work for power of two map sizes?

			if (x2>x1)
				return std::min(x2-x1, x1+mapW-x2);
			else
				return std::min(x1-x2, x2+mapW-x1);
		}

		static unsigned wrappingDistY(unsigned y1, unsigned y2, unsigned mapH) {
			assert(y1<mapH);
			assert(y2<mapH);

			if (y2>y1)
				return std::min(y2-y1, y1+mapH-y2);
			else
				return std::min(y1-y2, y2+mapH-y1);
		}

		static unsigned wrappingDistManhatten(unsigned x1, unsigned y1, unsigned x2, unsigned y2, unsigned mapW, unsigned mapH) {
			assert(x1<mapW);
			assert(y1<mapH);
			assert(x2<mapW);
			assert(y2<mapH);

			return wrappingDistX(x1, x2, mapW)+wrappingDistY(y1, y2, mapH);
		}

		static unsigned wrappingDistEuclideanSq(unsigned x1, unsigned y1, unsigned x2, unsigned y2, unsigned mapW, unsigned mapH) {
			assert(x1<mapW);
			assert(y1<mapH);
			assert(x2<mapW);
			assert(y2<mapH);

			unsigned dx=wrappingDistX(x1, x2, mapW);
			unsigned dy=wrappingDistY(y1, y2, mapH);
			return dx*dx+dy*dy;
		}

		// Takes a coordinate and adds a delta/displacement.
		// Inputs: 0<=offsetX<mapW, -mapW<dx<mapW
		// Output: 0<=resultX<mapW
		// (similarly for Y)
		static unsigned addTileOffsetX(unsigned offsetX, int dx, unsigned mapW) {
			assert(0<=offsetX && offsetX<mapW);
			assert(-dx<(int)mapW && dx<(int)mapW);

			int result=(offsetX+dx+mapW)%mapW;

			assert(result>=0 && result<mapW);
			return result;
		}

		static unsigned addTileOffsetY(unsigned offsetY, int dy, unsigned mapH) {
			assert(0<=offsetY && offsetY<mapH);
			assert(-dy<(int)mapH && dy<(int)mapH);

			int result=(offsetY+dy+mapH)%mapH;

			assert(result>=0 && result<mapH);
			return result;
		}

		// Like addTileOffsetX/Y but with dx/dy=1
		static unsigned incTileOffsetX(unsigned offsetX, unsigned mapW) {
			assert(0<=offsetX && offsetX<mapW);
			assert(mapW>0);

			unsigned result=(offsetX+1!=mapW ? offsetX+1 : 0);
			assert(result<mapW);

			return result;
		}

		static unsigned incTileOffsetY(unsigned offsetY, unsigned mapH) {
			assert(0<=offsetY && offsetY<mapH);
			assert(mapH>0);

			unsigned result=(offsetY+1!=mapH ? offsetY+1 : 0);
			assert(result<mapH);

			return result;
		}

		static unsigned decTileOffsetX(unsigned offsetX, unsigned mapW) {
			assert(0<=offsetX && offsetX<mapW);
			assert(mapW>0);

			unsigned result=(offsetX!=0 ? offsetX-1 : mapW-1);
			assert(result<mapW);

			return result;
		}

		static unsigned decTileOffsetY(unsigned offsetY, unsigned mapH) {
			assert(0<=offsetY && offsetY<mapH);
			assert(mapH>0);

			unsigned result=(offsetY!=0 ? offsetY-1 : mapH-1);
			assert(result<mapH);

			return result;
		}

		// write given values to the given pointer in little-endian format
		// return value is given data pointer advanced by length of value
		static uint8_t *writeDataLE8(uint8_t *data, uint8_t value) {
			data[0]=value;
			return data+1;
		}

		static uint8_t *writeDataLE16(uint8_t *data, uint16_t value) {
			data[0]=(value&255);
			data[1]=(value>>8);
			return data+2;
		}

		static uint8_t *writeDataLE32(uint8_t *data, uint32_t value) {
			data[0]=(value&255);
			data[1]=((value>>8)&255);
			data[2]=((value>>16)&255);
			data[3]=((value>>24)&255);
			return data+4;
		}

		static uint8_t *writeDataLE64(uint8_t *data, uint64_t value) {
			data=writeDataLE32(data, (value&0xFFFFFFFFu));
			return writeDataLE32(data, (value>>32));
		}

		// inverse of write functions above
		static uint8_t readDataLE8(const uint8_t *data) {
			return data[0];
		}

		static uint16_t readDataLE16(const uint8_t *data) {
			return (((uint16_t)data[0])|(((uint16_t)data[1])<<8));
		}

		static uint32_t readDataLE32(const uint8_t *data) {
			return (((uint32_t)data[0])|(((uint32_t)data[1])<<8)|(((uint32_t)data[2])<<16)|(((uint32_t)data[3])<<24));
		}

		static uint64_t readDataLE64(const uint8_t *data) {
			return (((uint64_t)readDataLE32(data))|(((uint64_t)readDataLE32(data+4))<<32));
		}

	};

	// The following functions can be used where a Util::ProgressFunctor is expected.
	bool utilProgressFunctorString(double progress, Util::TimeMs elapsedTimeMs, void *userData); // Designed for console use. Prints a string from the user (pointed to by userData), followed by progress as a percentage, followed by elapsed time and estimated remaining time.
	bool utilProgressFunctorScaled(double progress, Util::TimeMs elapsedTimeMs, void *userData); // Invokes another progress functor with the progress scaled based on an offset and a multiplier. Can be used for multiple independant operations as calculates the elapsed time from the user data instead of using the value passed as an argument. ProgressFunctorScaledData pointer to be passed as userData.

	// A helper functions for when using ProgressFunctorScaledData logic.
	void utilProgressFunctorScaledInit(Util::ProgressFunctorScaledData *data, Util::ProgressFunctor *progressFunctor, void *progressUserData);
	bool utilProgressFunctorScaledInvoke(double progress, Util::ProgressFunctorScaledData *data);
};

#endif
