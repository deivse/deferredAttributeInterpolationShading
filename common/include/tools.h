//-----------------------------------------------------------------------------
//  [PGR2] Some helpful common functions
//  19/02/2014
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#pragma once

#include <cmath>
#include <glm/gtc/random.hpp>
#include <imgui.h>
#include <implot.h>
#include <string>
#include <vector>

#include <glbinding/gl/gl.h>
#include <glbinding/gl/extension.h>
#include <glbinding/glbinding.h>

#include <GLFW/glfw3.h>
#include <glm/gtx/integer.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/exponential.hpp>

#include "defines.h"

using namespace gl;

namespace Statistic {
extern float FPS; // Current FPS

namespace Frame {
    extern int GPUTime; // GPU frame time
    extern int CPUTime; // CPU frame time
    extern int ID;      // Id of the frame
};                      // namespace Frame

namespace GPUMemory {
    extern int DedicatedMemory; // Size of dedicated GPU memory
    extern int TotalMemory;     // Size of total GPU memory
    extern int AllocatedMemory; // Size of allocated GPU memory
    extern int FreeMemory;      // Size of available GPU memory
    extern int EvictedMemory;   // Size of available GPU memory
};                              // namespace GPUMemory
};                              // end of namespace Statistic

namespace OpenGL {
enum eQueries
{
    FrameStartQuery = 0,
    FrameEndQuery,
    NumQueries
};

struct Program
{
    Program(GLuint _id);
    bool hasMVMatrix() const {
        return (MVPMatrix > -1) || (ModelViewMatrix > -1)
               || (ModelViewMatrixInverse > -1) || (ViewMatrix > -1)
               || (ModelMatrix > -1);
    }
    bool hasZOffset() const { return (ZOffset > -1); }
    GLuint id;
    GLint MVPMatrix;
    GLint ModelMatrix;
    GLint ViewMatrix;
    GLint ModelViewMatrix;
    GLint ModelViewMatrixInverse;
    GLint ProjectionMatrix;
    GLint NormalMatrix;
    GLint Viewport;
    GLint ZOffset;
    GLint VariableInt;
    GLint VariableFloat;
    GLint FrameCounter;
};
extern std::vector<Program> programs;

extern GLuint Query[NumQueries];
}; // end of namespace OpenGL

namespace Tools {
//-----------------------------------------------------------------------------
// Name: BufferRelease
// Desc:
//-----------------------------------------------------------------------------
template<GLenum target, GLenum query>
class BufferRelease
{
public:
    BufferRelease() {
        glGetIntegerv(query, &buffer);
        if (buffer > 0) glBindBuffer(target, 0);
    }
    ~BufferRelease() {
        if (buffer > 0) glBindBuffer(target, buffer);
    }

private:
    GLint buffer;
};

using VertexBufferRelease
  = BufferRelease<GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING>;
using IndexBufferRelease
  = BufferRelease<GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING>;
using QueryBufferRelease
  = BufferRelease<GL_QUERY_BUFFER, GL_QUERY_BUFFER_BINDING>;
using ShaderStorageBufferRelease
  = BufferRelease<GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER_BINDING>;
using TransformFeedbackBufferRelease
  = BufferRelease<GL_TRANSFORM_FEEDBACK_BUFFER,
                  GL_TRANSFORM_FEEDBACK_BUFFER_BINDING>;
using UniformBufferRelease
  = BufferRelease<GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING>;

//-----------------------------------------------------------------------------
// Name: GetCPUTime()
// Desc:
//-----------------------------------------------------------------------------
inline double GetCPUTime(bool micro = true) {
    return glfwGetTime() * (micro ? 1000000 : 1000000000);
}

//-----------------------------------------------------------------------------
// Name: GetGPUTime()
// Desc:
//-----------------------------------------------------------------------------
inline double GetGPUTime(bool micro = true) {
    GLint64 time = 0;
    glGetInteger64v(GL_TIMESTAMP, &time);
    return micro ? time * 0.001 : time;
}

// Very simple GPU timer (timer cannot be nested with other timer or
// GL_TIME_ELAPSED query
class GPUTimer
{
public:
    GPUTimer() = default;
    ~GPUTimer();

