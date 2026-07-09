#pragma once

#include <string>
#include <unordered_map>

class Shader
{
public:
    Shader();

    ~Shader();

public:
    bool Load(
        const std::string& vertexFile,
        const std::string& fragmentFile);

    void Destroy();

    void Bind() const;

    void Unbind() const;

public:
    void SetBool(
        const std::string& name,
        bool value);

    void SetInt(
        const std::string& name,
        int value);

    void SetFloat(
        const std::string& name,
        float value);

    void SetVec2(
        const std::string& name,
        float x,
        float y);

    void SetVec3(
        const std::string& name,
        float x,
        float y,
        float z);

    void SetVec4(
        const std::string& name,
        float x,
        float y,
        float z,
        float w);

    void SetMatrix4(
        const std::string& name,
        const float* matrix);

private:
    unsigned int CompileShader(
        unsigned int type,
        const std::string& source);

    std::string ReadFile(
        const std::string& file);

    int GetLocation(
        const std::string& name);

private:
    unsigned int m_Program;

    std::unordered_map<
        std::string,
        int> m_UniformCache;
};