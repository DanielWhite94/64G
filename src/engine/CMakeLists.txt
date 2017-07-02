cmake_minimum_required (VERSION 3.1.0)

file(GLOB SRC_FILES *.cpp physics/*.cpp graphics/*.cpp map/*.cpp )
add_executable(engine ${SRC_FILES})
add_library(enginelib ${SRC_FILES})

INCLUDE(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(engine ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES})

find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)

link_libraries(
	${SDL_LIBRARY}
	${SDLIMAGE_LIBRARY}
	physics
)

set_target_properties(engine enginelib PROPERTIES
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)