    void start(bool sync = false);

    void stop(bool sync = false) const;

    // micro - show in microsec, otherwise in nanosec
    unsigned int get(bool micro = true);

    // micro - show in microsec, otherwise in nanosec
    unsigned int getAverage(bool micro = true);

    unsigned int getCounter() const { return counter; }

    void reset() {
        timeTotal = 0;
        counter = 0;
    }

    GLuint getId(bool start) const { return start ? queryStart : queryStop; }

private:
    GLuint queryStart{}, queryStop{};
    GLuint64 time{};
    GLuint64 timeTotal{};
    GLuint counter{};
};

// Very simple GPU timer (timer cannot be nested with other timer or
// GL_TIME_ELAPSED query
class GPURangeTimer
{
public:
    GPURangeTimer() = default;
    ~GPURangeTimer() {
        if (query) glDeleteQueries(1, &query);
    }

    void start(bool sync = false);

    void stop(bool sync = false) const;

    // micro - show in microsec, otherwise in nanosec
    unsigned int get(bool micro = true);

    // micro - show in microsec, otherwise in nanosec
    unsigned int getAverage(bool micro = true);

    unsigned int getCounter() const { return counter; }

    void reset() {
        time = 0;
        timeTotal = 0;
        counter = 0;
    }

    GLuint getId() const { return query; }

private:
    GLuint query{};
    GLuint64 time{};
    GLuint64 timeTotal{};
    GLuint counter{};
};

// Very simple CPU timer
class CPUTimer
{
public:
    void start();

    void stop();

    unsigned int get(bool micro = true);

    unsigned int getAverage(bool micro = true);

    unsigned int getCounter() const { return counter; }

    void reset();

private:
    double timeStart{}, timeStop{};
    double time{};
    unsigned long long timeTotal{};
    unsigned int counter{};
};

// Very simple GL query wrapper
template<GLenum QueryTarget>
class GPUQuery
{
public:
    GPUQuery() = default;
    ~GPUQuery() {
        if (query) glDeleteQueries(1, &query);
    }

    void start() {
        if (query == 0) glCreateQueries(QueryTarget, 1, &query);
        result = 0;
        glBeginQuery(QueryTarget, query);
    }

    void stop() const {
        if ((result == 0) && (query > 0)) {
            glEndQuery(QueryTarget);
        }
    }

    unsigned int get() {
        if ((result == 0) && (query > 0)) {
            // Unbind query buffer if bound
            QueryBufferRelease queryBufferRelease;

            glGetQueryObjectui64v(query, GL_QUERY_RESULT, &result);
            resultTotal += result;
            counter++;
        }
        return static_cast<unsigned int>(result);
    }

    unsigned int getAverage() const {
        return static_cast<unsigned int>(
          static_cast<double>(resultTotal) / counter + 0.5);
    }

    unsigned int getCounter() const { return counter; }

    void clear() {
        result = 0;
        resultTotal = 0;
        counter = 0;
    }

    GLuint getId() const { return query; }

private:
    GLuint query{};
    GLuint64 result{};
    GLuint64 resultTotal{};
    GLuint counter{};
};

using GPUVSInvocationQuery = GPUQuery<GLenum::GL_VERTEX_SHADER_INVOCATIONS_ARB>;
using GPUFSInvocationQuery
  = GPUQuery<GLenum::GL_FRAGMENT_SHADER_INVOCATIONS_ARB>;

//-----------------------------------------------------------------------------
// Name: SaveFramebuffer()
// Desc:
//-----------------------------------------------------------------------------
void SaveFramebuffer(GLuint framebufferId = 0, GLsizei width = 0,
                     GLsizei height = 0);

//-----------------------------------------------------------------------------
// Name: GetFilePath()
// Desc:
//-----------------------------------------------------------------------------
std::string GetFilePath(const char* fileName);
//-----------------------------------------------------------------------------
// Name: OpenFile()
// Desc: TODO: fstream
//-----------------------------------------------------------------------------
FILE* OpenFile(const char* file_name);

//-----------------------------------------------------------------------------
// Name: ReadFile()
// Desc: TODO: use 'fstream'
//-----------------------------------------------------------------------------
bool ReadFile(std::vector<char>& buffer, const char* file_name);

//-----------------------------------------------------------------------------
// Name: WriteFile()
// Desc: TODO: use 'fstream'
//-----------------------------------------------------------------------------
bool WriteFile(void* data, size_t size, const char* fileName);

namespace Mesh {
    //-----------------------------------------------------------------------------
    // Name: CreatePlane()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreatePlane(float size, int slice_x, int slice_y,
                     std::vector<glm::vec3>& vertices);

