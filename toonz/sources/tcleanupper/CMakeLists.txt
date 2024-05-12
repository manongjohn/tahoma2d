add_executable(tcleanup
    tcleanupper.cpp
)

target_link_libraries(tcleanup
    Qt6::Core
    Qt6::Widgets
    tfarm
    image
)
