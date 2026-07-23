#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:
    Shader();
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool Load(const std::string& vertexFile, const std::string& fragmentFile);
    bool Load(const std::string& vertexFile, const std::string& fragmentFile, const std::string& geometryFile);

    void Destroy();

    void Bind() const;
    void Unbind() const;

    // Uniform setters
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, float x, float y) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, float x, float y, float z) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, float x, float y, float z, float w) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMat2(const std::string& name, const glm::mat2& mat) const;
    void SetMat3(const std::string& name, const glm::mat3& mat) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;

private:
    unsigned int CompileShader(unsigned int type, const std::string& source);
    std::string ReadFile(const std::string& file);
    int GetLocation(const std::string& name) const;

    unsigned int m_Program;
    mutable std::unordered_map<std::string, int> m_UniformCache;
};