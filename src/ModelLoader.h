#pragma once

#include "Mesh.h"
#include "VulkanBase.h"

#include <string>

class ModelLoader {
public:
    static ModelData load(const std::string& path);

private:
    static ModelData loadOBJ(const std::string& path);
#ifdef HAVE_ASSIMP
    static ModelData loadAssimp(const std::string& path);
#endif
};
