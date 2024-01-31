#include "deferred_attribute_interpolation_shading.h"
#include "option_declaration_macros.h"
#include "scene.h"

#include <glm/vec2.hpp>
#include <common.h>

#include <layout_constants.h>

namespace Algorithms {

DeferredAttributeInterpolationShading::
  ~DeferredAttributeInterpolationShading() {
    glDeleteFramebuffers(1, &FBO);
    glDeleteVertexArrays(1, &emptyVAO);
    glDeleteBuffers(1, &settingsUniformBuffer);
    glDeleteBuffers(1, &triangleSSBO);

    glDeleteTextures(1, &cacheTexture);
    glDeleteTextures(1, &locksTexture);
    glDeleteTextures(1, &triangleAddressFBOTexture);
}

void DeferredAttributeInterpolationShading::initialize() {
    logDebug("Initializing");
    createHashTableResources();
    createTriangleBuffer();
    createFBO(Variables::WindowSize);

    glLineWidth(2.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glGenVertexArrays(1, &emptyVAO);

    // Add rendering passes
    renderPasses.emplace_back(
      "Depth Prepass",
      [&]() -> void {
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
          glClear(GL_DEPTH_BUFFER_BIT);

          Scene::get().update();
          Scene::get().spheres.render();
      },
      "01_dais_depth_prepass");

    renderPasses.emplace_back(
      "Geometry Pass",
      [&]() -> void {
          glDepthFunc(GL_EQUAL);
          glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
          glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
          glDisable(GL_DITHER);

          Scene::get().update();
          Scene::get().spheres.render();
          glDepthFunc(GL_LESS);
          glEnable(GL_DITHER);

      },
      "02_dais_geometry_pass");

    renderPasses.emplace_back(
      "Shading Pass",
      [&]() -> void {
          glDisable(GL_DEPTH_TEST);
          glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
          glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

          glBindTextureUnit(layout::location(layout::texSamplerForFBOAttachment(
                              gl::GLenum::GL_COLOR_ATTACHMENT0)),
                            triangleAddressFBOTexture);

          glBindVertexArray(emptyVAO);
          glDrawArrays(GL_TRIANGLES, 0, 3);

          glEnable(GL_DEPTH_TEST);
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
      },
      "04_dais_shading_pass");
}

void DeferredAttributeInterpolationShading::createFBO(
  const glm::ivec2& resolution) {
    logDebug("Creating FBO...");

    Tools::Texture::Create2D(triangleAddressFBOTexture, GL_R32UI, resolution);

    glDeleteFramebuffers(1, &FBO);
    glCreateFramebuffers(1, &FBO);


    constexpr auto colorAttachment0 = gl::GLenum::GL_COLOR_ATTACHMENT0;
    glNamedFramebufferDrawBuffers(FBO, 1, &colorAttachment0);

    glNamedFramebufferTexture(FBO, GL_COLOR_ATTACHMENT0,
                              triangleAddressFBOTexture, 0);

    assert(glGetError() == GL_NO_ERROR);
    assert(glCheckNamedFramebufferStatus(FBO, GL_FRAMEBUFFER)
           == GL_FRAMEBUFFER_COMPLETE);
}

void DeferredAttributeInterpolationShading::createHashTableResources() {
    logDebug("Creating cache buffers...");

    constexpr auto createAndBindImageTexture
      = [](GLuint& texture, const gl::GLenum format,
           const GLsizei hashTableSize) {
            Tools::Texture::Create1D(texture, format, hashTableSize);
            glBindImageTexture(layout::location(layout::ImageUnits::DAIS_Cache),
                               texture, 0, GL_FALSE, 0, GL_READ_WRITE, format);
        };

    createAndBindImageTexture(cacheTexture, gl::GLenum::GL_RGBA32UI,
                              hashTableSize);
    createAndBindImageTexture(locksTexture, gl::GLenum::GL_R32UI,
                              hashTableSize);

    settings.bitwiseModHashSize = hashTableSize - 1;

    glDeleteBuffers(1, &settingsUniformBuffer);
    glCreateBuffers(1, &settingsUniformBuffer);
    glNamedBufferStorage(settingsUniformBuffer, sizeof(Settings), &settings,
                         GL_CLIENT_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER,
                     layout::location(layout::UniformBuffers::DAIS_Settings),
                     settingsUniformBuffer);
}

void DeferredAttributeInterpolationShading::createTriangleBuffer() {
    logDebug("Creating triangle buffer...");

    glDeleteBuffers(1, &triangleSSBO);
    glCreateBuffers(1, &triangleSSBO);

    constexpr auto WRITE_INDEX_SIZE = 4;
    constexpr auto TRIANGLE_SIZE = 96;
    constexpr auto MAX_TRIANGLE_COUNT = 10000;

    constexpr auto dataSize
      = WRITE_INDEX_SIZE + TRIANGLE_SIZE * MAX_TRIANGLE_COUNT;

    std::array<std::byte, dataSize> data{}; 

    glNamedBufferData(triangleSSBO, dataSize, data.data(),
                      gl::GLenum::GL_STATIC_COPY);
    glBindBufferBase(
      GL_SHADER_STORAGE_BUFFER,
      layout::location(layout::ShaderStorageBuffers::DAIS_Triangles),
      triangleSSBO);
}
} // namespace Algorithms
