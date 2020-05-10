
#pragma once

#include <array>
#include <stdint.h>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

class glTF_exporter {
public:

    void dump();

    uint32_t addMesh(uint32_t idx, std::vector<json> primitives);
    void addNode(uint32_t mesh_id, std::array<float, 16>);
    json uploadVerties(std::vector<float> vertices);
    json uploadPrimitive(std::vector<uint32_t> indices, json meshAccessor);

    void addCamera(float yfov, float znear, float zfar, float aspectRatio, std::array<float, 16>);
private:

    // class Node {
    //     uint32_t idx;
    //     std::array<float, 12>;
    // };


    std::vector<json> nodes;
    std::vector<json> meshes;
    std::vector<json> accessors;
    std::vector<json> cameras;
    std::map<uint32_t, uint32_t> meshCache;

    std::vector<float> vertex_buffer;
    std::vector<uint32_t> index_buffer;
};