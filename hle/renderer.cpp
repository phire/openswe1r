// Copyright 2020 OpenSWE1R Maintainers
// Licensed under GPLv2 or any later version
// Refer to the included LICENSE.txt file.

#include <unicorn/unicorn.h>

extern "C" {
#include "renderer.h"
#include "../emulation.h"
}

#include "sceneGraph.h"
#include "export_glTF.h"

#include <array>
#include <cmath>
#include <stdexcept>

static bool s_dumping = false;
static std::map<uint32_t, Texture> textures;


static Address renderSceneGraph_fixup;

struct Mesh * getMesh(uint offset) {
  Address MeshPool = *(Address*)Memory(*(Address*)Memory(0xdeb110) + 4);
  Address mesh_addr = MeshPool + offset * sizeof(struct Mesh);
  return (struct Mesh *) Memory(mesh_addr);
}

struct Mesh_Primitive *getMeshPrimitive(Address addr, size_t offset)
{
  struct Mesh_Primitive* array = (struct Mesh_Primitive*)Memory(addr);
  return &array[offset];
}

void PrintMatrix(char *prefix, float* m)
{

  printf("%s| %-f %-f %-f %-f |\n", prefix, m[0], m[3], m[6], m[9]);
  printf("%s| %-f %-f %-f %-f |\n", prefix, m[1], m[4], m[7], m[10]);
  printf("%s| %-f %-f %-f %-f |\n", prefix, m[2], m[5], m[8], m[11]);
}

void PrintPrimitive(char *prefix, struct Mesh_Primitive* prim)
{
  if (prim == NULL)
    return;
//  printf("%s  %i indexes tex: %x\n", prefix, prim->indexCount, prim->texture_handle);
}

void PrintDraw(char *prefix, struct Mesh* mesh)
{
   printf("%s %i verts, %i primitives\n", prefix, mesh->num_vertices, mesh->num_primitives);

  for (int i = 0; i < mesh->num_primitives; i++)
  {

    PrintPrimitive(prefix, getMeshPrimitive(mesh->primitives, i));
  }
}

void PrintSceneGraph(Ptr<SceneGraphNode> node, int indent)
{
  char buf[30];
  int idx = 0;
  while (idx < indent && idx < sizeof(buf-1))
  {
    buf[idx++] = ' ';
    buf[idx++] = ' ';
  }
  buf[idx] = '\0';

  if (node->objectIndex > 0 || node->numChildren)
  {
    PrintMatrix(buf, node->baseTransformMatrix);
    if (node->extraTransformCount)
    {
      printf("--------------------------------------------------------------- Extra Matrix\n");
      PrintMatrix(buf, node->extraTransformMatrix);
    }
  }
  else
  {
    printf("%s %.*s <empty> \n", buf, 4, node->id);
  }



  if (node->objectIndex > 0) {
    printf("%snode: %.4s draw %i\n", buf, &node->id, node->objectIndex);
    PrintDraw(buf, getMesh(node->objectIndex));
  }
  if (node->numChildren == 0)
    return;
  printf("%s%i children:\n", buf, node->numChildren);
  Ptr<SceneGraphNode> child = node->firstChild;
  for (int i = 0; i < node->numChildren; i++)
  {
    if (child != NULL) {
      PrintSceneGraph(child, indent + 1);
      child = child->nextSibling;
    }
  }
}

std::array<float, 16> convertMatrix(float* orig) {
  std::array<float, 16> matrix;
  matrix[0]  = orig[0];
  matrix[1]  = orig[1];
  matrix[2]  = orig[2];
  matrix[3]  = 0.0;
  matrix[4]  = orig[3];
  matrix[5]  = orig[4];
  matrix[6]  = orig[5];
  matrix[7]  = 0.0;
  matrix[8]  = orig[6];
  matrix[9]  = orig[7];
  matrix[10] = orig[8];
  matrix[11] = 0.0;
  matrix[12] = orig[9];
  matrix[13] = orig[10];
  matrix[14] = orig[11];
  matrix[15] = 1.0;

  return matrix;
}

void dumpMesh(glTF_exporter& exporter, Ptr<SceneGraphNode> node, struct Mesh* mesh) {

  // Get vertex buffer
  std::vector<float> vertices(mesh->num_vertices * 3);
  float *vtx = (float*)Memory(mesh->vertexData_ptr);
  memcpy(vertices.data(), vtx, mesh->num_vertices * sizeof(float) * 3);

  json meshAttributes = exporter.uploadVerties(vertices);

  // Get all primitives
  std::vector<json> primitives;

  for (int i=0; i < mesh->num_primitives; i++)
  {
    Mesh_Primitive* prim = getMeshPrimitive(mesh->primitives, i);

    int mat_id = -1;

    auto material = *prim->material;
    {
      if (material.num_textures == 1) {
        auto texture = *material.textures;
        auto texture_handle = texture.direct3dtexture2;

        auto actual_texture = textures[texture_handle];

        if (actual_texture.width * actual_texture.height != 0)
        {
          mat_id = exporter.uploadTexture(actual_texture);
        }
      }
    }

    // Get indices
    std::vector<uint32_t> indices(prim->indexCount);
    uint32_t *index = (uint32_t*)Memory(prim->IndexData_ptr);
    memcpy(indices.data(), index, prim->indexCount * sizeof(uint32_t));

    for (auto &idx : indices)
    {
      if (idx >= mesh->num_vertices)
      {
        printf("Indices too high %i > %i\n", idx, mesh->num_vertices);
        idx = 0;
      }
    }

    primitives.push_back(exporter.uploadPrimitive(indices, meshAttributes, mat_id));
  }


  uint32_t mesh_id = exporter.addMesh(mesh->this_offset, primitives);

  // Convert transform matrix
  auto matrix = convertMatrix(node->baseTransformMatrix);


  exporter.addNode(mesh_id, matrix);
}

