#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_LAYOUT_CONSTANTS
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_LAYOUT_CONSTANTS

#include <glbinding/gl/gl.h>

using namespace gl;

enum class UniformBufferBindings : GLuint
{
    Lights = 0,
    SphereOffsets
};

enum class UniformLocations : GLuint
{
    CameraPosition = 0,
    NumLights
};

template<typename EnumT>
constexpr std::underlying_type_t<EnumT> getLocation(EnumT value) {
    return static_cast<std::underlying_type_t<EnumT>>(value);
}

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_LAYOUT_CONSTANTS */
