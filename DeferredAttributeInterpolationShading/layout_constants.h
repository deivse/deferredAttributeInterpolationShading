#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_LAYOUT_CONSTANTS
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_LAYOUT_CONSTANTS

#include <glbinding/gl/gl.h>

namespace layout {
using namespace gl;

enum class UniformBuffers : GLuint
{
    Lights = 0,
    SphereOffsets
};

enum class Uniforms : GLuint
{
    CameraPosition = 0,
    NumLights
};

enum class TextureSamplers : GLuint
{
    /// @warning keep all Deferred* samplers corresponding to FBO layout at
    /// first positions
    DeferredColor = 0,
    DeferredNormal,
    DeferredVertex,
    Albedo,
};

constexpr TextureSamplers texSamplerForFBOAttachment(GLenum colorAttachment) {
    if (colorAttachment < gl::GLenum::GL_COLOR_ATTACHMENT0
        || colorAttachment > gl::GLenum::GL_COLOR_ATTACHMENT31)
        throw;
    if (static_cast<GLuint>(TextureSamplers::DeferredColor) != 0) throw;

    return static_cast<TextureSamplers>(
      static_cast<GLuint>(colorAttachment)
      - static_cast<GLuint>(gl::GLenum::GL_COLOR_ATTACHMENT0));
}

template<typename EnumT>
constexpr std::underlying_type_t<EnumT> location(EnumT value) {
    return static_cast<std::underlying_type_t<EnumT>>(value);
}

} // namespace layout

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_LAYOUT_CONSTANTS */
