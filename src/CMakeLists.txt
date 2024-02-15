file(GLOB SOURCES *.cpp)

add_executable(main ${SOURCES})

llvm_map_components_to_libnames(llvm_libs support core analysis)

target_link_libraries(main PRIVATE ${llvm_libs})