    //-----------------------------------------------------------------------------
    // Name: CreatePlane()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreatePlane(unsigned int vertex_density, unsigned int index_density,
                     GLenum mode, std::vector<glm::vec2>& vertices,
                     std::vector<GLuint>& indices);

    //-----------------------------------------------------------------------------
    // Name: CreateCircle()
    // Desc:
    //-----------------------------------------------------------------------------
    template<class INDEX_TYPE>
    bool CreateCircle(GLenum primitive_type, unsigned int num_triangles,
                      float z, std::vector<glm::vec3>& vertices,
                      std::vector<INDEX_TYPE>* indices);

    //-----------------------------------------------------------------------------
    // Name: CreateSphereVertexMesh()
    // Desc: Sphere for OpenGL with (radius, sectors, stacks)
    //       The min number of sectors is 3 and the min number of stacks are 2.
    // Auth: Song Ho Ahn (song.ahn@gmail.com)
    //-----------------------------------------------------------------------------
    bool CreateSphereVertexMesh(std::vector<glm::vec3>& vertices,
                                std::vector<GLushort>& indices, float radius,
                                int sectorCount, int stackCount);

    //-----------------------------------------------------------------------------
    // Name: CreateSphereVertexMesh()
    // Desc: Sphere for OpenGL with (radius, sectors, stacks)
    //       The min number of sectors is 3 and the min number of stacks are 2.
    // Auth: Song Ho Ahn (song.ahn@gmail.com)
    //-----------------------------------------------------------------------------
    bool CreateSphereVertexMesh(std::vector<glm::vec3>& vertices, float radius,
                                int sectorCount, int stackCount);
} // end of namespace Mesh

namespace Shader {
    //-----------------------------------------------------------------------------
    // Name: SaveBinaryCode()
    // Desc:
    //-----------------------------------------------------------------------------
    bool SaveBinaryCode(GLuint programId, const char* fileName);

    //-----------------------------------------------------------------------------
    // Name: CheckShaderInfoLog()
    // Desc:
    //-----------------------------------------------------------------------------
    void CheckShaderInfoLog(GLuint shader_id);

    //-----------------------------------------------------------------------------
    // Name: CheckProgramInfoLog()
    // Desc:
    //-----------------------------------------------------------------------------
    void CheckProgramInfoLog(GLuint program_id);

    //-----------------------------------------------------------------------------
    // Name: CheckShaderCompileStatus()
    // Desc:
    //-----------------------------------------------------------------------------
    GLboolean CheckShaderCompileStatus(GLuint shader_id);

    //-----------------------------------------------------------------------------
    // Name: CheckProgramLinkStatus()
    // Desc:
    //-----------------------------------------------------------------------------
    GLint CheckProgramLinkStatus(GLuint program_id);
    //-----------------------------------------------------------------------------
    // Name: CreateShaderFromSource()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateShaderFromSource(GLenum shader_type, const char* source,
                                  const char* shader_name = nullptr);

    //-----------------------------------------------------------------------------
    // Name: CreateShaderFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateShaderFromFile(GLenum shader_type, const char* file_name,
                                const char* preprocessor = nullptr);

    //-----------------------------------------------------------------------------
    // Name: CreateShaderProgram()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateShaderProgram(GLuint& programId, GLint count,
                             const GLuint* shader_ids);

    //-----------------------------------------------------------------------------
    // Name: CreateShaderProgramFromSource()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateShaderProgramFromSource(GLuint& programId, GLint count,
                                       const GLenum* shader_types,
                                       const char* const* source);
    //-----------------------------------------------------------------------------
    // Name: CreateShaderProgramFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateShaderProgramFromFile(GLuint& programId, const char* vs,
                                     const char* tc, const char* te,
                                     const char* gs, const char* fs,
                                     const char* preprocessor = nullptr,
                                     const std::vector<char*>* tbx = nullptr);
    //-----------------------------------------------------------------------------
    // Name: CreateMeshShaderProgramFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateMeshShaderProgramFromFile(GLuint& programId, const char* mesh,
                                         const char* task, const char* fragment,
                                         const char* preprocessor = nullptr);
    //-----------------------------------------------------------------------------
    // Name: CreateComputeShaderProgramFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    bool CreateComputeShaderProgramFromFile(GLuint& programId, const char* cs,
                                            const char* preprocessor = nullptr);
} // end of namespace Shader

