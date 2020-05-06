// Copyright 2020 OpenSWE1R Maintainers
// Licensed under GPLv2 or any later version
// Refer to the included LICENSE.txt file.

#include <unicorn/unicorn.h>

extern "C" {

#include "renderer.h"
#include "../emulation.h"

}

#include "sceneGraph.h"

static Address renderSceneGraph_fixup;

struct SceneGraphNode * getSceneGraphNode(Address addr) {
  return (struct SceneGraphNode*) Memory(addr);
}

struct Mesh * getMesh(uint offset) {
  Address MeshPool = *(Address*)Memory(*(Address*)Memory(0xdeb110) + 4);
  //struct Mesh** MeshPool_data = (struct Mesh**) Memory(MeshPool);
  Address* all_meshes = (Address*)Memory(MeshPool);
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
  // // I suspect this matrix is ordered in another way
  // printf("%s| %-f %-f %-f %-f |\n", prefix, m[0], m[1], m[2], m[3]);
  // printf("%s| %-f %-f %-f %-f |\n", prefix, m[4], m[5], m[6], m[7]);
  // printf("%s| %-f %-f %-f %-f |\n", prefix, m[8], m[9], m[10], m[11]);

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

void PrintSceneGraph(struct SceneGraphNode *node, int indent)
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
  struct SceneGraphNode *child = getSceneGraphNode(node->firstChild);
  for (int i = 0; i < node->numChildren; i++)
  {
    if (child != NULL) {
      PrintSceneGraph(child, indent + 1);
      child = getSceneGraphNode(child->nextSibling);
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

    struct SceneGraphNode *root = getSceneGraphNode(stack[1]);
    PrintSceneGraph(root, 0);
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
  printf("Fixup: ");

  for(int i = 0; i < 12; i++)
    printf("%02x ", code[i]);

  printf("\n");

  AddHltHandler(halt, renderSceneGraphCallback, 0);

  printf("Installed renderSceneGraph hook, fixup at %x\n", renderSceneGraph_fixup);
}