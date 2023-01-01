#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

using namespace Engine;

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
	const TimeMs minuteFactor=60;
	const TimeMs hourFactor=minuteFactor*60;
	const TimeMs dayFactor=hourFactor*24;

	// Convert to seconds.
	timeMs/=1000;

	// Print time.
	bool output=false;

	if (timeMs>=dayFactor) {
		TimeMs days=timeMs/dayFactor;
		timeMs-=days*dayFactor;
		printf("%llud", days);
		output=true;
	}

	if (timeMs>=hourFactor) {
		TimeMs hours=timeMs/hourFactor;
		timeMs-=hours*hourFactor;
		printf("%llih", hours);
		output=true;
	}

	if (timeMs>=minuteFactor) {
		TimeMs minutes=timeMs/minuteFactor;
		timeMs-=minutes*minuteFactor;
		printf("%llim", minutes);
		output=true;
	}

	if (timeMs>0 || !output)
		printf("%llis", timeMs);
}