namespace Texture {
    //-----------------------------------------------------------------------------
    // Name: Show2D()
    // Desc:
    //-----------------------------------------------------------------------------
    void Show2D(GLuint texID, int x, int y, int width, int height,
                float scale = 1.0f, int lod = 0, bool border = false);

    //-----------------------------------------------------------------------------
    // Name: Show3D()
    // Desc:
    //-----------------------------------------------------------------------------
    void Show3D(GLuint texID, GLint slice, GLint x, GLint y, GLsizei width,
                GLsizei height, float scale = 1.0f, int lod = 0,
                bool border = false);
    //-----------------------------------------------------------------------------
    // Name: ShowCube()
    // Desc:
    //-----------------------------------------------------------------------------
    void ShowCube(GLuint texID, GLint x, GLint y, GLsizei width,
                  GLsizei height);

    //-----------------------------------------------------------------------------
    // Name: ShowDepth()
    // Desc:
    //-----------------------------------------------------------------------------
    inline void ShowDepth(GLuint texID, GLint x, GLint y, GLsizei width,
                          GLsizei height, float nearZ, float farZ);
    //-----------------------------------------------------------------------------
    // Name: GetImageData()
    // Desc:
    //-----------------------------------------------------------------------------
    bool GetImageData(const char* filename, std::vector<unsigned char>& data,
                      glm::ivec3* resolution = nullptr);

    //-----------------------------------------------------------------------------
    // Name: CreateFromFile()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateFromFile(const char* filename, bool mipmapped = true,
                          glm::ivec3* resolution = nullptr);
    //-----------------------------------------------------------------------------
    // Name: CreateColored()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateColored(GLuint texture_id);

    //-----------------------------------------------------------------------------
    // Name: CreateSimpleTexture()
    // Desc:
    //-----------------------------------------------------------------------------
    GLuint CreateSimpleTexture(GLint width, GLint height);

    //-----------------------------------------------------------------------------
    // Name: Create2D()
    // Desc:
    //-----------------------------------------------------------------------------
    void Create2D(GLuint& texture, GLenum format, const glm::ivec2& resolution,
                  GLsizei numSamples = 0);

    //-----------------------------------------------------------------------------
    // Name: Create3D()
    // Desc:
    //-----------------------------------------------------------------------------
    void Create3D(GLuint& texture, GLenum format, const glm::ivec3& resolution,
                  bool mipmap = false);
}; // end of namespace Texture

namespace Buffer {
    //-----------------------------------------------------------------------------
    // Name: Create()
    // Desc:
    //-----------------------------------------------------------------------------
    void Create(GLuint& buffer, GLsizeiptr size, BufferStorageMask usage,
                GLenum target, GLuint index, const void* data = nullptr);
}; // namespace Buffer

namespace Noise {
    /* Taken from "Lode's Computer Graphics Tutorial",
     * http://lodev.org/cgtutor/randomnoise.html */

    constexpr int NoiseWidth = 128;
    constexpr int NoiseHeight = 128;
    extern float Noise[NoiseHeight][NoiseWidth]; // The noise array

    //-----------------------------------------------------------------------------
    // Name: GenerateNoise()
    // Desc: Generates array of random values
    //-----------------------------------------------------------------------------
    void GenerateNoise();

    //-----------------------------------------------------------------------------
    // Name: SmoothNoise()
    // Desc:
    //-----------------------------------------------------------------------------
    float SmoothNoise(float x, float y);

