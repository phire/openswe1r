#pragma once

#include <stdint.h>
#include "../emulation.h"

#include <array>

#pragma pack(push)

template <typename T>
class Ptr {
    Address ptr;
public:
    Ptr() {}

    Ptr(Address addr) : ptr(addr) {}

    T* data() {
        return reinterpret_cast<T*>(Memory(ptr));
    }

    T* operator->() {
        return data();
    }

    T& operator[](size_t idx) {
        return data()[idx];
    }

    T operator*() {
        return *data();
    }

    operator void*() const {
        return Memory(ptr);
    }
};


struct SceneGraphNode {
    char     id[8]; // Useally "Unknown\0", which is real helpful. Rootnode is "ROOT\0\0\0\0"
    uint32_t _ukn[32];
    int32_t objectIndex; // -1 when this node doesn't contain any drawable primitives
                         // Used to lookup the struct Mesh from the MeshPool
    Ptr<SceneGraphNode> parent; // Pointer to Struct SceneGraphNode
    uint32_t numChildren;
    Ptr<SceneGraphNode> firstChild; // Pointer to Struct SceneGraphNode
    Ptr<SceneGraphNode> nextSibling; // Pointer to Struct SceneGraphNode
    uint32_t _ukn_9c;
    uint32_t _ukn_a0[8];
    float baseTransformMatrix[12];
    uint32_t extraTransformCount;
    float extraTransformMatrix[]; // Some nodes have an extra transform matrix, which appears to be
                                  // applied to only the first extraTransformCount vertices
};

struct textureInfo {
    uint32_t ukn_0;
    uint32_t ukn_4;
    float ukn_8;
    float ukn_c;
    uint32_t ukn_10;
    uint32_t ukn_14;
    uint32_t ukn_18;
    uint32_t ukn_1c;
    uint32_t ukn_20;
    uint32_t ukn_24;
    uint32_t ukn_28;
    uint32_t ukn_2c;
    uint32_t ukn_30;
    uint32_t ukn_34;
    uint32_t ukn_38;
    uint32_t ukn_3c;
    uint32_t ukn_40;
    uint32_t ukn_44;
    uint32_t ukn_48;
    uint32_t ukn_4c;
    uint32_t ukn_50;
    uint32_t ukn_54;
    uint32_t ukn_58;
    uint32_t ukn_5c;
    uint32_t ukn_60;
    uint32_t ukn_64;
    uint32_t ukn_68;
    uint32_t ukn_6c;
    uint32_t ukn_70;
    uint32_t ukn_74;
    uint32_t ukn_78;
    uint32_t direct3dtexture2; // guess
    uint32_t ukn_80;
    uint32_t ukn_84;
    uint32_t ukn_88;
    uint32_t ukn_8c;
    uint32_t ukn_90;
};

struct MaterialInfo {
    char name[8];
    float ukn_8;
    float ukn_c;
    uint32_t ukn_10;
    uint32_t ukn_14;
    uint32_t ukn_18;
    uint32_t ukn_1c;
    uint32_t ukn_20;
    uint32_t ukn_24;
    uint32_t ukn_28;
    uint32_t ukn_2c;
    uint32_t ukn_30;
    uint32_t ukn_34;
    uint32_t ukn_38;
    uint32_t ukn_3c;
    uint32_t ukn_40;
    uint32_t ukn_44;
    uint32_t ukn_48;
    uint32_t ukn_4c;
    uint32_t ukn_50;
    uint32_t ukn_54;
    uint32_t ukn_58;
    uint32_t ukn_5c;
    uint32_t ukn_60;
    uint32_t ukn_64;
    uint32_t ukn_68;
    uint32_t ukn_6c;
    uint32_t ukn_70;
    uint32_t ukn_74;
    uint32_t ukn_78;
    uint32_t ukn_7c;
    uint32_t tex_width;
    uint32_t tex_height;
    uint32_t num_textures;
    uint32_t unk_8c;
    Ptr<textureInfo> textures; // Can be a pointer to multiple textures
                               // But the rendering side of the engine doesn't appear to support this
};

struct Mesh_Primitive {
    uint32_t prim_idx; // This increments with every primitive or two in a mesh. Might be base_index?
    uint32_t rendering_flags; // Transparency and stuff
    uint32_t vertex_format;  // vertex type: 0/1/2 = vertex only 3 = vertex + uv
    uint32_t blending_mode; // Might actually be more of a draw mode
    uint32_t indexCount;
    Address IndexData_ptr; // Pointer to array of uint32_t indices
    Address UvIndexData_ptr; // Used when vertex_format = 4 and blendmode < 3
                             // Otherwise vertex position and uv are linked to the main index
    Ptr<MaterialInfo> material;
    uint32_t  unk_20;

    // allows the game to add an offset to texture coordinates
    // Potentally used to to add random variation to repeating objects with the same mesh
    // Or to scroll textures on a mesh
    struct {
        float u;
        float v;
    } uvOffset;

    // Per-mesh color
    float alpha;
    float red;
    float green;
    float blue;

    float ukn_3c;
    float ukn_40;
    float ukn_44;
};

struct Mesh {
    char    id[4]; // Always seems to be "aes"
    uint32_t _unk[15];
    uint32_t this_offset; // The offset of this mesh in the Meshpool
    uint32_t visible; // maybe. The game only renders the mesh when this is non-zero.
                      // Always seems to be set to 4
    uint32_t max_blendmode;
    Ptr<float> vertexData_ptr;
    Ptr<float> uvData_ptr; // Just a guess
    Ptr<float> ukn_54;
    Ptr<float> colorData;
    float ukn_5c;
    float ukn_60;
    float ukn_64;
    float ukn_68;
    Ptr<Mesh_Primitive> primitives; // Pointer to array of Mesh_Primitive objects
    Address ukn_70;
    uint32_t num_vertices; // count of vertices in vertexData
    uint32_t num_uvs; // guess. Often seems to be the same as num_vertices
    uint32_t num_primitives;
    float    ukn_80; // Might be scale, might be distance from camera
    uint16_t render_count; // This gets decremented every time this mesh is drawn
    uint16_t enable; // Not really an enable flag. Gets set to 0 whenever render_count = 0
    uint8_t  unk_88;
    uint8_t  unk_89;
    uint8_t  unk_8a;
    uint8_t  unk_8b;
    float ukn_8c;
    float ukn_90;
};

struct LightInfo {

};

struct TransformInfo {
    int IsPrespective; // Seems to choose between an othro and perspective transform mode
    uint32_t unk_4;
    float ViewMatrix[12];
    float fovY;
    float unk_3c;
    float AspectRatio;
    float ukn_44;
    Ptr<std::array<float, 16>> Frustum;
    Ptr<uint32_t> TransformSingleFn;
    Ptr<uint32_t> transformMultipleFn;
    float ukn_84;
    float ukn_88;
    std::array<float, 4> ambientLight; // Added to per-vertex color at the end
    uint32_t countLights;
    Ptr<LightInfo> Lights;
};

#pragma pack(pop)

