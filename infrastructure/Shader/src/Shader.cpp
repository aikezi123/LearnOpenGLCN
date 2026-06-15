#include <learnopengl/Shader.hpp>
#include <glad/glad.h>

#include<fstream>
#include<iostream>
#include<sstream>

namespace learnopengl::infrastructure {
    Shader::Shader(const char *vertexPath, const char *fragmentPath) {
        // 1. 从文件路径中读取顶点着色器和片段着色器源码
        std::string vertexCode;
        std::string fragmentCode;

        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // 让ifstream在读取失败时抛出异常
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            // 打开文件
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);

            std::stringstream vShaderStream;
            std::stringstream fShaderStream;

            // 读取文件内容到字符串流
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            // 关闭文件
            vShaderFile.close();
            fShaderFile.close();

            // 字符串六转换为std::string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        } catch (const std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n"
                      << "Vertex path: " << vertexPath << '\n'
                      << "Fragment path: " << fragmentPath << '\n'
                      << "Reason: " << e.what() << std::endl;
        
            m_ID = 0;
            return;
        }

        const char *vShaderCode = vertexCode.c_str();
        const char *fShaderCode = fragmentCode.c_str();

        // 2. 编译顶点着色器
        unsigned int vertexID = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexID, 1, &vShaderCode, nullptr);
        glCompileShader(vertexID);
        checkCompileErrors(vertexID, "VERTEX");

        // 3. 编译片段着色器
        unsigned int fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentID, 1, &fShaderCode, nullptr);
        glCompileShader(fragmentID);
        checkCompileErrors(fragmentID, "FRAGMENT");

        // 4. 创建Shader Program，并连接两个着色器
        m_ID = glCreateProgram();
        glAttachShader(m_ID, vertexID);
        glAttachShader(m_ID, fragmentID);
        glLinkProgram(m_ID);
        checkCompileErrors(m_ID, "PROGRAM");

        // 5. 链接完成后，单独的shader对象就不再需要了
        glDeleteShader(vertexID);
        glDeleteShader(fragmentID);
        
    }

    void Shader::use() const {
        glUseProgram(m_ID);
    }

    void Shader::setBool(const std::string &name, bool value) const {
        glUniform1i(
            glGetUniformLocation(m_ID, name.c_str()),
            static_cast<int>(value)
        );
    }

    void Shader::setInt(const std::string &name, int value) const {
        glUniform1i(
            glGetUniformLocation(m_ID, name.c_str()),
            value
        );
    }

    void Shader::setFloat(const std::string &name,  float value) const {
        glUniform1f(
            glGetUniformLocation(m_ID, name.c_str()),
            value
        );
    }

    void Shader::setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(
            glGetUniformLocation(m_ID, name.c_str()), 
            x, 
            y
        );
    }

    void Shader::setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(
            glGetUniformLocation(m_ID, name.c_str()), 
            x, 
            y, 
            z
        );
    }

    void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(
            glGetUniformLocation(m_ID, name.c_str()), 
            x, 
            y, 
            z, 
            w
        );
    }

    void Shader::checkCompileErrors(unsigned int shader, const std::string &type) const {
        int success;
        char infoLog[1024];

        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type:"
                            << type << '\n'
                            << infoLog << '\n'
                            << "-- --------------------------------------------------- --"
                            << std::endl;
            }

        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);

                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type:"
                          << type << '\n'
                          << infoLog << '\n'
                          << "-- --------------------------------------------------- --"
                          << std::endl;
            }
            
        }

        
    }


}