add_executable(profkit_basic basic/main.cpp)
add_executable(profkit_multithread multithread/main.cpp)

target_link_libraries(profkit_basic PRIVATE profkit::profkit)
target_link_libraries(profkit_multithread PRIVATE profkit::profkit)