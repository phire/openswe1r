

#include "export_glTF.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <fstream>
#include <array>
#include <optional>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace glm
{
    void to_json(json& j, const glm::mat4x4 &m) {
        j = json {
            m[0][0], m[0][1], m[0][2], m[0][3],
            m[1][0], m[1][1], m[1][2], m[1][3],
            m[2][0], m[2][1], m[2][2], m[2][3],
            m[3][0], m[3][1], m[3][2], m[3][3]
        };
    }

    void from_json(const json& j, glm::mat4x4 &m)
    {
        std::array<double, 16> array{
            j[ 0].get<double>(),
            j[ 1].get<double>(),
            j[ 2].get<double>(),
            j[ 3].get<double>(),

            j[ 4].get<double>(),
            j[ 5].get<double>(),
            j[ 6].get<double>(),
            j[ 7].get<double>(),

            j[ 8].get<double>(),
            j[ 9].get<double>(),
            j[10].get<double>(),
            j[11].get<double>(),

            j[12].get<double>(),
            j[13].get<double>(),
            j[14].get<double>(),
            j[15].get<double>()
        };

        m = glm::make_mat4(array.data());
    }
}

void glTF_exporter::dump()
{

    size_t root_id = nodes.size();

    // Make a root which is the parent of all nodes
    {
        std::vector<int> children_idxes;
        for (int i = 0; i < nodes.size(); i++)
        {
            children_idxes.push_back(i);
        }

        auto rotation = glm::rotate(glm::mat4(1.0f), float(M_PI / 2.0), glm::vec3(1.0, 0.0, 0.0));

        nodes.push_back({
            {"name", "root"},
            {"matrix", rotation},
            {"children", children_idxes}
        });
    }


    json j {
        {"asset", {
            {"version", "2.0"},
            {"generator", "OpenSWE1R"}
        }},
        {"scenes", {
            {
                {"name", "framedump"},
                {"nodes", {root_id}}
            }
        }},
        {"scene", 0},
        {"buffers", {
            {
                {"byteLength", vertex_buffer.size() * sizeof(float)},
                {"uri", "test_vert.bin"}
            },
            {
                {"byteLength", index_buffer.size() * sizeof(uint32_t)},
                {"uri", "test_idx.bin"}
            }
        }},
        {"bufferViews", {
            {
                {"buffer", 0},
                {"byteLength", vertex_buffer.size() * sizeof(float)},
                //{"byteStride", 3 * sizeof(float)},
                {"name", "vertexBuffer"}
            },
            {
                {"buffer", 1},
                {"byteLength", index_buffer.size() * sizeof(uint32_t)},
                //{"byteStride", sizeof(uint32_t)},
                {"name", "indexBuffer"}
            }
        }},
        {"cameras", cameras},
        {"accessors", accessors},
        {"meshes", meshes},
        {"nodes", nodes}
    };

    {
        std::ofstream file;
        file.open("test.gltf");

        file << j;
    }

    {
        std::ofstream v_file("test_vert.bin", std::ios::out | std::ios::binary);
        v_file.write((char*)vertex_buffer.data(), vertex_buffer.size() * sizeof(float));
    }

    {
        std::ofstream i_file("test_idx.bin", std::ios::out | std::ios::binary);
        i_file.write((char*)index_buffer.data(), index_buffer.size() * sizeof(uint32_t));
    }
}

void glTF_exporter::addNode(uint32_t mesh_id, std::array<float, 16> matrix) {
    char buf[20];
    sprintf(buf, "mesh_%i", nodes.size());
    std::string name(buf);


    nodes.push_back(json {
        {"name", name},
        {"mesh", mesh_id},
        {"matrix", matrix},
    });
}

uint32_t glTF_exporter::addMesh(uint32_t idx,  std::vector<json> primitives) {
    auto cached = meshCache.find(idx);
    if (cached != meshCache.end()) {
        return cached->second;
    }

    uint32_t mesh_id = meshes.size();

    char buf[20];
    sprintf(buf, "mesh_%i", idx);

    std::string name(buf);

    meshes.push_back(json {
        {"name", name},
        {"primitives", primitives}
    });

    meshCache[idx] = mesh_id;
    return mesh_id;
}

json glTF_exporter::uploadVerties(std::vector<float> vertices) {
    size_t pos_offset = vertex_buffer.size() * sizeof(float);
    size_t vertex_accessor = accessors.size();

    accessors.push_back(json {
        {"bufferView", 0},
        {"byteOffset", pos_offset},
        {"componentType", 5126}, // Float
        {"count", vertices.size() / 3},
        {"type", "VEC3"}
    });

    vertex_buffer.insert(vertex_buffer.end(), vertices.begin(), vertices.end());

    return json {
        {"POSITION", vertex_accessor}
    };
}

json glTF_exporter::uploadPrimitive(std::vector<uint32_t> indices, json meshAttributes) {
    size_t offset = index_buffer.size() * sizeof(uint32_t);
    size_t index_accessor = accessors.size();

    accessors.push_back(json {
        {"bufferView", 1},
        {"byteOffset", offset},
        {"componentType", 5125}, // UINT
        {"count", indices.size()},
        {"type", "SCALAR"}
    });

    index_buffer.insert(index_buffer.end(), indices.begin(), indices.end());

    return json {
        {"indices", index_accessor},
        {"attributes", meshAttributes},
        {"mode", 4}
    };
}

void glTF_exporter::addCamera(float yfov, float znear, float zfar, float aspectRatio, std::array<float, 16> matrix) {
    size_t camera_id = cameras.size();

    cameras.push_back({
        {"name", "GameCamera"},
        {"type", "perspective"},
        {"perspective", {
            {"aspectRatio", aspectRatio},
            {"yfov", yfov},
            {"zfar", zfar},
            {"znear", znear}
        }}
    });

    // Invert the view matrix to get camera position
    auto inverted = glm::inverse(glm::make_mat4(matrix.data()));

    // Rotate to match the glTF coordnate space
    auto rotated = glm::rotate(inverted, float(M_PI / 2.0), glm::vec3(1.0f, 0.0f, 0.0f));

    nodes.push_back({
        {"name", "GameCameraNode"},
        {"camera", camera_id},
        {"matrix", rotated}
    });
}