add_executable(demogen demogen.cpp)

set_target_properties(demogen PROPERTIES
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)

TARGET_LINK_LIBRARIES(demogen enginelib)