    //-----------------------------------------------------------------------------
    // Name: Turbulence()
    // Desc:
    //-----------------------------------------------------------------------------
    float Turbulence(int x, int y, int size);

    //-----------------------------------------------------------------------------
    // Name: GenerateMarblePattern()
    // Desc:
    //-----------------------------------------------------------------------------
    bool GenerateMarblePattern(std::vector<glm::u8vec3> pixels,
                               unsigned int width, unsigned int height,
                               int turbSize = 32, float turbPower = 5.0f,
                               float xPeriod = 5.0f, float yPeriod = 10.0f);
}; // end of namespace Noise

namespace GUI {
    // Utility structure for realtime plot
    struct RollingBuffer
    {
        float Span;
        ImVector<ImVec2> Data;
        RollingBuffer() {
            Span = 10.0f;
            Data.reserve(2000);
        }
        void AddPoint(float x, float y) {
            float xmod = fmodf(x, Span);
            if (!Data.empty() && xmod < Data.back().x) Data.shrink(0);
            Data.push_back(ImVec2(xmod, y));
        }
    };

    template<typename CounterT>
    class Graph
    {
    public:
        Graph(const char* name, glm::ivec2 size, CounterT* counters,
              const std::vector<char*>& titles)
          : name(name), size(size.x, size.y), counters(counters),
            titles(titles) {
            values.resize(titles.size());
            data.resize(titles.size());
        }

        void update(bool showHistory = false) {
            ImGui::Text(name);

            static unsigned int maxValue = 0;
            unsigned int sumValues = 0;
            for (int i = 0; i < titles.size(); i++) {
                unsigned int value = counters[i].get();
                values[i] = value;
                sumValues += value;
                maxValue = glm::max(maxValue, value);
            }

            for (int i = 0; i < titles.size(); i++) {
                ImGui::SetNextItemWidth(100.0f * IMGUI_RESIZE_FACTOR);
                ImGui::PushStyleColor(
                  ImGuiCol_PlotHistogram,
                  (ImVec4)ImColor(0.02f, 0.62f, 0.82f, 1.0f));
                ImGui::ProgressBar(values[i] / float(sumValues),
                                   ImVec2(0.0f, 0.0f));
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::Text("%s: %u", titles[i], values[i]);
            }
            ImGui::PopStyleColor(titles.size());
            if (titles.size() > 1) {
                ImGui::Text("total: %u", sumValues);
            }

            if (showHistory) {
                ImGui::SetNextItemWidth(
                  glm::max(90.0f, size.x / IMGUI_RESIZE_FACTOR - 55.0f)
                  * IMGUI_RESIZE_FACTOR);
                ImGui::SliderFloat("History", &history, 10, 1000,
                                   "%.0f frames");
                for (int i = 0; i < titles.size(); i++) {
                    Tools::GUI::RollingBuffer& buffer = data[i];
                    buffer.Span = history;
                    buffer.AddPoint(Statistic::Frame::ID, values[i]);
                }

                static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_NoTickLabels;
                ImPlot::SetNextAxisLimits(ImAxis_X1, 0, history);
                ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, maxValue);
                if (ImPlot::BeginPlot("##Rolling")) {
                    ImPlot::SetupAxis(ImAxis_X1, nullptr,
                                      ImPlotAxisFlags_NoTickLabels);
                    ImPlot::SetupAxis(ImAxis_Y1, nullptr,
                                      ImPlotAxisFlags_NoTickLabels);
                    for (int i = 0; i < titles.size(); i++) {
                        ImPlot::PlotLine(
                          titles[i], &data[i].Data[0].x, &data[i].Data[0].y,
                          data[i].Data.size(), 0, 2 * sizeof(float));
                    }
                    ImPlot::EndPlot();
                }
            }
        }

    protected:
        const char* name;
        ImVec2 size;
        CounterT* counters;
        const std::vector<char*>& titles;
        std::vector<unsigned int> values;
        std::vector<Tools::GUI::RollingBuffer> data;
        float history{};
    };
}; // end of namespace GUI
}; // end of namespace Tools
