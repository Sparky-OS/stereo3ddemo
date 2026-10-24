# KDBindings 1.1.0 (KDAB, MIT), vendored for builds whose distribution has no package of it:
# the headers of src/kdbindings at commit efb54c58c3c2fce280d7089617935ec265fe4e2d, unchanged.
add_library(KDBindings INTERFACE)
add_library(KDAB::KDBindings ALIAS KDBindings)
set_target_properties(KDBindings PROPERTIES INTERFACE_COMPILE_FEATURES cxx_std_17)
target_include_directories(KDBindings INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
