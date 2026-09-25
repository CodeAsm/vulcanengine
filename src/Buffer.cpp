#include "Buffer.h"
#include <cstring>
 
static void uploadViaStaging(const VulkanBase& base, const void* data, VkDeviceSize size,
                              VkBufferUsageFlags usage, VkBuffer& outBuffer, VkDeviceMemory& outMemory) {
    VkBuffer staging;
    VkDeviceMemory stagingMemory;
    base.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       staging, stagingMemory);
 
    void* mapped;
    vkMapMemory(base.device, stagingMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, (size_t)size);
    vkUnmapMemory(base.device, stagingMemory);
 
    base.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       outBuffer, outMemory);
    base.copyBuffer(staging, outBuffer, size);
 
    vkDestroyBuffer(base.device, staging, nullptr);
    vkFreeMemory(base.device, stagingMemory, nullptr);
}
 
GPUMesh uploadMesh(const VulkanBase& base, const MeshData& mesh) {
    GPUMesh gpu;
    gpu.indexCount = (uint32_t)mesh.indices.size();
    gpu.materialIndex = mesh.materialIndex;
 
    VkDeviceSize vertexSize = sizeof(Vertex) * mesh.vertices.size();
    uploadViaStaging(base, mesh.vertices.data(), vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      gpu.vertexBuffer, gpu.vertexMemory);
 
    VkDeviceSize indexSize = sizeof(uint32_t) * mesh.indices.size();
    uploadViaStaging(base, mesh.indices.data(), indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      gpu.indexBuffer, gpu.indexMemory);
 
    return gpu;
}
 
void destroyGPUMesh(VkDevice device, GPUMesh& mesh) {
    if (mesh.vertexBuffer) vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
    if (mesh.vertexMemory) vkFreeMemory(device, mesh.vertexMemory, nullptr);
    if (mesh.indexBuffer) vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
    if (mesh.indexMemory) vkFreeMemory(device, mesh.indexMemory, nullptr);
    mesh = GPUMesh{};
}
 
