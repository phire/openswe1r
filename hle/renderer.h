// Copyright 2020 OpenSWE1R Maintainers
// Licensed under GPLv2 or any later version
// Refer to the included LICENSE.txt file.

#pragma once

void InitRenderer();

void Renderer_DumpFrame();

typedef enum  {
    RGBA_8888,
    BGRA_4444,
    BGRA_5551,
} texture_format;

void Renderer_UploadTexture(uint32_t handle, texture_format format, int width, int height, void* data);