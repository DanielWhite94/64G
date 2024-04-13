/*

.....

want to add compiling from emap to smap

what does smap need? and what can we drop from emap?
emap has:
* images - used for web slippymap so not needed
* maptiled - used for slippymap (own/api)
	keep this - and want to make sure it is all fully generated before compiling (so can copy it over in full)
	will be useful for in game world map and minimap etc
	- simply copy
* textures - keep obviously
	- simply copy
* regions
	..... ????
* items - keep obviously
	- simply copy
* leaflet and css/html files not needed
* metadata
	needed, maybe copy or ??? need to look at it
* kingdoms
	???
* landmasses
	???

todo:
* continue adding smap cpp files to make them workable
* update server to use smap not emap
	also probably need to think about stuff that was in 'engine.cpp' or w/e
	which calculated ticks and ran things
	this needs moving to server stuff presumably

	turns out server doesn't even do any map stuff yet
	need to:
	* grab map argument from argv
	* load the map
	* run the map tick function as well as communicating with players

* add a function or w/e somewhere to create SMaps from EMaps
	this is used by the editor to save compiled output
	so basically reads one map and outputs another
	no need to even return an SMap instance or...?
	we just need the files to be written


	could be a function in the emap class e.g: smap=emap->compile(outpath)
	or a special constructor for the smap e.g: smap(outpath, emap)
	or a compule module which could do e.g. smap=Compile::compile(emap, outpath);
	or something else


*/
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