void dumpSceneGraph(glTF_exporter& exporter, Ptr<SceneGraphNode> node)
{
  if (node->objectIndex > 0) {
    dumpMesh(exporter, node, getMesh(node->objectIndex));
  }

  Ptr<SceneGraphNode> child = node->firstChild;
  for (int i = 0; i < node->numChildren; i++)
  {
    if (child != NULL) {
      dumpSceneGraph(exporter, child);
      child = child->nextSibling;
    }
  }
}

void renderSceneGraphCallback(void* _uc, Address address, void* user_data)
{
  uc_engine* uc = (uc_engine*)_uc;

  Address stackAddress;
  uc_reg_read(uc, UC_X86_REG_ESP, &stackAddress);
  uint32_t* stack = (uint32_t*)Memory(stackAddress);

  uint32_t return_addr = stack[0];

  // Ignore non-root root calls
  if (return_addr != 0x48f1f7)
  {
    printf("SceneGraph Hooked, Called from %x\n", return_addr);

    auto transform_ptr = Ptr<Ptr<TransformInfo>>(0xdf7f2c);
    auto transform = **transform_ptr;

    auto root = Ptr<SceneGraphNode>(stack[1]);
    //PrintSceneGraph(root, 0);
    if (s_dumping)
    {
      glTF_exporter exporter;

      float znear = (*transform.Frustum)[1] * 1000;
      float zfar  = (*transform.Frustum)[2] * 1000;
      float yfov  = transform.fovY * (M_PI / 180.0);
      auto matrix = convertMatrix(transform.ViewMatrix);

      exporter.addCamera(transform.fovY, znear, zfar, transform.AspectRatio, matrix);

      dumpSceneGraph(exporter, root);
      exporter.dump();
      s_dumping = false;
    }
  }

  if (1) {
    // Restore controlflow to render function
    uc_reg_write(uc, UC_X86_REG_EIP, &renderSceneGraph_fixup);
  } else {
    // Force a return without doing work
    stackAddress += 8;
    uc_reg_write(uc, UC_X86_REG_ESP, &stackAddress);
    uc_reg_write(uc, UC_X86_REG_EIP, &return_addr);
  }
}

void InitRenderer() {
  // Hook render scene node function

  Address halt = PatchHlt(0x48f180, &renderSceneGraph_fixup, 5);

  uint8_t *code = (uint8_t*)Memory(renderSceneGraph_fixup);

  AddHltHandler(halt, renderSceneGraphCallback, 0);

  printf("Installed renderSceneGraph hook, fixup at %x\n", renderSceneGraph_fixup);
}

void Renderer_DumpFrame() {
  s_dumping = true;
}

void Renderer_UploadTexture(uint32_t handle, texture_format format, int width, int height, void* data)
{
  auto expand5 = [] (uint16_t data) {
    return (data & 0x1f) << 3 | (data & 0x1f) >> 2;
  };
  auto expand4 = [] (uint16_t data) {
    return (data & 0x0f) << 4 | (data & 0x0f);
  };
  auto expand1 = [] (uint16_t data) {
    if ((data & 1) != 0)
      return 0xff;
    return 0x00;
  };

  Texture tex;
  tex.width = width;
  tex.height = height;
  tex.handle = handle;
  for (int i=0; i <= width*height; i++) {
    switch (format) {
      case RGBA_8888:
        tex.data.push_back(((uint32_t*)data)[i]);
        break;
      case BGRA_5551: {
        uint16_t pixel = ((uint16_t*)data)[i];

        uint32_t r = expand5(pixel >> 10);
        uint32_t g = expand5(pixel >> 5);
        uint32_t b = expand5(pixel);
        uint32_t a = expand1(pixel >> 15);

        tex.data.push_back(a << 24 | b << 16 | g << 8| r);
        break;
      }
      case BGRA_4444: {
        uint16_t pixel = ((uint16_t*)data)[i];

        uint32_t r = expand4(pixel >> 8);
        uint32_t g = expand4(pixel >> 4);
        uint32_t b = expand4(pixel);
        uint32_t a = expand4(pixel >> 12);

        tex.data.push_back(a << 24 | b << 16 | g << 8| r);
        break;
      }
    }
  }

  textures[handle] = std::move(tex);
}