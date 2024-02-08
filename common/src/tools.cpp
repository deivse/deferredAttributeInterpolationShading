//-----------------------------------------------------------------------------
//  [PGR2] Some helpful common functions
//  19/02/2014
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#include <iostream>
#include <tools.h>
#include <globals.h>
#include <fmt/format.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>

namespace Statistic {
float FPS = 0.0f; // Current FPS

namespace Frame {
    int GPUTime = 0; // GPU frame time
    int CPUTime = 0; // CPU frame time
    int ID = 0;      // Id of the frame
};                   // namespace Frame

namespace GPUMemory {
    int DedicatedMemory = 0; // Size of dedicated GPU memory
    int TotalMemory = 0;     // Size of total GPU memory
    int AllocatedMemory = 0; // Size of allocated GPU memory
    int FreeMemory = 0;      // Size of available GPU memory
    int EvictedMemory = 0;   // Size of available GPU memory
};                           // namespace GPUMemory
};                           // end of namespace Statistic

namespace OpenGL {
Program::Program(GLuint _id)
  : id(_id), MVPMatrix(glGetUniformLocation(id, "u_MVPMatrix")),
    ModelMatrix(glGetUniformLocation(id, "u_ModelMatrix")),
    ViewMatrix(glGetUniformLocation(id, "u_ViewMatrix")),
    ModelViewMatrix(glGetUniformLocation(id, "u_ModelViewMatrix")),
    ModelViewMatrixInverse(
      glGetUniformLocation(id, "u_ModelViewMatrixInverse")),
    ProjectionMatrix(glGetUniformLocation(id, "u_ProjectionMatrix")),
    NormalMatrix(glGetUniformLocation(id, "u_NormalMatrix")),
    Viewport(glGetUniformLocation(id, "u_Viewport")),
    ZOffset(glGetUniformLocation(id, "u_ZOffset")),
    VariableInt(glGetUniformLocation(id, "u_VariableInt")),
    VariableFloat(glGetUniformLocation(id, "u_VariableFloat")),
    FrameCounter(glGetUniformLocation(id, "u_FrameCounter")) {}

std::vector<Program> programs;
GLuint Query[NumQueries];
}; // end of namespace OpenGL

namespace Tools {

// Very simple GPU timer (timer cannot be nested with other timer or
// GL_TIME_ELAPSED query
GPUTimer::~GPUTimer() {
    if (queryStart) glDeleteQueries(1, &queryStart);
    if (queryStop) glDeleteQueries(1, &queryStop);
}

void GPUTimer::start(bool sync) {
    if (queryStart == 0) {
        glCreateQueries(GL_TIMESTAMP, 1, &queryStart);
        glCreateQueries(GL_TIMESTAMP, 1, &queryStop);
    }
    time = 0;
    if (sync) glFinish();
    glQueryCounter(queryStart, GL_TIMESTAMP);
}

void GPUTimer::stop(bool sync) const {
    if (sync) glFinish();
    if (time == 0) {
        glQueryCounter(queryStop, GL_TIMESTAMP);
    }
}

unsigned int
  GPUTimer::get(bool micro) { // micro - show in microsec, otherwise in nanosec
    if ((time == 0) && (queryStart > 0) && (queryStop > 0)) {
        // Unbind query buffer if bound
        QueryBufferRelease queryBufferRelease;

        GLuint64 start_time = 0, stop_time = 0;
        glGetQueryObjectui64v(queryStart, GL_QUERY_RESULT, &start_time);
        glGetQueryObjectui64v(queryStop, GL_QUERY_RESULT, &stop_time);
        time = stop_time - start_time;
        timeTotal += time;
        counter++;
    }
    return micro ? static_cast<unsigned int>(time * 0.001f)
                 : static_cast<unsigned int>(time);
}

unsigned int GPUTimer::getAverage(bool micro) {
    get(micro);
    const float result = static_cast<float>(timeTotal) / counter + 0.5f;
    return static_cast<unsigned int>(micro ? result * 0.001f : result);
}

// Very simple GPU timer (timer cannot be nested with other timer or
// GL_TIME_ELAPSED query

void GPURangeTimer::start(bool sync) {
    if (query == 0) glCreateQueries(GL_TIME_ELAPSED, 1, &query);
    time = 0;
    if (sync) glFinish();
    glBeginQuery(GL_TIME_ELAPSED, query);
}

void GPURangeTimer::stop(bool sync) const {
    if (sync) glFinish();
    if ((time == 0) && (query > 0)) {
        glEndQuery(GL_TIME_ELAPSED);
    }
}

unsigned int GPURangeTimer::get(
  bool micro) { // micro - show in microsec, otherwise in nanosec
    // Unbind query buffer if bound
    QueryBufferRelease queryBufferRelease;

    if ((time == 0) && (query > 0)) {
        glGetQueryObjectui64v(query, GL_QUERY_RESULT, &time);
        timeTotal += time;
        counter++;
    }
    return micro ? static_cast<unsigned int>(time * 0.001f)
                 : static_cast<unsigned int>(time);
}

unsigned int GPURangeTimer::getAverage(
  bool micro) { // micro - show in microsec, otherwise in nanosec
    get(micro);
    const float result = static_cast<float>(timeTotal) / counter + 0.5f;
    return static_cast<unsigned int>(micro ? result * 0.001f : result);
}

void CPUTimer::start() {
    time = 0;
    timeStart = Tools::GetCPUTime(false);
}

void CPUTimer::stop() {
    if (time == 0) {
        timeStop = Tools::GetCPUTime(false);
    }
}

unsigned int CPUTimer::get(bool micro) {
    if (time == 0) {
        time = timeStop - timeStart;
        timeTotal += time;
        counter++;
    }
    return micro ? static_cast<unsigned int>(time * 0.001f)
                 : static_cast<unsigned int>(time);
}

unsigned int CPUTimer::getAverage(bool micro) {
    get(micro);
    const float result = static_cast<float>(timeTotal) / counter + 0.5f;
    return static_cast<unsigned int>(micro ? result * 0.001f : result);
}

void CPUTimer::reset() {
    time = 0;
    timeTotal = 0;
    counter = 0;
}

//-----------------------------------------------------------------------------
// Name: SaveFramebuffer()
// Desc:
//-----------------------------------------------------------------------------
void SaveFramebuffer(GLuint framebufferId, GLsizei width, GLsizei height) {
    if ((width <= 0) || (height <= 0)) {
        GLint viewport[4] = {0};
        glGetIntegerv(GL_VIEWPORT, viewport);
        width = viewport[2];
        height = viewport[3];
    }

    const GLsizei numPixels = width * height;
    std::vector<unsigned char> pixels(numPixels * 4);

    GLint fboId = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &fboId);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferId);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
    char file_name[50] = {0};
    sprintf(file_name, "framebuffer_%d_%dx%d_frame_%d.png", framebufferId,
            width, height, Statistic::Frame::ID);

    if (stbi_write_png(file_name, width, height, 4, &pixels[0], 4 * width)) {
        spdlog::info("Screen saved to {}", file_name);
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboId);
}

//-----------------------------------------------------------------------------
// Name: GetFilePath()
// Desc:
//-----------------------------------------------------------------------------
std::string GetFilePath(const char* fileName) {
    std::string absolute_path = fileName;
#ifdef PROJECT_DIRECTORY
    if (fileName) {
        std::ifstream infile(fileName);
        if (!infile.good()) {
            absolute_path = std::string(PROJECT_DIRECTORY) + fileName;
        }
    }
#endif
    return absolute_path;
}

//-----------------------------------------------------------------------------
// Name: OpenFile()
// Desc: TODO: fstream
//-----------------------------------------------------------------------------
FILE* OpenFile(const char* file_name) {
    if (!file_name) return nullptr;

    FILE* fin = fopen(file_name, "rb");

#ifdef PROJECT_DIRECTORY
    if (!fin) {
        std::string absolute_path = std::string(PROJECT_DIRECTORY) + file_name;
        fin = fopen(absolute_path.c_str(), "rb");
    }
#endif
    return fin;
}

