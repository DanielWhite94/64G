#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <sys/ioctl.h>
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
