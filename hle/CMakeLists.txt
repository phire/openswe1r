
add_library(racer_hle
    renderer.cpp
    export_glTF.cpp
)


set(JSON_BuildTests OFF CACHE INTERNAL "")

add_subdirectory(json)
#add_library(foo ...)
target_link_libraries(racer_hle PRIVATE nlohmann_json::nlohmann_json)
