add_executable(mappng mappng.cpp)

set_target_properties(mappng PROPERTIES
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)

TARGET_LINK_LIBRARIES(mappng enginelib png)
