#include "Shader.h"

#include <fstream>
#include <sstream>

#include <glad/glad.h>

Shader::Shader()
    : m_Program(0)
{
}

Shader::~Shader()
{
    Destroy();
}

bool Shader::Load(
    const std::string&,
    const std::string&)
{
    return true;
}

void Shader::Destroy()
{
    if (m_Program != 0)
    {
        glDeleteProgram(m_Program);
        m_Program = 0;
    }
}

void Shader::Bind() const
{
    glUseProgram(m_Program);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

void Shader::SetBool(
    const std::string& name,
    bool value)
{
    glUniform1i(GetLocation(name), value);
}

void Shader::SetInt(
    const std::string& name,
    int value)
{
    glUniform1i(GetLocation(name), value);
}

void Shader::SetFloat(
    const std::string& name,
    float value)
{
    glUniform1f(GetLocation(name), value);
}

void Shader::SetVec2(
    const std::string& name,
    float x,
    float y)
{
    glUniform2f(GetLocation(name), x, y);
}

void Shader::SetVec3(
    const std::string& name,
    float x,
    float y,
    float z)
{
    glUniform3f(GetLocation(name), x, y, z);
}

void Shader::SetVec4(
    const std::string& name,
    float x,
    float y,
    float z,
    float w)
{
    glUniform4f(GetLocation(name), x, y, z, w);
}

void Shader::SetMatrix4(
    const std::string& name,
    const float* matrix)
{
    glUniformMatrix4fv(
        GetLocation(name),
        1,
        GL_FALSE,
        matrix);
}

unsigned int Shader::CompileShader(
    unsigned int,
    const std::string&)
{
    return 0;
}

std::string Shader::ReadFile(
    const std::string& file)
{
    std::ifstream stream(file);

    if (!stream.is_open())
        return "";

    std::stringstream ss;

    ss << stream.rdbuf();

    return ss.str();
}

int Shader::GetLocation(
    const std::string& name)
{
    auto it = m_UniformCache.find(name);

    if (it != m_UniformCache.end())
        return it->second;

    int location =
        glGetUniformLocation(
            m_Program,
            name.c_str());

    m_UniformCache[name] = location;

    return location;
}