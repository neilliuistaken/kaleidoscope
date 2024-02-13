file(GLOB SOURCES *.cpp)

add_executable(main ${SOURCES})

target_link_libraries(main PRIVATE LLVM-14)