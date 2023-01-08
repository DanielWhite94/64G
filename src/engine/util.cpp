#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

using namespace Engine;

namespace Engine {
	int Util::floordiv(int n, int d) {
		if ((n^d)>0)
			return n/d;
		else {
			ldiv_t res=ldiv(n,d);
			return res.rem ? res.quot-1 : res.quot;
		}
	}

	double Util::angleFromXYToXY(double x1, double y1, double x2, double y2) {
		return atan2(-(y2-y1), (x2-x1));
	}

	void Util::clearConsoleLine() {
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

		char fmtString[1024];
		sprintf(fmtString, "\r%%%is\r", w.ws_col);

		printf(fmtString, "");
		fflush(stdout);
	}

	bool Util::randBool(void) {
		return (Util::randIntInInterval(0, 2)==0);
	}

	long long Util::randIntInInterval(long long min, long long max) {
		assert(min<max);

		return (long long)floor(Util::randFloatInInterval(min, max));
	}

	double Util::randFloatInInterval(double min, double max) {
		assert(min<max);

		return (((double)rand())/RAND_MAX)*(max-min)+min;
	}


	unsigned Util::chooseWithProb(const double *probabilities, size_t count) {
		assert(probabilities!=NULL);

		// One or fewer items?
		if (count<2)
			return 0;

		// Find total.
		double probabilityTotal=0.0;
		for(size_t i=0; i<count; ++i) {
			assert(probabilities[i]>0.0);
			probabilityTotal+=probabilities[i];
		}
		assert(probabilityTotal>0.0);

		// Choose random value and return index associated with it.
		double randomValue=Util::randFloatInInterval(0.0, 1.0);
		randomValue*=probabilityTotal;
		double loopTotal=0.0;
		for(size_t i=0; i<count; ++i) {
			loopTotal+=probabilities[i];
			if (loopTotal>=randomValue)
				return i;
		}
		assert(false);
		return 0;
	}

	bool Util::isDir(const char *path) {
		assert(path!=NULL);

		struct stat pathStat;
		if (stat(path, &pathStat)!=0)
			return false;
		return S_ISDIR(pathStat.st_mode);
	}

	bool Util::isFile(const char *path) {
		assert(path!=NULL);

		struct stat pathStat;
		if (stat(path, &pathStat)!=0)
			return false;
		return S_ISREG(pathStat.st_mode);
	}

	size_t Util::getFileSize(const char *path) {
		struct stat st;
		if (stat(path, &st)==0)
			return st.st_size;
		else
			return 0;
	}

	bool Util::makeDir(const char *path) {
		assert(path!=NULL);

		return (mkdir(path, 0777)==0);
	}

	bool Util::unlinkFile(const char *path) {
		return (unlink(path)==0);
	}

	bool Util::unlinkDir(const char *path) {
		return (rmdir(path)==0);
	}

	bool Util::isImageWhite(const char *path) {
		assert(path!=NULL);

		// Create command to calculate mean value of pixels
		char command[1024]; // TODO: better
		sprintf(command, "convert %s -format \"%%[fx:(mean==1)]\" info:", path);

		// Run command, capturing output
		FILE *fp=popen(command, "r");
		if (fp==NULL)
			return false;

		char outputBuf[32]; // TODO: better
		if (fgets(outputBuf, sizeof(outputBuf), fp)==NULL) {
			pclose(fp);
			return false;
		}

		pclose(fp);

		return (outputBuf[0]=='1');
	}

	Util::TimeMs Util::getTimeMs(void) {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	void Util::printTime(TimeMs timeMs) {
		char str[16];
		sprintTime(str, timeMs);
		printf("%s", str);
	}

	void Util::sprintTime(char *str, TimeMs timeMs) {
		const long long int minuteFactor=60;
		const long long int hourFactor=minuteFactor*60;
		const long long int dayFactor=hourFactor*24;

		// Convert to seconds.
		long long int timeS=timeMs/1000;

		// Print time.
		char sub[16];
		str[0]='\0';
		bool output=false;

		if (timeS>=dayFactor) {
			TimeMs days=timeS/dayFactor;
			timeS-=days*dayFactor;
			sprintf(sub, "%llud", days);
			strcat(str, sub);
			output=true;
		}

		if (timeS>=hourFactor) {
			TimeMs hours=timeS/hourFactor;
			timeS-=hours*hourFactor;
			sprintf(sub, "%llih", hours);
			strcat(str, sub);
			output=true;
		}

		if (timeS>=minuteFactor) {
			TimeMs minutes=timeS/minuteFactor;
			timeS-=minutes*minuteFactor;
			sprintf(sub, "%llim", minutes);
			strcat(str, sub);
			output=true;
		}

		if (timeS>0 || !output)
			sprintf(str, "%llis", timeS);
	}

	Util::TimeMs Util::calculateTimeRemaining(double progress, TimeMs elapsedTimeMs) {
		assert(progress>=0.0 && progress<=1.0);

		if (progress<0.0001)
			return 0;
		if (progress>0.9999)
			return -1;

		Util::TimeMs remainingTimeMs=elapsedTimeMs*(1.0/progress-1.0);
		if (remainingTimeMs<1000)
			return 0;
		if (remainingTimeMs>=365ll*24ll*60ll*60ll*1000ll)
			return -1;

		return remainingTimeMs;
	}

	bool utilProgressFunctorString(double progress, Util::TimeMs elapsedTimeMs, void *userData) {
		assert(progress>=0.0 && progress<=1.0);
		assert(userData!=NULL);

		const char *string=(const char *)userData;

		// Clear old line.
		Util::clearConsoleLine();

		// Print start of new line, including users message and the percentage complete.
		printf("%s%.3f%% ", string, progress*100.0);

		// Append time elapsed so far.
		Util::printTime(elapsedTimeMs);

		// Attempt to compute estimated total time.
		Util::TimeMs remainingTimeMs=Util::calculateTimeRemaining(progress, elapsedTimeMs);
		if (remainingTimeMs>=0) {
			printf(" (~");
			Util::printTime(remainingTimeMs);
			printf(" remaining)");
		}

		// Flush output manually (as we are not printing a newline).
		fflush(stdout);

		return true;
	}

};
