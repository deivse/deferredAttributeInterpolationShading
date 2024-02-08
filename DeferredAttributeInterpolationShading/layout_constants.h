#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_LAYOUT_CONSTANTS
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_LAYOUT_CONSTANTS

#include <glbinding/gl/gl.h>

namespace layout {
using namespace gl;

enum class AtomicCounterBuffers : GLuint
{
    DAIS_TriangleCounter = 0
};

enum class UniformBuffers : GLuint
{
    SphereOffsets = 0,
    DAIS_Uniforms,
    DS_Uniforms
};

enum class ShaderStorageBuffers : GLuint
{
    DAIS_Triangles = 0,
    DAIS_Derivatives,
    Lights
};

enum class Uniforms : GLuint
{
    CameraPosition = 0,
    NumLights,
    MVPInverse
    // TODO: use uniform buffer
};

enum class TextureUnits : GLuint
{
    /// @warning keep all DS_* samplers corresponding to FBO layout at
    /// first positions
    DS_Color_DAIS_TriangleAddress = 0,
    DS_Normal,
    DS_Vertex,
    Albedo,
    DAIS_TriangleAddressMS
};

enum class ImageUnits : GLuint
{
    DAIS_Cache = 0,
    DAIS_Locks,
};

constexpr TextureUnits texSamplerForFBOAttachment(GLenum colorAttachment) {
    if (colorAttachment < gl::GLenum::GL_COLOR_ATTACHMENT0
        || colorAttachment > gl::GLenum::GL_COLOR_ATTACHMENT31)
        throw;
    if (static_cast<GLuint>(TextureUnits::DS_Color_DAIS_TriangleAddress) != 0)
        throw;

    return static_cast<TextureUnits>(
      static_cast<GLuint>(colorAttachment)
      - static_cast<GLuint>(gl::GLenum::GL_COLOR_ATTACHMENT0));
}

template<typename EnumT>
constexpr std::underlying_type_t<EnumT> location(EnumT value) {
    return static_cast<std::underlying_type_t<EnumT>>(value);
}

} // namespace layout

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_LAYOUT_CONSTANTS */
