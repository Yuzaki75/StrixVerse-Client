#include "Shader.h"

#include <format>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "../core/Logger.h"

Shader::Shader()
    : m_Program(0)
{
}

Shader::~Shader()
{
    Destroy();
}

bool Shader::Load(const std::string& vertexFile, const std::string& fragmentFile)
{
    return Load(vertexFile, fragmentFile, "");
}

bool Shader::Load(const std::string& vertexFile, const std::string& fragmentFile, const std::string& geometryFile)
{
    // Clean up any existing program
    if (m_Program != 0)
    {
        glDeleteProgram(m_Program);
        m_Program = 0;
    }
    m_UniformCache.clear();

    // Read shader sources
    std::string vertexCode = ReadFile(vertexFile);
    std::string fragmentCode = ReadFile(fragmentFile);
    std::string geometryCode;
    bool hasGeometry = !geometryFile.empty();
    if (hasGeometry)
        geometryCode = ReadFile(geometryFile);

    if (vertexCode.empty() || fragmentCode.empty() || (hasGeometry && geometryCode.empty()))
    {
        std::string errorMsg = std::format("Failed to read shader files: Vertex='{}', Fragment='{}', Geometry='{}'", vertexFile, fragmentFile, geometryFile);
        Logger::Error(errorMsg);
        return false;
    }

    // Compile shaders
    unsigned int vertex = CompileShader(GL_VERTEX_SHADER, vertexCode);
    unsigned int fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentCode);
    unsigned int geometry = 0;
    if (hasGeometry)
        geometry = CompileShader(GL_GEOMETRY_SHADER, geometryCode);

    if (vertex == 0 || fragment == 0 || (hasGeometry && geometry == 0))
    {
        // Cleanup shaders
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        if (hasGeometry && geometry) glDeleteShader(geometry);
        return false;
    }

    // Link program
    m_Program = glCreateProgram();
    glAttachShader(m_Program, vertex);
    glAttachShader(m_Program, fragment);
    if (hasGeometry)
        glAttachShader(m_Program, geometry);
    glLinkProgram(m_Program);

    // Check linking
    int success;
    char infoLog[512];
    glGetProgramiv(m_Program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(m_Program, 512, nullptr, infoLog);
        std::string errorMsg = std::format("Shader program linking failed: {}", infoLog);
        Logger::Error(errorMsg);
        glDeleteProgram(m_Program);
        m_Program = 0;
        // Cleanup shaders
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (hasGeometry) glDeleteShader(geometry);
        return false;
    }

    // Detach and delete shaders (they're no longer needed after linking)
    glDetachShader(m_Program, vertex);
    glDetachShader(m_Program, fragment);
    if (hasGeometry)
        glDetachShader(m_Program, geometry);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (hasGeometry)
        glDeleteShader(geometry);

    std::string geometryStr = hasGeometry ? ("  Geometry: " + geometryFile) : "";
    std::string infoMsg = std::format("Shader program created successfully from:\n  Vertex: {}\n  Fragment: {}\n{}", vertexFile, fragmentFile, geometryStr);
    Logger::Info(infoMsg);

    return true;
}

void Shader::Destroy()
{
    if (m_Program != 0)
    {
        glDeleteProgram(m_Program);
        m_Program = 0;
    }
    m_UniformCache.clear();
}

void Shader::Bind() const
{
    if (m_Program == 0)
    {
        Logger::Warning("Attempted to bind an uninitialized shader program.");
        return;
    }
    glUseProgram(m_Program);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

void Shader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(GetLocation(name), static_cast<int>(value));
}

void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(GetLocation(name), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(GetLocation(name), value);
}

void Shader::SetVec2(const std::string& name, float x, float y) const
{
    glUniform2f(GetLocation(name), x, y);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(GetLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(GetLocation(name), x, y, z);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(GetLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetVec4(const std::string& name, float x, float y, float z, float w) const
{
    glUniform4f(GetLocation(name), x, y, z, w);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(GetLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetMat2(const std::string& name, const glm::mat2& mat) const
{
    glUniformMatrix2fv(GetLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetMat3(const std::string& name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(GetLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(GetLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source)
{
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::string shaderTypeStr = (type == GL_VERTEX_SHADER) ? "VERTEX"
                     : (type == GL_FRAGMENT_SHADER) ? "FRAGMENT"
                     : (type == GL_GEOMETRY_SHADER) ? "GEOMETRY"
                     : "UNKNOWN";
        std::string errorMsg = std::format("{} shader compilation failed:\n{}", shaderTypeStr, infoLog);
        Logger::Error(errorMsg);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

std::string Shader::ReadFile(const std::string& file)
{
    std::ifstream stream(file);
    if (!stream.is_open())
    {
        std::string errorMsg = std::format("Failed to open shader file: {}", file);
        Logger::Error(errorMsg);
        return "";
    }
    std::stringstream ss;
    ss << stream.rdbuf();
    return ss.str();
}

int Shader::GetLocation(const std::string& name) const
{
    auto it = m_UniformCache.find(name);
    if (it != m_UniformCache.end())
        return it->second;

    int location = glGetUniformLocation(m_Program, name.c_str());
    if (location == -1)
    {
        std::string warnMsg = std::format("Uniform '{}' not found in shader program.", name);
        Logger::Warning(warnMsg);
    }
    m_UniformCache[name] = location;
    return location;
}