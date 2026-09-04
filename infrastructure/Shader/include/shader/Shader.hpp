#ifndef SHADER_H
#define SHADER_H

#include<string>

namespace engineeringlab::infrastructure {
    class Shader {
    public:
        // 程序ID
        unsigned int m_ID;  
        
        // 构造器读取并构建着色器
        Shader(const char* vertexPath, const char *fragmentPath);
        
        // 使用/激活程序
        void use() const;
        
        // unifrom工具函数
        void setBool(const std::string &name, bool value) const;
        void setInt(const std::string &name, int value) const;
        void setFloat(const std::string &name, float value) const;
        void setVec2(const std::string& name, float x, float y) const;
        void setVec3(const std::string& name, float x, float y, float z) const;
        void setVec4(const std::string& name, float x, float y, float z, float w) const;
            
    private:
        // 检查shader编译错误或者program链接错误
        void checkCompileErrors(unsigned int shader, const std::string &type) const;
    };
}



#endif