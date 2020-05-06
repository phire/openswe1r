#pragma once

#include <stdint.h>
#include "emulation.h"

#pragma pack(push)


struct SceneGraphNode {
    char     id[8]; // Useally "Unknown\0", which is real helpful. Rootnode is "ROOT\0\0\0\0"
    uint32_t _ukn[32];
    int32_t objectIndex; // -1 when this node doesn't contain any drawable primitives
    Address parent;
    uint32_t numChildren;
    Address firstChild;
    Address nextSibling;
    uint32_t _ukn_9c;
    uint32_t _ukn_a0[8];
    float baseTransformMatrix[12];
    uint32_t extraTransformCount;
    float extraTransformMatrix[];
};

struct Mesh_Primitive {
    uint32_t prim_idx; // This increments with every primitive or two in a mesh. Might be base_index?
    uint32_t rendering_flags;
    uint32_t vertex_format;  // vertex type: 0/1/2 = vertex only 3 = vertex + uv
    uint32_t blending_mode; // Might actually be more of a draw mode
    uint32_t indexCount;
    Address IndexData_ptr;
    Address primitive_type;
    uint32_t texture_handle;
    uint32_t  unk_20;
    struct {
        float u;
        float v
    } uvOffset;
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
    uint32_t this_offset;
    uint32_t visible;
    uint32_t max_blendmode;
    Address vertexData_ptr;
    Address colorData_ptr; // Just a guess
    Address ukn_54;
    Address uvData_ptr;
    float ukn_5c;
    float ukn_60;
    float ukn_64;
    float ukn_68;
    Address primitives; // Triangle strips
    Address ukn_70;
    uint32_t num_vertices; // count of vertices in vertexData
    uint32_t ukn_7c; // often seems to be the same as num_vertices
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

#pragma pack(pop)

