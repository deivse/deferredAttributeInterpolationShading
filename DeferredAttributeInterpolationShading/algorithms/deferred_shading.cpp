#include "deferred_shading.h"
#include "models/cornell_box.h"

#include <common.h>

#define DECLARE_OPTION(name, defaultValue) \
    bool& name = (options[#name] = defaultValue);
namespace Algorithms {

void DefferedShading::initialize() {
    DECLARE_OPTION(wireModel, false);
    createGBuffer();

    // Add rendering passes
    renderPasses.emplace_back(
      "Pass",
      [&]() -> void {
          // Set OpenGL state variables
          glEnable(GL_DEPTH_TEST);
          glLineWidth(2.0);
          glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

          glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          Tools::DrawCornellBox();

          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

          glBindFramebuffer(GL_FRAMEBUFFER, 0);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          Tools::Texture::Show2D(colorTextures[0], Variables::WindowSize.x / 2,
                                 0, Variables::WindowSize.x / 2,
                                 Variables::WindowSize.y / 2);
          Tools::Texture::Show2D(colorTextures[1], Variables::WindowSize.x / 2,
                                 Variables::WindowSize.y / 2,
                                 Variables::WindowSize.x / 2,
                                 Variables::WindowSize.y / 2);
      },
      "shader");
}

void DefferedShading::createGBuffer() {
    std::array colorAttachments{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};

    GLuint depthStencilTex{};
    Tools::Texture::Create2D(depthStencilTex, gl::GLenum::GL_DEPTH24_STENCIL8,
                             Variables::WindowSize);

    for (auto& texture : colorTextures) {
        Tools::Texture::Create2D(texture, gl::GLenum::GL_RGBA8,
                                 Variables::WindowSize);
    }

    // Create a framebuffer object ...
    glCreateFramebuffers(1, &gBufferFBO);
    glNamedFramebufferDrawBuffers(gBufferFBO, colorAttachments.size(),
                                  colorAttachments.data());

    // and attach color and depth textures
    for (size_t i = 0; i < colorAttachments.size(); i++) {
        glNamedFramebufferTexture(gBufferFBO, colorAttachments[i], colorTextures[i],
                                  0);
    }
    glNamedFramebufferTexture(gBufferFBO, gl::GLenum::GL_DEPTH_STENCIL_ATTACHMENT,
                              depthStencilTex, 0);

    assert(glGetError() == GL_NO_ERROR);
    assert(glCheckNamedFramebufferStatus(gBufferFBO, GL_FRAMEBUFFER)
           == GL_FRAMEBUFFER_COMPLETE);
}

} // namespace Algorithms
