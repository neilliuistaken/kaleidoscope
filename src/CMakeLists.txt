file(GLOB SOURCES *.cpp)

add_executable(main ${SOURCES})

llvm_map_components_to_libnames(llvm_libs
    support
    core
    analysis
    passes
    X86AsmParser
    X86CodeGen
    OrcJIT)

target_link_libraries(main PRIVATE ${llvm_libs})