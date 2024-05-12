add_executable(tconverter
    tconverter.cpp
)

target_link_libraries(tconverter
    Qt6::Core
    toonzlib
    image
)
