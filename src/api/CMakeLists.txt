add_library(compound_corrector STATIC
    compound_corrector.cxx
    compound_corrector.hxx
    bundle.cxx
    bundle.hxx
    nuspell_corrector.cc
    nuspell_corrector.h
)
# Pure C++14 — no ICU dependency.
target_link_libraries(compound_corrector PUBLIC Nuspell::nuspell)

# Resource packer
add_executable(pack_resources pack_resources.cxx)
target_link_libraries(pack_resources PRIVATE compound_corrector)

# Generate res.bundle whenever res/* files change.
set(RESOURCE_BUNDLE "${CMAKE_BINARY_DIR}/res.bundle")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/res")
file(GLOB RESOURCE_FILES CONFIGURE_DEPENDS
    ${RESOURCE_DIR}/en_US.aff
    ${RESOURCE_DIR}/en_US.dic
    ${RESOURCE_DIR}/ug
    ${RESOURCE_DIR}/acronyms.txt
)
add_custom_command(
    OUTPUT ${RESOURCE_BUNDLE}
    COMMAND pack_resources ${RESOURCE_DIR} ${RESOURCE_BUNDLE}
    DEPENDS ${RESOURCE_FILES} pack_resources
    COMMENT "Packing resource bundle"
)
add_custom_target(pack_resources_bundle ALL DEPENDS ${RESOURCE_BUNDLE})

add_executable(test_compound test_compound.cxx)
target_link_libraries(test_compound PRIVATE compound_corrector)
add_dependencies(test_compound pack_resources_bundle)

add_executable(spellcheck_filter spellcheck_filter.cxx)
target_link_libraries(spellcheck_filter PRIVATE compound_corrector)

add_executable(classify_missing_words classify_missing_words.cxx)
target_link_libraries(classify_missing_words PRIVATE compound_corrector)
