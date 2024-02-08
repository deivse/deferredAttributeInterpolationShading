#include "deferred_attribute_interpolation_shading.h"
#include "option_declaration_macros.h"
#include "scene.h"

#include <glm/vec2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <common.h>

#include <layout_constants.h>

namespace Algorithms {

DeferredAttributeInterpolationShading::
  ~DeferredAttributeInterpolationShading() {
    glDeleteFramebuffers(1, &FBO);
    glDeleteVertexArrays(1, &emptyVAO);
    glDeleteBuffers(1, &settingsUniformBuffer);
    glDeleteBuffers(1, &triangleSSBO);
    glDeleteBuffers(1, &derivativeSSBO);
    glDeleteBuffers(1, &atomicCounterBuffer);

    glDeleteTextures(1, &cacheTexture);
    glDeleteTextures(1, &locksTexture);
    glDeleteTextures(1, &triangleAddressFBOTexture);
    glDeleteTextures(1, &FBOdepthTexture);
}

void DeferredAttributeInterpolationShading::initialize() {
    DECLARE_OPTION(restoreDepth, true);

    logDebug("Initializing");
    createHashTableResources();
    createSSBO(triangleSSBO, layout::ShaderStorageBuffers::DAIS_Triangles);
    createSSBO(derivativeSSBO, layout::ShaderStorageBuffers::DAIS_Derivatives);
    createAtomicCounterBuffer();
    createSettingsUniformBuffer();
    createFBO(Variables::WindowSize);

    glLineWidth(2.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_DITHER);

    glGenVertexArrays(1, &emptyVAO);

    // Add rendering passes
    renderPasses.emplace_back(
      "Depth Prepass",
      [&]() -> void {
          glBindFramebuffer(GL_FRAMEBUFFER, FBO);
          glClear(GL_DEPTH_BUFFER_BIT);

          Scene::get().spheres.render();
      },
      "01_dais_depth_prepass");

    renderPasses.emplace_back(
      "Reset cache and triangle buffer",
      [&]() -> void {
          resetHashTable();
          auto* address = static_cast<GLuint*>(
            glMapNamedBuffer(atomicCounterBuffer, GL_WRITE_ONLY));
          *address = 0;
          if (glUnmapNamedBuffer(atomicCounterBuffer) == GL_FALSE) {
              logWarning("Triangle SSBO data store contents have become "
                         "corrupt during the time the data store was mapped, "
                         "reinitializing.");
              createAtomicCounterBuffer();
          }
      },
      nullptr);

    renderPasses.emplace_back(
      "Geometry Pass",
      [&]() -> void {
          glDepthFunc(GL_EQUAL);
          constexpr auto clearValue = glm::uvec4(-1);
          glClearNamedFramebufferuiv(FBO, GL_COLOR, 0,
                                     glm::value_ptr(clearValue));

          // update settings buffer
          auto* settings = reinterpret_cast<UniformSettingsBuffer*>(
            glMapNamedBuffer(settingsUniformBuffer, GL_WRITE_ONLY));
          settings->bitwiseModHashSize = hashTableSize - 1;
          settings->numTrianglesPerSphere
            = Scene::get().spheres.trianglesPerSphere;
          if (glUnmapNamedBuffer(settingsUniformBuffer) == GL_FALSE) {
              logWarning("Settings uniform buffer data store contents have "
                         "become corrupt during the "
                         "time the data store was mapped, reinitializing.");
              createSettingsUniformBuffer(UniformSettingsBuffer{
                hashTableSize - 1, static_cast<uint32_t>(
                                     Scene::get().spheres.trianglesPerSphere)});
          }

          Scene::get().update();
          Scene::get().spheres.render();
          glDepthFunc(GL_LESS);
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
      },
      "02_dais_geometry_pass");

    renderPasses.emplace_back(
      "Partial Derivatives Compute Pass",
      [&]() -> void {
          auto numTriangles = *static_cast<GLuint*>(
            glMapNamedBuffer(atomicCounterBuffer, GL_READ_ONLY));
          if (glUnmapNamedBuffer(atomicCounterBuffer) == GL_FALSE) {
              logWarning("Triangle SSBO data store contents have become "
                         "corrupt during the time the data store was mapped, "
                         "reinitializing.");
              createAtomicCounterBuffer();
          }

          if (numTriangles == 0) return;
          glDispatchCompute(numTriangles, 1, 1);
      },
      "03_dais_compute_pass");

    renderPasses.emplace_back(
      "Shading Pass",
      [&]() -> void {
          glDisable(GL_DEPTH_TEST);
          glClear(GL_COLOR_BUFFER_BIT);

          const auto cameraPosition = Variables::Transform.ModelViewInverse
                                      * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
          glUniform3fv(layout::location(layout::Uniforms::CameraPosition), 1,
                       &cameraPosition.x);

          // TODO: add to Uniform Buffer
          const glm::mat4 MVPInverse
            = glm::inverse(Variables::Transform.ModelViewProjection);
          glUniformMatrix4fv(layout::location(layout::Uniforms::MVPInverse), 1,
                             GL_FALSE, glm::value_ptr(MVPInverse));

          glBindTextureUnit(layout::location(layout::texSamplerForFBOAttachment(
                              gl::GLenum::GL_COLOR_ATTACHMENT0)),
                            triangleAddressFBOTexture);

          glBindVertexArray(emptyVAO);
          glDrawArrays(GL_TRIANGLES, 0, 3);
          glEnable(GL_DEPTH_TEST);
      },
      "04_dais_shading_pass");

    renderPasses.emplace_back(
      "Restore Z-Buffer",
      [&]() -> void {
          glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
          glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
          glBlitFramebuffer(0, 0, Variables::WindowSize.x,
                            Variables::WindowSize.y, 0, 0,
                            Variables::WindowSize.x, Variables::WindowSize.y,
                            GL_DEPTH_BUFFER_BIT, GL_NEAREST);
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
      },
      &restoreDepth);
}

void DeferredAttributeInterpolationShading::createFBO(
  const glm::ivec2& resolution) {
    logDebug("Creating FBO...");

    Tools::Texture::Create2D(triangleAddressFBOTexture, GL_R32I, resolution);
    Tools::Texture::Create2D(FBOdepthTexture, gl::GLenum::GL_DEPTH24_STENCIL8,
                             resolution);

    glDeleteFramebuffers(1, &FBO);
    glCreateFramebuffers(1, &FBO);

    glNamedFramebufferDrawBuffers(FBO, 1, &GL_COLOR_ATTACHMENT0);

    glNamedFramebufferTexture(FBO, GL_COLOR_ATTACHMENT0,
                              triangleAddressFBOTexture, 0);
    glNamedFramebufferTexture(FBO, GL_DEPTH_ATTACHMENT, FBOdepthTexture, 0);

    assert(glGetError() == GL_NO_ERROR);
    assert(glCheckNamedFramebufferStatus(FBO, GL_FRAMEBUFFER)
           == GL_FRAMEBUFFER_COMPLETE);
}

void DeferredAttributeInterpolationShading::createAtomicCounterBuffer() {
    logDebug("Creating atomic counter buffer...");

    glDeleteBuffers(1, &atomicCounterBuffer);
    glCreateBuffers(1, &atomicCounterBuffer);
    constexpr GLuint zero = 0;
    glNamedBufferStorage(atomicCounterBuffer, sizeof(GLuint), &zero,
                         GL_CLIENT_STORAGE_BIT | GL_MAP_WRITE_BIT
                           | GL_MAP_READ_BIT);
    glBindBufferBase(
      GL_ATOMIC_COUNTER_BUFFER,
      layout::location(layout::AtomicCounterBuffers::DAIS_TriangleCounter),
      atomicCounterBuffer);
}

void DeferredAttributeInterpolationShading::createSettingsUniformBuffer(
  std::optional<UniformSettingsBuffer> initialData) {
    logDebug("Creating settings uniform buffer...");

    glDeleteBuffers(1, &settingsUniformBuffer);
    glCreateBuffers(1, &settingsUniformBuffer);
    glNamedBufferStorage(settingsUniformBuffer, sizeof(UniformSettingsBuffer),
                         initialData ? &initialData : nullptr,
                         GL_CLIENT_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER,
                     layout::location(layout::UniformBuffers::DAIS_Settings),
                     settingsUniformBuffer);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void DeferredAttributeInterpolationShading::resetHashTable() {
    static std::vector<glm::uvec4> initialCacheData = [](size_t hashTableSize) {
        std::vector<glm::uvec4> data;
        data.reserve(hashTableSize);
        return data;
    }(hashTableSize);
    initialCacheData.resize(hashTableSize, glm::uvec4(-1));

    glTextureSubImage1D(cacheTexture, 0, 0, hashTableSize, GL_RGBA_INTEGER,
                        GL_UNSIGNED_INT, initialCacheData.data());
}

void DeferredAttributeInterpolationShading::createHashTableResources() {
    logDebug("Creating cache buffers...");

    constexpr auto createAndBindImageTexture
      = [](GLuint& texture, const gl::GLenum format,
           const GLsizei hashTableSize, const layout::ImageUnits unit) {
            Tools::Texture::Create1D(texture, format, hashTableSize);
            glBindImageTexture(layout::location(unit), texture, 0, GL_FALSE, 0,
                               GL_READ_WRITE, format);
        };

    createAndBindImageTexture(cacheTexture, gl::GLenum::GL_RGBA32UI,
                              hashTableSize, layout::ImageUnits::DAIS_Cache);
    createAndBindImageTexture(locksTexture, gl::GLenum::GL_R32UI, hashTableSize,
                              layout::ImageUnits::DAIS_Locks);

    resetHashTable();
}

void DeferredAttributeInterpolationShading::createSSBO(
  GLuint& ssbo, layout::ShaderStorageBuffers binding) {
    logDebug("Creating triangle buffer...");

    glDeleteBuffers(1, &ssbo);
    glCreateBuffers(1, &ssbo);

    constexpr auto dataSize = TRIANGLE_SIZE * MAX_TRIANGLE_COUNT;

    glNamedBufferStorage(ssbo, dataSize, nullptr, GL_CLIENT_STORAGE_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, layout::location(binding), ssbo);
}
} // namespace Algorithms
