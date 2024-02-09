#include "deferred_attribute_interpolation_shading.h"
#include "option_declaration_macros.h"
#include "scene.h"

#include <array>

#include <glm/vec2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <common.h>

#include <layout_constants.h>

namespace Algorithms {

DeferredAttributeInterpolationShading::
  ~DeferredAttributeInterpolationShading() {
    glDeleteFramebuffers(1, &FBO);
    glDeleteVertexArrays(1, &emptyVAO);
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
    createUniformBuffer();
    createFBO(Variables::WindowSize);

    glLineWidth(2.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_DITHER);

    glGenVertexArrays(1, &emptyVAO);

    renderPasses.emplace_back(
      "Reset buffers",
      [&]() -> void {
          resetHashTable();
          auto* address = static_cast<GLuint*>(
            glMapNamedBuffer(atomicCounterBuffer, GL_WRITE_ONLY));
          *address = 0;
          if (glUnmapNamedBuffer(atomicCounterBuffer) == GL_FALSE) {
              logWarning(
                "Atomic counter buffer data store contents have become "
                "corrupt during the time the data store was mapped, "
                "reinitializing.");
              createAtomicCounterBuffer();
          }

          const auto cameraPosition = Variables::Transform.ModelViewInverse
                                      * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
          const glm::mat4 MVPInverse
            = glm::inverse(Variables::Transform.ModelViewProjection);

          {
              // update uniform buffer
              auto uniforms = uniformBuffer.mapForWrite();
              *uniforms = UniformBufferData{
                CommonUniformBufferData{
                  cameraPosition, Variables::Transform.ModelViewProjection},
                MVPInverse,
                Variables::Transform.Viewport,
                hashTableSize - 1,
                static_cast<GLuint>(Scene::get().spheres.trianglesPerSphere),
                Variables::Transform.Projection[3][2],
                Variables::Transform.Projection[2][2],
                MSAASampleCount};
          }
      },
      nullptr);

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
      "Geometry Pass",
      [&]() -> void {
          glDepthFunc(GL_EQUAL);
          constexpr auto clearValue = glm::uvec4(-1);
          glClearNamedFramebufferuiv(FBO, GL_COLOR, 0,
                                     glm::value_ptr(clearValue));

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

          // Always bind both textures to avoid warnings, one will be empty.
          glBindTextureUnit(layout::location(layout::texSamplerForFBOAttachment(
                              gl::GLenum::GL_COLOR_ATTACHMENT0)),
                            triangleAddressFBOTexture);
          glBindTextureUnit(
            layout::location(layout::TextureUnits::DAIS_TriangleAddressMS),
            triangleAddressFBOTextureMS);

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

void DeferredAttributeInterpolationShading::setMSAASampleCount(
  const uint8_t numSamples) {
    MSAASampleCount = numSamples;
    createFBO(Variables::WindowSize);
}

void DeferredAttributeInterpolationShading::createFBO(
  const glm::ivec2& resolution) {
    logDebug("Creating FBO...");

    Tools::Texture::Create2D(triangleAddressFBOTexture, GL_R32I,
                             MSAASampleCount ? glm::ivec2(1, 1) : resolution);
    Tools::Texture::Create2D(triangleAddressFBOTextureMS, GL_R32I,
                             MSAASampleCount ? resolution : glm::ivec2{1, 1},
                             MSAASampleCount > 0 ? MSAASampleCount : 1);
    Tools::Texture::Create2D(FBOdepthTexture, gl::GLenum::GL_DEPTH24_STENCIL8,
                             resolution, MSAASampleCount);

    glDeleteFramebuffers(1, &FBO);
    glCreateFramebuffers(1, &FBO);

    glNamedFramebufferDrawBuffers(FBO, 1, &GL_COLOR_ATTACHMENT0);

    glNamedFramebufferTexture(FBO, GL_COLOR_ATTACHMENT0,
                              MSAASampleCount > 0 ? triangleAddressFBOTextureMS
                                                  : triangleAddressFBOTexture,
                              0);
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

size_t DeferredAttributeInterpolationShading::customGui() {
    constexpr auto sizeToChoice = [](GLsizei size) -> int {
        switch (size) {
            case 256:
                return 0;
            case 512:
                return 1;
            case 1024:
                return 2;
            case 2048:
                return 3;
            case 4096:
                return 4;
            case 8192:
                return 5;
            case 16384:
                return 6;
            case 32768:
                return 7;
            default:
                return 5;
        }
    };
    static int choice = sizeToChoice(hashTableSize);
    constexpr auto hashTableSizeLabels = std::array{
      "256", "512", "1024", "2048", "4096", "8192", "16384", "32768"};
    constexpr auto choiceToSize
      = std::array{256, 512, 1024, 2048, 4096, 8192, 16384, 32768};

    if (ImGui::Combo("Hash Table Size", &choice, hashTableSizeLabels.data(),
                     hashTableSizeLabels.size())) {
        hashTableSize = choiceToSize[choice];
        createHashTableResources();
    }
    return 1;
}
} // namespace Algorithms
