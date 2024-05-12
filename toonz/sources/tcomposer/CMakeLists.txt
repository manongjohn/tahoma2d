add_executable(tcomposer
    tcomposer.cpp
)

target_link_libraries(tcomposer
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    toonzlib
    tfarm
    tnzstdfx
    sound
    image
    colorfx
    toonzqt
)