//-----------------------------------------------------------------------------
// Name: ReadFile()
// Desc: TODO: use 'fstream'
//-----------------------------------------------------------------------------
bool ReadFile(std::vector<char>& buffer, const char* file_name) {
    // Read input file
    FILE* fin = OpenFile(file_name);
    if (fin) {
        fseek(fin, 0, SEEK_END);
        long file_size = ftell(fin);
        rewind(fin);

        if (file_size > 0) {
            buffer.resize(file_size);
            size_t count = fread(&buffer[0], sizeof(char), file_size, fin);
        }
        fclose(fin);
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Name: WriteFile()
// Desc: TODO: use 'fstream'
//-----------------------------------------------------------------------------
bool WriteFile(void* data, size_t size, const char* fileName) {
    FILE* fout = fopen(Tools::GetFilePath(fileName).c_str(), "wb");
    if (fout) {
        const size_t written = fwrite(data, size, 1, fout);
        fclose(fout);
        return written == size;
    }
    return false;
}

namespace Mesh {
    //-----------------------------------------------------------------------------
    // Name: CreatePlane()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreatePlane(float size, int slice_x, int slice_y,
                     std::vector<glm::vec3>& vertices) {
        const int NUM_VERTICES = 4 * slice_x * slice_y;
        if (NUM_VERTICES < 4) return false;

        // Compute position deltas for moving down the X, and Z axis during mesh
        // creation
        const glm::vec3 delta = glm::vec3(size / slice_x, size / slice_y, 0.0f);
        const glm::vec3 start = glm::vec3(-size * 0.5f, -size * 0.5f, 0.0f);
        vertices.reserve(NUM_VERTICES);
        for (int x = 0; x < slice_x; x++) {
            for (int y = 0; y < slice_y; y++) {
                vertices.emplace_back(start + delta * glm::vec3(x, y, 0.0f));
                vertices.emplace_back(start
                                      + delta * glm::vec3(x + 1, y, 0.0f));
                vertices.emplace_back(start
                                      + delta * glm::vec3(x + 1, y + 1, 0.0f));
                vertices.emplace_back(start
                                      + delta * glm::vec3(x, y + 1, 0.0f));
            }
        }
        return true;
    }

    //-----------------------------------------------------------------------------
    // Name: CreatePlane()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreatePlane(unsigned int vertex_density, unsigned int index_density,
                     GLenum mode, std::vector<glm::vec2>& vertices,
                     std::vector<GLuint>& indices) {
        if ((vertex_density < 2)
            || ((index_density < 2) && (index_density != 0))) {
            return false;
        }
        if ((mode != GL_TRIANGLES) && (mode != GL_TRIANGLE_STRIP)
            && (mode != GL_QUADS)) {
            return false;
        }

        // Generate mesh vertices
        const int total_vertices = vertex_density * vertex_density;
        const GLfloat offset = 2.0f / (vertex_density - 1.0f);

        vertices.clear();
        vertices.reserve(total_vertices);
        for (unsigned int y = 0; y < vertex_density - 1; y++) {
            for (unsigned int x = 0; x < vertex_density - 1; x++) {
                vertices.push_back(
                  glm::vec2(-1.0f + x * offset, -1.0f + y * offset));
            }
            vertices.push_back(glm::vec2(1.0f, -1.0f + y * offset));
        }
        for (unsigned int x = 0; x < vertex_density - 1; x++) {
            vertices.push_back(glm::vec2(-1.0f + x * offset, 1.0f));
        }
        vertices.push_back(glm::vec2(1.0f, 1.0f));

        // Generate mesh indices
        indices.clear();
        if (index_density == 0) {
            const unsigned int total_indices
              = (mode == GL_TRIANGLES)
                  ? 6 * (vertex_density - 1) * (vertex_density - 1)
                  : 2 * (vertex_density * vertex_density - 2);
            indices.reserve(total_indices);

            GLuint index = 0;
            if (mode == GL_TRIANGLES) {
                for (unsigned int y = 0; y < vertex_density - 1; y++) {
                    for (unsigned int x = 0; x < vertex_density - 1; x++) {
                        indices.push_back(index);
                        indices.push_back(index + 1);
                        indices.push_back(index + vertex_density);
                        indices.push_back(index + vertex_density);
                        indices.push_back(index + 1);
                        indices.push_back(index + 1 + vertex_density);
                        index++;
                    }
                    index++;
                }
            } else {
                for (unsigned int y = 0; y < vertex_density - 1; y++) {
                    for (unsigned int x = 0; x < vertex_density; x++) {
                        indices.push_back(index);
                        indices.push_back(index + vertex_density);
                        index++;
                    }
                    if ((vertex_density > 2) && (y != vertex_density - 2)) {
                        indices.push_back(index + vertex_density - 1);
                        indices.push_back(index);
                    }
                }
            }
        } else { // (index_density == 0)
            const unsigned int total_indices
              = (mode == GL_TRIANGLES)
                  ? 6 * (index_density - 1) * (index_density - 1)
                  : ((mode == GL_TRIANGLE_STRIP)
                       ? 2 * (index_density * index_density - 2)
                       : 4 * (index_density - 1) * (index_density - 1));
            const float h_index_offset
              = static_cast<float>(vertex_density - 1) / (index_density - 1);
            indices.reserve(total_indices);

            GLuint index = 0;
            switch (mode) {
                case GL_TRIANGLES:
                    for (unsigned int y = 0; y < index_density - 1; y++) {
                        const GLuint vindex1 = glm::clamp(
                          static_cast<unsigned int>(
                            glm::round(y * h_index_offset) * vertex_density),
                          0u, vertex_density * (vertex_density - 1));
                        const GLuint vindex2 = glm::clamp(
                          static_cast<unsigned int>(
                            glm::round((y + 1) * h_index_offset)
                            * vertex_density),
                          0u, vertex_density * (vertex_density - 1));
                        for (unsigned int x = 0; x < index_density - 1; x++) {
                            const GLuint hindex1
                              = glm::clamp(static_cast<unsigned int>(
                                             x * h_index_offset + 0.5f),
                                           0u, vertex_density - 1);
                            const GLuint hindex2
                              = glm::clamp(static_cast<unsigned int>(
                                             (x + 1) * h_index_offset + 0.5f),
                                           0u, vertex_density - 1);
                            indices.push_back(vindex1 + hindex1); // 0
                            indices.push_back(vindex1 + hindex2); // 1
                            indices.push_back(vindex2 + hindex1); // 2
                            indices.push_back(vindex2 + hindex1); // 3
                            indices.push_back(vindex1 + hindex2); // 4
                            indices.push_back(vindex2 + hindex2); // 5
                        }
                    }
                    break;
                case GL_TRIANGLE_STRIP:
                    for (unsigned int y = 0; y < index_density - 1; y++) {
                        const GLuint vindex1 = glm::clamp(
                          static_cast<unsigned int>(
                            glm::round(y * h_index_offset) * vertex_density),
                          0u, vertex_density * (vertex_density - 1));
                        const GLuint vindex2 = glm::clamp(
                          static_cast<unsigned int>(
                            glm::round((y + 1) * h_index_offset)
                            * vertex_density),
                          0u, vertex_density * (vertex_density - 1));
                        const size_t startIdx = indices.size() + 1;
                        const size_t endIdx
                          = indices.size() + 2 * index_density - 1;
                        for (unsigned int x = 0; x < index_density; x++) {
                            const GLuint hindex
                              = glm::clamp(static_cast<unsigned int>(
                                             x * h_index_offset + 0.5f),
                                           0u, vertex_density - 1);
                            indices.push_back(hindex + vindex1);
                            indices.push_back(hindex + vindex2);
                        }
                        if (y < index_density - 2) {
                            // add 2 degenerated triangles
                            indices.push_back(indices[endIdx]);
                            indices.push_back(indices[startIdx]);
                        }
                    }
                    break;
                case GL_QUADS:
                    for (unsigned int y = 0; y < index_density - 1; y++) {
                        const GLuint vindex1 = glm::clamp(
                          static_cast<unsigned int>(
                            glm::round(y * h_index_offset) * vertex_density),
                          0u, vertex_density * (vertex_density - 1));
                        const GLuint vindex2 = glm::clamp(
                          static_cast<unsigned int>(
                            glm::round((y + 1) * h_index_offset)
                            * vertex_density),
                          0u, vertex_density * (vertex_density - 1));
                        for (unsigned int x = 0; x < index_density - 1; x++) {
                            const GLuint hindex1
                              = glm::clamp(static_cast<unsigned int>(
                                             x * h_index_offset + 0.5f),
                                           0u, vertex_density - 1);
                            const GLuint hindex2
                              = glm::clamp(static_cast<unsigned int>(
                                             (x + 1) * h_index_offset + 0.5f),
                                           0u, vertex_density - 1);
                            indices.push_back(hindex1 + vindex1);
                            indices.push_back(hindex1 + vindex2);
                            indices.push_back(hindex2 + vindex2);
                            indices.push_back(hindex2 + vindex1);
                        }
                    }
                    break;
                default:
                    assert(0);
                    break;
            }
        }

        return true;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateCircle()
    // Desc:
    //-----------------------------------------------------------------------------
    template<class INDEX_TYPE>
    bool CreateCircle(GLenum primitive_type, unsigned int num_triangles,
                      float z, std::vector<glm::vec3>& vertices,
                      std::vector<INDEX_TYPE>* indices) {
        if (((primitive_type != GL_TRIANGLES)
             && (primitive_type != GL_TRIANGLE_FAN))
            || (num_triangles < 4))
            return false;

        const GLfloat TWO_PI = 2.0f * glm::pi<float>();
        const GLfloat alpha_step = TWO_PI / num_triangles;
        GLfloat alpha = 0.0f;

        if (indices) {
            // Generate mesh vertices
            vertices.reserve(num_triangles + 1);
            vertices.push_back(glm::vec3());
            for (unsigned int iTriangle = 0; iTriangle < num_triangles;
                 iTriangle++) {
                vertices.push_back(glm::vec3(cos(alpha), sin(alpha), z));
                alpha = glm::min(TWO_PI, alpha + alpha_step);
            }

            // Generate mesh indices
            if (primitive_type == GL_TRIANGLES) {
                indices->reserve(num_triangles * 3);
                for (unsigned int iTriangle = 0; iTriangle < num_triangles - 1;
                     iTriangle++) {
                    indices->push_back(static_cast<INDEX_TYPE>(0));
                    indices->push_back(static_cast<INDEX_TYPE>(iTriangle + 1));
                    indices->push_back(static_cast<INDEX_TYPE>(iTriangle + 2));
                }
                indices->push_back(static_cast<INDEX_TYPE>(0));
                indices->push_back(static_cast<INDEX_TYPE>(num_triangles));
                indices->push_back(static_cast<INDEX_TYPE>(1));
            } else {
                indices->reserve(vertices.size() + 1);
                for (unsigned int iVertex = 0; iVertex < vertices.size();
                     iVertex++) {
                    indices->push_back(static_cast<INDEX_TYPE>(iVertex));
                }
                indices->push_back(static_cast<INDEX_TYPE>(1));
            }
        } else {
            // Generate sequence geometry mesh
            vertices.reserve((primitive_type == GL_TRIANGLES)
                               ? (num_triangles * 3)
                               : (num_triangles + 2));
            for (unsigned int iTriangle = 0; iTriangle < num_triangles;
                 iTriangle++) {
                // 1st vertex of triangle
                if ((primitive_type == GL_TRIANGLES) || (iTriangle == 0))
                    vertices.push_back(glm::vec3());

                // 2nd vertex of triangle
                vertices.push_back(glm::vec3(cos(alpha), sin(alpha), z));
                // 3rd vertex of triangle
                alpha = glm::min(TWO_PI, alpha + alpha_step);
                if (iTriangle == num_triangles - 1)
                    vertices.push_back(glm::vec3(1.0f, 0.0f, z));
                else if (primitive_type == GL_TRIANGLES)
                    vertices.push_back(glm::vec3(cos(alpha), sin(alpha), z));
            }
        }

        return true;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateSphereVertexMesh()
    // Desc: Sphere for OpenGL with (radius, sectors, stacks)
    //       The min number of sectors is 3 and the min number of stacks are 2.
    // Auth: Song Ho Ahn (song.ahn@gmail.com)
    //-----------------------------------------------------------------------------
    bool CreateSphereVertexMesh(std::vector<glm::vec3>& vertices,
                                std::vector<GLushort>& indices, float radius,
                                int sectorCount, int stackCount) {
        if ((sectorCount < 3) || (stackCount < 2)) return false;

        glm::vec3 vertex;
        float sectorStep = 2.0f * glm::pi<float>() / sectorCount;
        float stackStep = glm::pi<float>() / stackCount;

        for (int i = 0; i <= stackCount; ++i) {
            const float stackAngle
              = glm::pi<float>() * 0.5f
                - i * stackStep; // starting from pi/2 to -pi/2
            const float r = radius * glm::cos(stackAngle); // r * cos(u)
            const float z = radius * sinf(stackAngle);     // r * sin(u)

            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same position and normal, but
            // different tex coords
            for (int j = 0; j <= sectorCount; ++j) {
                const float sectorAngle
                  = j * sectorStep; // starting from 0 to 2pi
                vertices.emplace_back(r * cosf(sectorAngle),
                                      r * sinf(sectorAngle), z);
                // normal is  : glm::normalize(vertices.back())
                // texCoord is: glm::vec2(j / sectorCount, i / stackCount);
            }
        }

        // indices
        //  k1--k1+1
        //  |  / |
        //  | /  |
        //  k2--k2+1
        unsigned int k1, k2;
        for (int i = 0; i < stackCount; ++i) {
            k1 = i * (sectorCount + 1); // beginning of current stack
            k2 = k1 + sectorCount + 1;  // beginning of next stack

            for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
                // 2 triangles per sector excluding 1st and last stacks
                if (i != 0) {
                    indices.emplace_back(k1);
                    indices.emplace_back(k2);
                    indices.emplace_back(k1 + 1); // k1---k2---k1+1
                }
                if (i != (stackCount - 1)) {
                    indices.emplace_back(k1 + 1);
                    indices.emplace_back(k2);
                    indices.emplace_back(k2 + 1); // k1+1---k2---k2+1
                }
            }
        }

        return true;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateSphereVertexMesh()
    // Desc: Sphere for OpenGL with (radius, sectors, stacks)
    //       The min number of sectors is 3 and the min number of stacks are 2.
    // Auth: Song Ho Ahn (song.ahn@gmail.com)
    //-----------------------------------------------------------------------------
    bool CreateSphereVertexMesh(std::vector<glm::vec3>& vertices, float radius,
                                int sectorCount, int stackCount) {
        if ((sectorCount < 3) || (stackCount < 2)) return false;

        std::vector<glm::vec3> unique_vertices;
        glm::vec3 vertex;
        float sectorStep = 2.0f * glm::pi<float>() / sectorCount;
        float stackStep = glm::pi<float>() / stackCount;

        for (int i = 0; i <= stackCount; ++i) {
            const float stackAngle
              = glm::pi<float>() * 0.5f
                - i * stackStep; // starting from pi/2 to -pi/2
            const float r = radius * glm::cos(stackAngle); // r * cos(u)
            const float z = radius * sinf(stackAngle);     // r * sin(u)

            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same position and normal, but
            // different tex coords
            for (int j = 0; j <= sectorCount; ++j) {
                const float sectorAngle
                  = j * sectorStep; // starting from 0 to 2pi
                unique_vertices.emplace_back(r * cosf(sectorAngle),
                                             r * sinf(sectorAngle), z);
                // normal is  : glm::normalize(vertices.back())
                // texCoord is: glm::vec2(j / sectorCount, i / stackCount);
            }
        }

        // indices
        //  k1--k1+1
        //  |  / |
        //  | /  |
        //  k2--k2+1
        unsigned int k1, k2;
        for (int i = 0; i < stackCount; ++i) {
            k1 = i * (sectorCount + 1); // beginning of current stack
            k2 = k1 + sectorCount + 1;  // beginning of next stack

            for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
                // 2 triangles per sector excluding 1st and last stacks
                if (i != 0) {
                    // k1---k1+1---k2
                    vertices.emplace_back(unique_vertices[k1]);
                    vertices.emplace_back(unique_vertices[k1 + 1]);
                    vertices.emplace_back(unique_vertices[k2]);
                }
                if (i != (stackCount - 1)) {
                    // k1+1---k2+1---k2
                    vertices.emplace_back(unique_vertices[k1 + 1]);
                    vertices.emplace_back(unique_vertices[k2 + 1]);
                    vertices.emplace_back(unique_vertices[k2]);
                }
            }
        }

        return true;
    }
} // end of namespace Mesh

namespace Shader {
    //-----------------------------------------------------------------------------
    // Name: SaveBinaryCode()
    // Desc:
    //-----------------------------------------------------------------------------
    bool SaveBinaryCode(GLuint programId, const char* fileName) {
        if (!Variables::ShaderBinaryOutput) {
            spdlog::error("Shader binary output is not supported!");
            return false;
        }

        GLint binaryLength = 0;
        glGetProgramiv(programId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
        if (binaryLength < 1) {
            spdlog::error("Unable to get shader binary code!");
        }

        GLenum binaryFormat = GL_NONE;
        void* binary = malloc(binaryLength);
        glGetProgramBinary(programId, binaryLength, nullptr, &binaryFormat,
                           binary);
        const bool result = Tools::WriteFile(binary, binaryLength, fileName);
        free(binary);

        return result;
    }

    //-----------------------------------------------------------------------------
    // Name: CheckShaderInfoLog()
    // Desc:
    //-----------------------------------------------------------------------------
    void CheckShaderInfoLog(GLuint shader_id) {
        if (shader_id == 0) return;

        int log_length = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

        if (log_length > 0) {
            std::vector<char> buffer(log_length);
            int written = 0;
            glGetShaderInfoLog(shader_id, log_length, &written, &buffer[0]);
            spdlog::info(
              "Shader info log: {}",
              std::string_view{buffer.data(), static_cast<size_t>(written)});
        }
    }

    //-----------------------------------------------------------------------------
    // Name: CheckProgramInfoLog()
    // Desc:
    //-----------------------------------------------------------------------------
    void CheckProgramInfoLog(GLuint program_id) {
        if (program_id == 0) return;

        int log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        if (log_length > 0) {
            std::vector<char> buffer(log_length);
            int written = 0;
            glGetProgramInfoLog(program_id, log_length, &written, &buffer[0]);
            spdlog::info(
              "Program info log: {}",
              std::string_view{buffer.data(), static_cast<size_t>(written)});
        }
    }

    //-----------------------------------------------------------------------------
    // Name: CheckShaderCompileStatus()
    // Desc:
    //-----------------------------------------------------------------------------
    GLboolean CheckShaderCompileStatus(GLuint shader_id) {
        GLint status = GL_FALSE.m_value;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
        return static_cast<GLboolean>(status);
    }

    //-----------------------------------------------------------------------------
    // Name: CheckProgramLinkStatus()
    // Desc:
    //-----------------------------------------------------------------------------
    GLint CheckProgramLinkStatus(GLuint program_id) {
        GLint status = GL_FALSE.m_value;
        glGetProgramiv(program_id, GL_LINK_STATUS, &status);
        return status;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateShaderFromSource()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateShaderFromSource(GLenum shader_type, const char* source,
                                  const char* shader_name) {
        if (!source) return 0;

        auto shader_type_str = [](GLenum shader_type) {
            switch (shader_type) {
                case GL_VERTEX_SHADER:
                    return "Vertex";
                case GL_FRAGMENT_SHADER:
                    return "Fragment";
                case GL_TESS_CONTROL_SHADER:
                    return "Tesselation Control";
                case GL_TESS_EVALUATION_SHADER:
                    return "Tesselation Evaluation";
                case GL_GEOMETRY_SHADER:
                    return "Geometry";
                case GL_COMPUTE_SHADER:
                    return "Compute";
                case GL_MESH_SHADER_NV:
                    return "Mesh";
                case GL_TASK_SHADER_NV:
                    return "Task";
                default:
                    return "(unknown shader type)";
            }
        };

        GLuint shader_id = glCreateShader(shader_type);
        if (shader_id == 0) return 0;

        glShaderSource(shader_id, 1, &source, nullptr);
        glCompileShader(shader_id);
        if (CheckShaderCompileStatus(shader_id) != GL_TRUE) {
            spdlog::error("{} shader compilation failed ({})",
                          shader_type_str(shader_type),
                          shader_name ? shader_name : "shader name unknown");
            CheckShaderInfoLog(shader_id);
            glDeleteShader(shader_id);
            return 0;
        }
        spdlog::debug("{} shader compiled.{}", shader_type_str(shader_type),
                      shader_name ? fmt::format(" ({})", shader_name) : "");
        return shader_id;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateShaderFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateShaderFromFile(GLenum shader_type, const char* file_name,
                                const char* preprocessor) {
        std::vector<char> fileContent;
        if (!Tools::ReadFile(fileContent, file_name)) {
            spdlog::error("Shader creation failed, input file ({}) is "
                          "empty or missing!",
                          file_name);
            return 0;
        }
        fileContent.emplace_back('\0'); // null terminated

        std::string shader_header;
        if (preprocessor) {
            shader_header += preprocessor;
        }
        if (Variables::Shader::UserTest) {
            shader_header += "\n#define USER_TEST\n";
        }

        std::string shader_source = &fileContent[0];
        if (!shader_header.empty()) {
            std::size_t insertIdx
              = shader_source.find("\n", shader_source.find("#version"));
            shader_source.insert((insertIdx != std::string::npos) ? insertIdx
                                                                  : 0,
                                 std::string("\n") + shader_header + "\n\n");
        }

        return CreateShaderFromSource(shader_type, shader_source.c_str(),
                                      file_name);
    }

    //-----------------------------------------------------------------------------
    // Name: _updateProgramList()
    // Desc:
    //-----------------------------------------------------------------------------
    void _updateProgramList(GLuint oldProgram, GLuint newProgram) {
        // Remove program from OpenGL and internal list
        for (auto it = OpenGL::programs.begin(); it != OpenGL::programs.end();
             ++it) {
            if (it->id == oldProgram) {
                OpenGL::programs.erase(it);
                break;
            }
        }

        // Save program ID if it contains any user variables
        if ((glGetUniformLocation(newProgram, "u_MVPMatrix") > -1)
            || (glGetUniformLocation(newProgram, "u_ModelMatrix") > -1)
            || (glGetUniformLocation(newProgram, "u_ViewMatrix") > -1)
            || (glGetUniformLocation(newProgram, "u_ModelViewMatrix") > -1)
            || (glGetUniformLocation(newProgram, "u_ModelViewMatrixInverse")
                > -1)
            || (glGetUniformLocation(newProgram, "u_ProjectionMatrix") > -1)
            || (glGetUniformLocation(newProgram, "u_NormalMatrix") > -1)
            || (glGetUniformLocation(newProgram, "u_Viewport") > -1)
            || (glGetUniformLocation(newProgram, "u_ZOffset") > -1)
            || (glGetUniformLocation(newProgram, "u_VariableInt") > -1)
            || (glGetUniformLocation(newProgram, "u_VariableFloat") > -1)
            || (glGetUniformLocation(newProgram, "u_FrameCounter") > -1)) {
            OpenGL::programs.push_back(newProgram);
        }
    }

    //-----------------------------------------------------------------------------
    // Name: CreateShaderProgram()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateShaderProgram(GLuint& programId, GLint count,
                             const GLuint* shader_ids) {
        if (!shader_ids) return false;

        // Create shader program object
        GLuint pr_id = glCreateProgram();
        for (int i = 0; i < count; i++) {
            glAttachShader(pr_id, shader_ids[i]);
            glDeleteShader(shader_ids[i]);
        }
        if (Variables::ShaderBinaryOutput) {
            glProgramParameteri(pr_id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT,
                                GL_TRUE);
        }
        glLinkProgram(pr_id);
        if (!CheckProgramLinkStatus(pr_id)) {
            CheckProgramInfoLog(pr_id);
            spdlog::error("Program linking failed. See previous log messages.");
            glDeleteProgram(pr_id);
            return false;
        }

        // Remove program from OpenGL and update internal list
        glDeleteProgram(programId);
        _updateProgramList(programId, pr_id);
        programId = pr_id;

        return true;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateShaderProgramFromSource()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateShaderProgramFromSource(GLuint& programId, GLint count,
                                       const GLenum* shader_types,
                                       const char* const* source) {
        if (!shader_types || !source) return false;

        // Create shader program object
        GLuint pr_id = glCreateProgram();
        for (int i = 0; i < count; i++) {
            GLuint shader_id
              = CreateShaderFromSource(shader_types[i], source[i]);
            if (shader_id == 0) {
                glDeleteProgram(pr_id);
                return false;
            }
            glAttachShader(pr_id, shader_id);
            glDeleteShader(shader_id);
        }
        if (Variables::ShaderBinaryOutput) {
            glProgramParameteri(pr_id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT,
                                GL_TRUE);
        }
        glLinkProgram(pr_id);
        if (!CheckProgramLinkStatus(pr_id)) {
            CheckProgramInfoLog(pr_id);
            spdlog::error("Program linking failed. See previous log messages.");
            glDeleteProgram(pr_id);
            return false;
        }

        // Remove program from OpenGL and update internal list
        glDeleteProgram(programId);
        _updateProgramList(programId, pr_id);
        programId = pr_id;

        return true;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateShaderProgramFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateShaderProgramFromFile(GLuint& programId, const char* vs,
                                     const char* tc, const char* te,
                                     const char* gs, const char* fs,
                                     const char* preprocessor,
                                     const std::vector<char*>* tbx) {
        GLenum shader_types[5] = {
          static_cast<GLenum>(vs ? GL_VERTEX_SHADER : GL_NONE),
          static_cast<GLenum>(tc ? GL_TESS_CONTROL_SHADER : GL_NONE),
          static_cast<GLenum>(te ? GL_TESS_EVALUATION_SHADER : GL_NONE),
          static_cast<GLenum>(gs ? GL_GEOMETRY_SHADER : GL_NONE),
          static_cast<GLenum>(fs ? GL_FRAGMENT_SHADER : GL_NONE),
        };
        const char* source_file_names[5] = {vs, tc, te, gs, fs};

        // Create shader program object
        GLuint pr_id = glCreateProgram();
        for (int i = 0; i < 5; i++) {
            if (source_file_names[i]) {
                GLuint shader_id = CreateShaderFromFile(
                  shader_types[i], source_file_names[i], preprocessor);
                if (shader_id == 0) {
                    glDeleteProgram(pr_id);
                    return false;
                }
                glAttachShader(pr_id, shader_id);
                glDeleteShader(shader_id);
            }
        }
        if (tbx && !tbx->empty()) {
            glTransformFeedbackVaryings(pr_id,
                                        static_cast<GLsizei>(tbx->size()),
                                        &(*tbx)[0], GL_INTERLEAVED_ATTRIBS);
        }
        if (Variables::ShaderBinaryOutput) {
            glProgramParameteri(pr_id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT,
                                GL_TRUE);
        }
        glLinkProgram(pr_id);
        if (!CheckProgramLinkStatus(pr_id)) {
            CheckProgramInfoLog(pr_id);
            spdlog::error("Program linking failed. See previous log messages.");

            glDeleteProgram(pr_id);
            return false;
        }

        // Remove program from OpenGL and update internal list
        glDeleteProgram(programId);
        _updateProgramList(programId, pr_id);
        programId = pr_id;

        return true;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateMeshShaderProgramFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateMeshShaderProgramFromFile(GLuint& programId, const char* mesh,
                                         const char* task, const char* fragment,
                                         const char* preprocessor) {
        GLenum shader_types[3]
          = {static_cast<GLenum>(mesh ? GL_MESH_SHADER_NV : GL_NONE),
             static_cast<GLenum>(task ? GL_TASK_SHADER_NV : GL_NONE),
             static_cast<GLenum>(fragment ? GL_FRAGMENT_SHADER : GL_NONE)};
        const char* source_file_names[3] = {mesh, task, fragment};

        // Create shader program object
        GLuint pr_id = glCreateProgram();
        for (int i = 0; i < 3; i++) {
            if (source_file_names[i]) {
                GLuint shader_id = CreateShaderFromFile(
                  shader_types[i], source_file_names[i], preprocessor);
                if (shader_id == 0) {
                    glDeleteProgram(pr_id);
                    return false;
                }
                glAttachShader(pr_id, shader_id);
                glDeleteShader(shader_id);
            }
        }
        if (Variables::ShaderBinaryOutput) {
            glProgramParameteri(pr_id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT,
                                GL_TRUE);
        }
        glLinkProgram(pr_id);
        if (!CheckProgramLinkStatus(pr_id)) {
            CheckProgramInfoLog(pr_id);
            spdlog::error("Program linking failed. See previous log messages.");

            glDeleteProgram(pr_id);
            return false;
        }

        // Remove program from OpenGL and update internal list
        glDeleteProgram(programId);
        _updateProgramList(programId, pr_id);
        programId = pr_id;
        return true;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateComputeShaderProgramFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateComputeShaderProgramFromFile(GLuint& programId, const char* cs,
                                            const char* preprocessor) {
        if (cs == nullptr) return false;

        // Create shader program object
        GLuint pr_id = glCreateProgram();
        GLuint shader_id
          = CreateShaderFromFile(GL_COMPUTE_SHADER, cs, preprocessor);
        if (shader_id == 0) {
            glDeleteProgram(pr_id);
            return false;
        }
        glAttachShader(pr_id, shader_id);
        glDeleteShader(shader_id);
        if (Variables::ShaderBinaryOutput) {
            glProgramParameteri(pr_id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT,
                                GL_TRUE);
        }
        glLinkProgram(pr_id);
        if (!CheckProgramLinkStatus(pr_id)) {
            CheckProgramInfoLog(pr_id);
            spdlog::error("Program linking failed. See previous log messages.");

            glDeleteProgram(pr_id);
            return false;
        }

        // Remove program from OpenGL and update internal list
        glDeleteProgram(programId);
        _updateProgramList(programId, pr_id);
        programId = pr_id;

        return true;
    }
} // end of namespace Shader

namespace Texture {
    //-----------------------------------------------------------------------------
    // Name: Show2D()
    // Desc:
    //-----------------------------------------------------------------------------
    void Show2D(GLuint texID, int x, int y, int width, int height, float scale,
                int lod, bool border) {
        constexpr GLenum SHADER_TYPES[]
          = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
        // Vertex shader (same for all texture types)
        constexpr const auto* const vs
          = "#version 450 core\nuniform vec4 Vertex[4];\nout vec2 "
            "v_TexCoord;\nvoid main(void) {\nv_TexCoord  = "
            "Vertex[gl_VertexID].zw;\ngl_Position = "
            "vec4(Vertex[gl_VertexID].xy, 0.0, 1.0f);\n}";
        // Fragment shaders, following formats are supported GL_RGBA8,
        // GL_RGBA8MS, GL_R32UI
        constexpr const auto* const fsRGBA8
          = "#version 400 core\nlayout (location = 0) out vec4 FragColor;\nin "
            "vec2 v_TexCoord;\nuniform sampler2D u_Texture;\nuniform int u_Lod "
            "= 0;\nuniform float u_Scale = 1.0;\nvoid main(void) {\nFragColor "
            "= vec4(textureLod(u_Texture, v_TexCoord, u_Lod).rgb * u_Scale, "
            "1.0);\n}";
        constexpr const auto* const fsRGBA8MS
          = "#version 400 core\nlayout (location = 0) out vec4 FragColor;\nin "
            "vec2 v_TexCoord;\nuniform sampler2DMS u_Texture;\nuniform float   "
            "    u_Scale = 1.0;\nvoid main(void) {\nvec3 color = "
            "vec3(0.0);\nint numSamples = textureSamples(u_Texture);\nivec2 "
            "texCoords = ivec2(v_TexCoord * textureSize(u_Texture));\nfor (int "
            "i = 0; i < numSamples; i++)\ncolor += texelFetch(u_Texture, "
            "texCoords, i).rgb * u_Scale;\nFragColor = "
            "vec4(color/u_NumSamples, 1.0);\n}";
        constexpr const auto* const fsR32UI
          = "#version 400 core\nlayout (location = 0) out vec4 FragColor;\nin "
            "vec2 v_TexCoord;\nuniform usampler2D u_Texture;\nuniform int "
            "u_Lod = 0;\nuniform float u_Scale = 1.0;\nvoid main(void) {\nuint "
            "color = textureLod(u_Texture, v_TexCoord, u_Lod).r;\nFragColor = "
            "vec4(float(color) * u_Scale, 0.0, 0.0, 1.0);\n}";
        static GLuint programs[3] = {0};
        static GLuint vertexArrayId = 0;

        if (vertexArrayId == 0) {
            glCreateVertexArrays(1, &vertexArrayId);
        }

        if (glIsTexture(texID) == GL_FALSE) {
            return;
        }

        // Prepare vertices
        glm::vec4 vp;
        glGetFloatv(GL_VIEWPORT, &vp.x);
        glm::vec2 tcOffse
          = border ? glm::vec2(1.0f / width, 1.0f / height) : glm::vec2(0.0f);
        const GLfloat vertices[] = {
          (x - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          0.0f - tcOffse.x,
          0.0f - tcOffse.y,
          (x + width - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          1.0f + tcOffse.x,
          0.0f - tcOffse.y,
          (x + width - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y + height - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          1.0f + tcOffse.x,
          1.0f + tcOffse.y,
          (x - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y + height - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          0.0f - tcOffse.x,
          1.0f + tcOffse.y,
        };

        // Get texture properties
        GLint numSamples = 0;
        GLint format = 0;
        GLint compMode = 0;
        glGetTextureLevelParameteriv(texID, 0, GL_TEXTURE_SAMPLES, &numSamples);
        glGetTextureLevelParameteriv(texID, 0, GL_TEXTURE_INTERNAL_FORMAT,
                                     &format);
        glGetTextureParameteriv(texID, GL_TEXTURE_COMPARE_MODE, &compMode);
        GLenum target
          = (numSamples == 0) ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE;
        if (compMode != GL_NONE) {
            glTextureParameteri(texID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }

        // Select shader
        const char* sources[2] = {vs, fsR32UI}; // GL_R32UI is default choice
        int programIdx = 0;
        if (format != GL_R32UI) {
            sources[1] = (target == GL_TEXTURE_2D) ? fsRGBA8 : fsRGBA8MS;
            programIdx = (target == GL_TEXTURE_2D) ? 1 : 2;
        };

        // Compile shaders
        if ((programs[programIdx] == 0)
            && !Shader::CreateShaderProgramFromSource(programs[programIdx], 2,
                                                      SHADER_TYPES, sources)) {
            spdlog::error("Texture::Show2D: Unable to compile shader program.");
            return;
        }

        // Setup texture
        GLint origTexID = 0;
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &origTexID);
        glBindTexture(target, texID);

        // Render textured screen quad
        GLint curVertexArrayId = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &curVertexArrayId);
        GLint curProgramId = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &curProgramId);
        const GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
        if (depthTest) {
            glDisable(GL_DEPTH_TEST);
        }
        glUseProgram(programs[programIdx]);
        glUniform1i(glGetUniformLocation(programs[programIdx], "u_Texture"), 0);
        glUniform1i(glGetUniformLocation(programs[programIdx], "u_NumSamples"),
                    numSamples);
        glUniform1i(glGetUniformLocation(programs[programIdx], "u_Lod"), lod);
        glUniform1f(glGetUniformLocation(programs[programIdx], "u_Scale"),
                    scale);
        glUniform4fv(glGetUniformLocation(programs[programIdx], "Vertex"), 4,
                     vertices);
        glBindVertexArray(vertexArrayId);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glUseProgram(curProgramId);
        // Restore original states
        if (depthTest) {
            glEnable(GL_DEPTH_TEST);
        }
        if (compMode != GL_NONE) {
            glTextureParameteri(texID, GL_TEXTURE_COMPARE_MODE, compMode);
        }
        glBindTexture(GL_TEXTURE_2D, origTexID);
        glBindVertexArray(curVertexArrayId);
    }

    //-----------------------------------------------------------------------------
    // Name: Show3D()
    // Desc:
    //-----------------------------------------------------------------------------
    void Show3D(GLuint texID, GLint slice, GLint x, GLint y, GLsizei width,
                GLsizei height, float scale, int lod, bool border) {
        static const GLenum SHADER_TYPES[]
          = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
        // Vertex shader
        constexpr const auto vs
          = "#version 400 core\nuniform vec4 Vertex[4];\nout vec2 "
            "v_TexCoord;\nvoid main(void) {\nv_TexCoord  = "
            "Vertex[gl_VertexID].zw;\ngl_Position = "
            "vec4(Vertex[gl_VertexID].xy, 0.0, 1.0f);\n}";
        // Fragment shader GL_RGBA
        constexpr const auto fsRGBA8
          = "#version 450 core\nlayout (location = 0) out vec4 FragColor;\nin "
            "vec2 v_TexCoord;\nuniform sampler3D u_Texture;\nuniform float "
            "u_SliceZ = 0.0;\nuniform int u_Lod = 0;\nuniform float u_Scale = "
            "1.0;\nvoid main(void) {\nFragColor = vec4(textureLod(u_Texture, "
            "vec3(v_TexCoord, u_SliceZ), u_Lod).rrr * u_Scale, 1.0);\n}";

        if (glIsTexture(texID) == GL_FALSE) {
            return;
        }

        // Prepare vertices
        glm::vec4 vp;
        glGetFloatv(GL_VIEWPORT, &vp.x);
        glm::vec2 tcOffse
          = border ? glm::vec2(1.0f / width, 1.0f / height) : glm::vec2(0.0f);
        const GLfloat vertices[] = {
          (x - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          0.0f - tcOffse.x,
          0.0f - tcOffse.y,
          (x + width - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          1.0f + tcOffse.x,
          0.0f - tcOffse.y,
          (x + width - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y + height - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          1.0f + tcOffse.x,
          1.0f + tcOffse.y,
          (x - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y + height - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          0.0f - tcOffse.x,
          1.0f + tcOffse.y,
        };

        // Select shader
        static GLuint program = 0;
        // Compile shaders
        if (program == 0) {
            const char* sources[2] = {vs, fsRGBA8};
            if (!Shader::CreateShaderProgramFromSource(program, 2, SHADER_TYPES,
                                                       sources)) {
                spdlog::error(
                  "Texture::Show3D: Unable to compile shader program.");
                return;
            }
        }

        // Setup texture
        GLint origTexID = 0;
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_3D, &origTexID);
        glBindTexture(GL_TEXTURE_3D, texID);

        // Render textured screen quad
        GLint curProgramId = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &curProgramId);
        const GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
        if (depthTest) {
            glDisable(GL_DEPTH_TEST);
        }

        glUseProgram(program);
        glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);
        glUniform1f(glGetUniformLocation(program, "u_SliceZ"), slice);
        glUniform1i(glGetUniformLocation(program, "u_Lod"), lod);
        glUniform1f(glGetUniformLocation(program, "u_Scale"), scale);
        glUniform4fv(glGetUniformLocation(program, "Vertex"), 4, vertices);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glUseProgram(curProgramId);

        if (depthTest) {
            glEnable(GL_DEPTH_TEST);
        }
        glBindTexture(GL_TEXTURE_3D, origTexID);
    }

    //-----------------------------------------------------------------------------
    // Name: ShowCube()
    // Desc:
    //-----------------------------------------------------------------------------
    void ShowCube(GLuint texID, GLint x, GLint y, GLsizei width,
                  GLsizei height) {
        static const GLenum SHADER_TYPES[]
          = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
        constexpr std::array sources = {
          // Vertex shader
          "#version 400 core\nuniform vec3 u_Vertices[36];\nuniform vec3 "
          "u_TexCoords[36];\nuniform vec2 u_Offset[7];\nuniform vec4 "
          "u_Scale;\nout vec3 v_TexCoord;\nvoid main(void) {\nvec3 vertex = "
          "u_Vertices[gl_VertexID];\nv_TexCoord  = u_TexCoords[gl_VertexID]; "
          "\ngl_Position = vec4((vertex.xy*u_Scale.xy + "
          "u_Offset[int(vertex.z)]) * u_Scale.zw - 1.0, 0.0, 1.0f);\n}",
          // Fragment shader
          "#version 400 core\nlayout (location = 0) out vec4 "
          "FragColor;\nuniform samplerCube u_Texture;\nuniform vec4 u_Color;\n "
          "in vec3 v_TexCoord; \nvoid main(void) {\nFragColor = "
          "vec4(texture(u_Texture, v_TexCoord).rgb, 1.0) * u_Color;\n}"};
        static GLuint program = 0;

        // Compile shader
        if (program == 0) {
            if (!Shader::CreateShaderProgramFromSource(program, 2, SHADER_TYPES,
                                                       sources.data())) {
                spdlog::error("Texture::ShowCube: Unable to compile shader "
                              "program.");
                return;
            }

            // Setup static uniforms
            const GLfloat vertices[] = {
              0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
              0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // positive x
              0.0f, 0.0f, 2.0f, 1.0f, 0.0f, 2.0f, 1.0f, 1.0f, 2.0f, 0.0f,
              0.0f, 2.0f, 1.0f, 1.0f, 2.0f, 0.0f, 1.0f, 2.0f, // negative x
              0.0f, 0.0f, 3.0f, 1.0f, 0.0f, 3.0f, 1.0f, 1.0f, 3.0f, 0.0f,
              0.0f, 3.0f, 1.0f, 1.0f, 3.0f, 0.0f, 1.0f, 3.0f, // positive y
              0.0f, 0.0f, 4.0f, 1.0f, 0.0f, 4.0f, 1.0f, 1.0f, 4.0f, 0.0f,
              0.0f, 4.0f, 1.0f, 1.0f, 4.0f, 0.0f, 1.0f, 4.0f, // negative y
              0.0f, 0.0f, 5.0f, 1.0f, 0.0f, 5.0f, 1.0f, 1.0f, 5.0f, 0.0f,
              0.0f, 5.0f, 1.0f, 1.0f, 5.0f, 0.0f, 1.0f, 5.0f, // positive z
              0.0f, 0.0f, 6.0f, 1.0f, 0.0f, 6.0f, 1.0f, 1.0f, 6.0f, 0.0f,
              0.0f, 6.0f, 1.0f, 1.0f, 6.0f, 0.0f, 1.0f, 6.0f, // negative z
            };
            const GLfloat texCoords[] = {
              1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
              1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,
              1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f, // positive x
              -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,
              -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,
              -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, // negative x
              1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
              -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  -1.0f,
              -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f, // positive y
              1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
              -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,
              -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, // negative y
              -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
              1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
              1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f, // positive z
              1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
              -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,
              -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, // negative z
            };

            glProgramUniform1i(program,
                               glGetUniformLocation(program, "u_Texture"), 0);
            glProgramUniform4f(program,
                               glGetUniformLocation(program, "u_Color"), 1.0f,
                               1.0f, 1.0f, 1.0f);
            glProgramUniform3fv(program,
                                glGetUniformLocation(program, "u_Vertices"), 36,
                                vertices);
            glProgramUniform3fv(program,
                                glGetUniformLocation(program, "u_TexCoords"),
                                36, texCoords);
        }

        glm::vec4 vp;
        glGetFloatv(GL_VIEWPORT, &vp.x);
        const GLfloat offset[] = {
          static_cast<GLfloat>(x), static_cast<GLfloat>(y),
          static_cast<GLfloat>(x), y + height / 3.0f,        // positive_x
          x + width / 2.0f,        y + height / 3.0f,        // negative_x
          x + width / 4.0f,        y + height * 2.0f / 3.0f, // positive_y
          x + width / 4.0f,        static_cast<GLfloat>(y),  // negative_y
          x + width * 3.0f / 4.0f, y + height / 3.0f,        // positive_z
          x + width / 4.0f,        y + height / 3.0f,        // negative_z
        };

        // Save GL states
        GLint curProgramId = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &curProgramId);
        const GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean blending = glIsEnabled(GL_BLEND);
        if (depthTest) {
            glDisable(GL_DEPTH_TEST);
        }

        // Setup texture
        GLint origTexID = 0;
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &origTexID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

        // Render textured screen quad
        glUseProgram(program);
        glBindTextureUnit(0, texID);
        glUniform2fv(glGetUniformLocation(program, "u_Offset"), 7, offset);
        glUniform4f(glGetUniformLocation(program, "u_Scale"), width / 4.0f,
                    height / 3.0f, 2.0f / (vp.z - vp.x), 2.0f / (vp.w - vp.y));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glUseProgram(curProgramId);
        if (depthTest) {
            glEnable(GL_DEPTH_TEST);
        }
        if (blending) {
            glEnable(GL_BLEND);
        }
        glBindTexture(GL_TEXTURE_CUBE_MAP, origTexID);
    }

    //-----------------------------------------------------------------------------
    // Name: ShowDepth()
    // Desc:
    //-----------------------------------------------------------------------------
    void ShowDepth(GLuint texID, GLint x, GLint y, GLsizei width,
                   GLsizei height, float nearZ, float farZ) {
        static const GLenum SHADER_TYPES[]
          = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
        // Vertex shader
        constexpr const auto vs
          = "#version 450 core\nuniform vec4 Vertex[4];\nout vec2 "
            "v_TexCoord;\nvoid main(void) {\nv_TexCoord  = "
            "Vertex[gl_VertexID].zw;\ngl_Position = "
            "vec4(Vertex[gl_VertexID].xy, 0.0, 1.0f);\n}";
        // Fragment shader GL_DEPTH_COMPONENT*
        constexpr const auto fs = "#version 450 core\n\
                layout (location = 0) out vec4 FragColor;\n\
                in      vec2      v_TexCoord;\n\
                uniform sampler2D u_Texture;\n\
                uniform float     u_NearZ;\n\
                uniform float     u_FarZ;\n\
                void main(void) {\n\
                  float z = texture(u_Texture, v_TexCoord).r;\n\
                  //float depthSample = 2.0 * z - 1.0;\n\
                  //float linear_z = 2.0 * u_NearZ * u_FarZ / (u_FarZ + u_NearZ - depthSample * (u_FarZ - u_NearZ));\n\
                  //float linear_z = 2.0 * u_NearZ * u_FarZ / (u_FarZ + u_NearZ + z * (u_NearZ - u_FarZ));\n\
                  float linear_z = (2.0 * u_NearZ) / (u_FarZ + u_NearZ - z * (u_FarZ - u_NearZ));\n\
                  FragColor = vec4(linear_z, linear_z, linear_z, 1.0);\n\
                }";
        // Fragment shader GL_DEPTH_COMPONENT* multisampled
        constexpr const auto fsMS = "#version 450 core\n\
                layout (location = 0) out vec4 FragColor;\n\
                in      vec2        v_TexCoord;\n\
                uniform sampler2DMS u_Texture;\n\
                uniform int         u_NumSamples;\n\
                uniform float       u_NearZ;\n\
                uniform float       u_FarZ;\n\
                void main(void) {\n\
                  float linear_z = 0.0;\n\
                  ivec2 texCoords = ivec2(v_TexCoord * textureSize(u_Texture));\n\
                  for (int i = 0; i < u_NumSamples; i++) {\n\
                     float z = texelFetch(u_Texture, texCoords, i).r;\n\
                     linear_z += (2.0 * u_NearZ) / (u_FarZ + u_NearZ - z * (u_FarZ - u_NearZ));\n\
                  }\n\
                  FragColor = vec4(linear_z, linear_z, linear_z, 1.0);\n\
                }";
        static GLuint programs[2] = {0};

        if (glIsTexture(texID) == GL_FALSE) {
            return;
        }

        // Prepare vertices
        glm::vec4 vp;
        glGetFloatv(GL_VIEWPORT, &vp.x);
        const GLfloat vertices[] = {
          (x - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          0.0f,
          0.0f,
          (x + width - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          1.0f,
          0.0f,
          (x + width - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y + height - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          1.0f,
          1.0f,
          (x - vp.x) / (vp.z - vp.x) * 2.0f - 1.0f,
          (y + height - vp.y) / (vp.w - vp.y) * 2.0f - 1.0f,
          0.0f,
          1.0f,
        };

        // Get texture properties
        GLint numSamples = 0;
        GLint format = 0;
        GLint compMode = 0;
        glGetTextureLevelParameteriv(texID, 0, GL_TEXTURE_SAMPLES, &numSamples);
        glGetTextureLevelParameteriv(texID, 0, GL_TEXTURE_INTERNAL_FORMAT,
                                     &format);
        GLenum target
          = (numSamples == 0) ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE;
        if (compMode != GL_NONE) {
            glTextureParameteri(texID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }

        // Select shader
        const char* sources[2] = {vs, (target == GL_TEXTURE_2D) ? fs : fsMS};
        int programIdx = (target == GL_TEXTURE_2D) ? 0 : 1;
        // Compile shaders
        if ((programs[programIdx] == 0)
            && !Shader::CreateShaderProgramFromSource(programs[programIdx], 2,
                                                      SHADER_TYPES, sources)) {
            spdlog::error("Texture::ShowDepth: Unable to compile shader "
                          "program.");
            return;
        }

        // Setup texture
        GLint origTexID = 0;
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &origTexID);
        glBindTexture(target, texID);

        // Render textured screen quad
        GLint curProgramId = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &curProgramId);
        const GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
        if (depthTest) {
            glDisable(GL_DEPTH_TEST);
        }

        // Render textured screen quad
        glUseProgram(programs[programIdx]);
        glUniform1i(glGetUniformLocation(programs[programIdx], "u_Texture"), 0);
        glUniform1i(glGetUniformLocation(programs[programIdx], "u_NumSamples"),
                    numSamples);
        glUniform4fv(glGetUniformLocation(programs[programIdx], "Vertex"), 4,
                     vertices);
        glUniform1f(glGetUniformLocation(programs[programIdx], "u_NearZ"),
                    nearZ);
        glUniform1f(glGetUniformLocation(programs[programIdx], "u_FarZ"), farZ);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glUseProgram(curProgramId);

        // Restore original states
        if (depthTest) {
            glEnable(GL_DEPTH_TEST);
        }
        if (compMode != GL_NONE) {
            glTextureParameteri(texID, GL_TEXTURE_COMPARE_MODE, compMode);
        }
        glBindTexture(GL_TEXTURE_2D, origTexID);
    }

    //-----------------------------------------------------------------------------
    // Name: GetImageData()
    // Desc:
    //-----------------------------------------------------------------------------
    bool GetImageData(const char* filename, std::vector<unsigned char>& data,
                      glm::ivec3* resolution) {
        int width = 0, height = 0, numChannels = 0;
        unsigned char* pixels = stbi_load(GetFilePath(filename).c_str(), &width,
                                          &height, &numChannels, 0);
        if (pixels) {
            data.reserve(data.size() + width * height * numChannels);
            data.insert(data.end(), &pixels[0],
                        &pixels[width * height * numChannels]);
            stbi_image_free(pixels);

            if (resolution) {
                resolution->x = width;
                resolution->y = height;
                resolution->z = numChannels;
            }
            return true;
        }
        return false;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateFromFile(const char* filename, bool mipmapped,
                          glm::ivec3* resolution) {
        GLuint texId = 0;
        int width = 0, height = 0, numChannels = 0;
        unsigned char* pixels = stbi_load(GetFilePath(filename).c_str(), &width,
                                          &height, &numChannels, 0);
        if (pixels && (numChannels == 3 || numChannels == 4)) {
            const auto log = glm::floor(glm::log2(glm::max(width, height)));
            const int numLevels = !mipmapped ? 1 : static_cast<int>(log + 1);

            glCreateTextures(GL_TEXTURE_2D, 1, &texId);
            glTextureParameteri(texId, GL_TEXTURE_MIN_FILTER,
                                mipmapped ? GL_LINEAR_MIPMAP_LINEAR
                                          : GL_LINEAR);
            glTextureParameteri(texId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(texId, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(texId, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTextureStorage2D(texId, numLevels, GL_RGBA8, width, height);
            glTextureSubImage2D(texId, 0, 0, 0, width, height,
                                (numChannels == 3) ? GL_RGB : GL_RGBA,
                                GL_UNSIGNED_BYTE, pixels);
            if (numLevels > 1) {
                glGenerateTextureMipmap(texId);
            }
            stbi_image_free(pixels);

            if (resolution) {
                resolution->x = width;
                resolution->y = height;
                resolution->z = numChannels;
            }
        }

        return texId;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateColored()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateColored(GLuint texture_id) {
        if (texture_id == 0) return 0;

        // Clone texture with colorized mipmap layers
        GLint width = 0, height = 0, max_level = 0;
        glGetTextureLevelParameteriv(texture_id, 0, GL_TEXTURE_WIDTH, &width);
        glGetTextureLevelParameteriv(texture_id, 0, GL_TEXTURE_HEIGHT, &height);
        glGetTextureParameteriv(texture_id, GL_TEXTURE_IMMUTABLE_LEVELS,
                                &max_level);
        if (width * height == 0) return 0;

        GLuint colored_texture_id = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &colored_texture_id);
        glTextureParameteri(colored_texture_id, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(colored_texture_id, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
        glTextureParameteri(colored_texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(colored_texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureStorage2D(colored_texture_id, max_level, GL_RGBA8, width,
                           height);

        std::vector<GLubyte> texels(width * height * 4);
        const GLubyte color_shift[10][3]
          = {{0, 6, 6}, {0, 0, 6}, {6, 0, 6}, {6, 0, 0}, {6, 6, 0},
             {0, 6, 0}, {3, 3, 3}, {6, 6, 6}, {3, 6, 3}, {6, 3, 3}};
        const int MAX_COLOR_LEVELS
          = sizeof(color_shift) / sizeof(color_shift[0]);

        for (GLint level = 0; level < max_level; level++) {
            glGetTextureLevelParameteriv(texture_id, level, GL_TEXTURE_WIDTH,
                                         &width);
            glGetTextureLevelParameteriv(texture_id, level, GL_TEXTURE_HEIGHT,
                                         &height);
            glGetTextureImage(texture_id, level, GL_RGBA, GL_UNSIGNED_BYTE,
                              width * height * 4 * sizeof(GLubyte), &texels[0]);

            for (int i = 0; i < width * height; i++) {
                const int shift_level = glm::min(level, MAX_COLOR_LEVELS);
                texels[4 * i + 0] >>= color_shift[shift_level][0];
                texels[4 * i + 1] >>= color_shift[shift_level][1];
                texels[4 * i + 2] >>= color_shift[shift_level][2];
            }
            glTextureSubImage2D(colored_texture_id, level, 0, 0, width, height,
                                GL_RGBA, GL_UNSIGNED_BYTE, &texels[0]);
        }

        assert(glGetError() == GL_NO_ERROR);
        return colored_texture_id;
    }

    //-----------------------------------------------------------------------------
    // Name: CreateSimpleTexture()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateSimpleTexture(GLint width, GLint height) {
        if (width * height <= 0) {
            return 0;
        }

        std::vector<GLubyte> texels;
        texels.reserve(width * height * 4);

        const GLubyte pattern_width = 16;
        GLubyte pattern = 0xFF;

        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                texels.emplace_back(pattern);
                texels.emplace_back(pattern);
                texels.emplace_back(pattern);
                texels.emplace_back(255);
                if (w % pattern_width == 0) pattern = ~pattern;
            }

            if (h % pattern_width == 0) pattern = ~pattern;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        GLsizei max_level = glm::log2(glm::max(width, height)) + 1;

        GLuint tex_id = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &tex_id);
        glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(tex_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
        glTextureStorage2D(tex_id, max_level, GL_RGBA8, width, height);
        glTextureSubImage2D(tex_id, 0, 0, 0, width, height, GL_RGBA,
                            GL_UNSIGNED_BYTE, &texels[0]);
        glGenerateTextureMipmap(tex_id);

        return tex_id;
    }

    //-----------------------------------------------------------------------------
    // Name: Create1D()
    // Desc:
    //-----------------------------------------------------------------------------
    void Create1D(GLuint& texture, GLenum format, GLsizei resolution) {
        glDeleteTextures(1, &texture);
        glCreateTextures(GL_TEXTURE_1D, 1, &texture);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureStorage1D(texture, 1, format, resolution);
    }

    //-----------------------------------------------------------------------------
    // Name: Create2D()
    // Desc:
    //-----------------------------------------------------------------------------
    void Create2D(GLuint& texture, GLenum format, const glm::ivec2& resolution,
                  GLsizei numSamples) {
        glDeleteTextures(1, &texture);
        glCreateTextures(numSamples == 0 ? GL_TEXTURE_2D
                                         : GL_TEXTURE_2D_MULTISAMPLE,
                         1, &texture);
        if (numSamples > 0) {
            glTextureStorage2DMultisample(texture, numSamples, format,
                                          resolution.x, resolution.y, GL_TRUE);
        } else {
            glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTextureStorage2D(texture, 1, format, resolution.x, resolution.y);
        }
    }

    //-----------------------------------------------------------------------------
    // Name: Create3D()
    // Desc:
    //-----------------------------------------------------------------------------
    void Create3D(GLuint& texture, GLenum format, const glm::ivec3& resolution,
                  bool mipmap) {
        glDeleteTextures(1, &texture);
        glCreateTextures(GL_TEXTURE_3D, 1, &texture);
        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
        const glm::ivec3 levels = glm::log2(resolution);
        const GLsizei max_level
          = mipmap ? glm::max(glm::max(levels.x, levels.y), levels.z) : 1;
        glTextureStorage3D(texture, max_level, format, resolution.x,
                           resolution.y, resolution.z);
        if (mipmap) {
            glGenerateTextureMipmap(texture);
        }
    }
}; // end of namespace Texture

namespace Buffer {
    //-----------------------------------------------------------------------------
    // Name: Create()
    // Desc:
    //-----------------------------------------------------------------------------
    void Create(GLuint& buffer, GLsizeiptr size, BufferStorageMask usage,
                GLenum target, GLuint index, const void* data) {
        glDeleteBuffers(1, &buffer);
        glCreateBuffers(1, &buffer);
        glNamedBufferStorage(buffer, size, data, usage);
        if (target == GL_ATOMIC_COUNTER_BUFFER
            || target == GL_TRANSFORM_FEEDBACK_BUFFER
            || target == GL_UNIFORM_BUFFER
            || target == GL_SHADER_STORAGE_BUFFER) {
            glBindBufferBase(target, index, buffer);
        }
    }
}; // namespace Buffer

namespace Framebuffer {

}

namespace Noise {
    /* Taken from "Lode's Computer Graphics Tutorial",
     * http://lodev.org/cgtutor/randomnoise.html */

    float Noise[NoiseHeight][NoiseWidth] = {0};

    //-----------------------------------------------------------------------------
    // Name: GenerateNoise()
    // Desc: Generates array of random values
    //-----------------------------------------------------------------------------
    void GenerateNoise() {
        if (Noise[0][0] != 0.0f) return;

        for (int y = 0; y < NoiseHeight; y++) {
            for (int x = 0; x < NoiseWidth; x++) {
                Noise[y][x] = glm::linearRand<float>(0, 1);
            }
        }
    }

    //-----------------------------------------------------------------------------
    // Name: SmoothNoise()
    // Desc:
    //-----------------------------------------------------------------------------
    float SmoothNoise(float x, float y) {
        // Get fractional part of x and y
        const float fractX = x - int(x);
        const float fractY = y - int(y);

        // Wrap around
        const int x1 = (int(x) + NoiseWidth) % NoiseWidth;
        const int y1 = (int(y) + NoiseHeight) % NoiseHeight;

        // Neighbor values
        const int x2 = (x1 + NoiseWidth - 1) % NoiseWidth;
        const int y2 = (y1 + NoiseHeight - 1) % NoiseHeight;

        // Smooth the noise with bilinear interpolation
        return fractY
                 * (fractX * Noise[y1][x1] + (1.0f - fractX) * Noise[y1][x2])
               + (1.0f - fractY)
                   * (fractX * Noise[y2][x1] + (1.0f - fractX) * Noise[y2][x2]);
    }

    //-----------------------------------------------------------------------------
    // Name: Turbulence()
    // Desc:
    //-----------------------------------------------------------------------------
    float Turbulence(int x, int y, int size) {
        const int initialSize = size;
        float value = 0.0;

        while (size >= 1) {
            const float sizeInv = 1.0f / size;
            value += SmoothNoise(x * sizeInv, y * sizeInv) * size;
            size >>= 1;
        }

        return (128.0f * value / initialSize);
    }

    //-----------------------------------------------------------------------------
    // Name: GenerateMarblePattern()
    // Desc:
    //-----------------------------------------------------------------------------
    bool GenerateMarblePattern(std::vector<glm::u8vec3> pixels,
                               unsigned int width, unsigned int height,
                               int turbSize, float turbPower, float xPeriod,
                               float yPeriod) {
        GenerateNoise();

        // xPeriod and yPeriod together define the angle of the lines
        // xPeriod and yPeriod both 0 ==> it becomes a normal clouds or
        // turbulence pattern
        xPeriod
          /= NoiseWidth; // defines repetition of marble lines in x direction
        yPeriod
          /= NoiseHeight; // defines repetition of marble lines in y direction

        pixels.resize(width * height);

        for (unsigned int y = 0; y < height; y++) {
            for (unsigned int x = 0; x < width; x++) {
                const float xyValue
                  = x * xPeriod + y * yPeriod
                    + turbPower * Turbulence(x, y, turbSize) * 0.01227184f;
                const float sineValue = 256 * fabs(sin(xyValue));
                pixels[x + y * width] = glm::u8vec3(sineValue);
            }
        }

        return true;
    }
}; // end of namespace Noise
}; // end of namespace Tools
