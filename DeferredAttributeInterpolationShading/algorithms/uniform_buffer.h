#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_UNIFORM_BUFFER
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_UNIFORM_BUFFER

#include <glbinding/gl/gl.h>
#include <optional>
#include <spdlog/spdlog.h>

#include <layout_constants.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

using namespace gl;

template<typename DataT>
struct UniformBufferObject
{
    GLuint buffer = 0;
    layout::UniformBuffers binding;

    void initialize(layout::UniformBuffers binding,
                    std::optional<DataT> initialData) {
        this->binding = binding;
        recreateBuffer(initialData);
    }

    void recreateBuffer(std::optional<DataT> initialData) {
        glDeleteBuffers(1, &buffer);
        glCreateBuffers(1, &buffer);
        glNamedBufferStorage(buffer, sizeof(DataT),
                             initialData ? &initialData : nullptr,
                             GL_CLIENT_STORAGE_BIT | GL_MAP_WRITE_BIT);
        glBindBufferBase(GL_UNIFORM_BUFFER, layout::location(binding), buffer);
    }

    class UniformBufferWriteRef
    {
        UniformBufferObject<DataT>& parent;
        DataT* data;

    public:
        DataT* operator->() { return data; }
        DataT& operator*() { return *data; }

        UniformBufferWriteRef(const UniformBufferWriteRef& other) = delete;
        UniformBufferWriteRef(UniformBufferWriteRef&& other) = delete;
        UniformBufferWriteRef& operator=(const UniformBufferWriteRef& other)
          = delete;
        UniformBufferWriteRef& operator=(UniformBufferWriteRef&& other)
          = delete;

        ~UniformBufferWriteRef() {
            if (glUnmapNamedBuffer(parent.buffer) != GL_TRUE) {
                spdlog::warn(
                  "Uniform buffer data store contents have become corrupt "
                  "during the time the data store was mapped, and the data "
                  "store contents have become undefined. Recreating buffer.");
                // not initializing data here, it will be potentially invalid
                // for one frame, which is not a big issue
                parent.recreateBuffer(std::nullopt);
            }
        }

    private:
        explicit UniformBufferWriteRef(UniformBufferObject<DataT>& parent,
                                       DataT* data)
          : parent(parent), data(data) {
            if (data == nullptr)
                throw std::runtime_error(
                  "Failed to map uniform buffer for write access.");
        }

        friend struct UniformBufferObject<DataT>;
    };

    UniformBufferWriteRef mapForWrite(BufferAccessMask additionalAccessFlags
                                      = BufferAccessMask::GL_NONE_BIT) {
        return UniformBufferWriteRef(
          *this, static_cast<DataT*>(glMapNamedBufferRange(
                   buffer, 0, sizeof(DataT),
                   GL_MAP_WRITE_BIT | additionalAccessFlags)));
    }

    ~UniformBufferObject() { glDeleteBuffers(1, &buffer); }
};

/**
 * @brief Uniform buffer data that is shared by both algorithms.
 *
 */
struct CommonUniformBufferData
{
    glm::vec4 cameraPosition;
    glm::mat4 MVPMatrix;
};

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_UNIFORM_BUFFER */
