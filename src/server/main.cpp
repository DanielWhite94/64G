#include "../common/common.h"

#include "server.h"

using namespace Common;

Server *net=NULL;

bool tick(void);

int main(int argc, char const *argv[]) {
	net=new Server();
	while(tick())
		;
	delete net;
	return 0;
}

bool tick(void) {
	// Run network tick function
	if (!net->tick())
		return false;

	return true;
}
