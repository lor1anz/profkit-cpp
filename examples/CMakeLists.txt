add_executable(profkit_basic basic/main.cpp)
add_executable(profkit_multithread multithread/main.cpp)
add_executable(profkit_empty_dump empty_dump/main.cpp)
add_executable(profkit_multiple_sessions multiple_sessions/main.cpp)

target_link_libraries(profkit_basic PRIVATE profkit::profkit)
target_link_libraries(profkit_multithread PRIVATE profkit::profkit)
target_link_libraries(profkit_empty_dump PRIVATE profkit::profkit)
target_link_libraries(profkit_multiple_sessions PRIVATE profkit::profkit)