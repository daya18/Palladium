#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cassert>
#include <unordered_map>
#include <functional>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vulkan/vulkan.hpp>

#include <stb_image.h>

#include <vk_mem_alloc.h>

#include <freetype/freetype.h>
#include FT_GLYPH_H
#include FT_BITMAP_H