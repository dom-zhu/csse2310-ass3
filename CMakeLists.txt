cmake_minimum_required(VERSION 3.13)  # CMake version check
project(2310A)               # Create project "simple_example"
set(CMAKE_BUILD_TYPE Debug)
# Add main.c file of project root directory as source file
set(SOURCE_FILES_A 2310A.c path.c util.c deck.c)
set(SOURCE_FILES_DEALER 2310dealer.c path.c util.c deck.c)
set(SOURCE_FILES_B 2310B.c path.c util.c deck.c)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(2310A ${SOURCE_FILES_A})
add_executable(2310B ${SOURCE_FILES_B})
add_executable(2310Dealer ${SOURCE_FILES_DEALER})


set_property(TARGET 2310Dealer PROPERTY C_STANDARD 99)
set_property(TARGET 2310A PROPERTY C_STANDARD 99)
set_property(TARGET 2310B PROPERTY C_STANDARD 99)
