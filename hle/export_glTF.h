
#pragma once

#include <array>
#include <stdint.h>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct Texture {
  uint32_t handle;
  int width;
  int height;
  std::vector<uint32_t> data;
};

class glTF_exporter {
public:

    void dump();

    uint32_t addMesh(uint32_t idx, std::vector<json> primitives);
    void addNode(uint32_t mesh_id, std::array<float, 16>);
    json uploadVerties(std::vector<float> vertices, std::vector<float> uvs);
    json uploadPrimitive(std::vector<uint32_t> indices, json meshAccessor, int mat_id);
    int uploadTexture(Texture &tex);

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
    std::vector<json> images;
    std::vector<json> textures;
    std::vector<json> materials;
    std::map<uint32_t, uint32_t> meshCache;
    std::map<uint32_t, int> textureMaping;

    std::vector<float> vertex_buffer;
    std::vector<float> uv_buffer;
    std::vector<uint32_t> index_buffer;
};