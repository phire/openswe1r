

#include "export_glTF.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <fstream>

void glTF_exporter::dump()
{
    // Make a root which is the parent of all nodes
    {
        std::vector<int> children_idxes;
        for (int i = 0; i < nodes.size(); i++)
        {
            children_idxes.push_back(i);
        }

        nodes.push_back({
            {"name", "root"},
            {"children", children_idxes}
        });
    }

    json j {
        {"asset", {
            {"version", "2.0"},
            {"generator", "OpenSWE1R"}
        }},
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

    // for (uint32_t index = indices)
    // {
    //     index_buffer.push_back(index);
    // }
    index_buffer.insert(index_buffer.end(), indices.begin(), indices.end());

    return json {
        {"indices", index_accessor},
        {"attributes", meshAttributes},
        {"mode", 4}
    };
}