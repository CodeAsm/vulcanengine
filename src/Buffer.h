#pragma once
#include "VulkanBase.h"
#include "Mesh.h"
 
// Uploads CPU mesh data to GPU-local vertex/index buffers via a staging buffer.
GPUMesh uploadMesh(const VulkanBase& base, const MeshData& mesh);
void destroyGPUMesh(VkDevice device, GPUMesh& mesh);
 
