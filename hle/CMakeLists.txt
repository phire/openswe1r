
add_library(racer_hle
    renderer.cpp
    export_glTF.cpp
)

set_target_properties(racer_hle PROPERTIES
    CXX_STANDARD 17
)

include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY git@github.com:g-truc/glm.git
    GIT_TAG 0.9.9.8
)

FetchContent_MakeAvailable(glm)


set(JSON_BuildTests OFF CACHE INTERNAL "")

add_subdirectory(json)
target_link_libraries(racer_hle PRIVATE nlohmann_json::nlohmann_json glm::glm)
