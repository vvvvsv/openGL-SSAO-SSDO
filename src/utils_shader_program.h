#pragma once

#include "gl_env.h"

#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>

// defines several possible options for shader categories
enum ShaderCategory
{
    SHADER_VERTEX,
    SHADER_FRAGMENT
};


// Shader Program
class ShaderProgram
{
private:
    // load shader from file
    GLuint loadShader(const char *path, ShaderCategory category)
    {
        // read code from file
        std::string strcode;
        std::ifstream shaderFile(path);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        strcode = shaderStream.str();
        const char *code = strcode.c_str();

        // compile shader
        GLuint shader;
        if (category == SHADER_VERTEX)
            shader = glCreateShader(GL_VERTEX_SHADER);
        else if (category == SHADER_FRAGMENT)
            shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(shader, 1, &code, NULL);
        glCompileShader(shader);

        // check whether the compilation is successful
        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        };
        return shader;
    }

public:
    GLuint programID;

    ShaderProgram() {programID = -1;}

    ShaderProgram(const char* vertexPath, const char* fragmentPath)
    {
        // load shaders
        GLuint vertexShader, fragmentShader;
        vertexShader   = loadShader(vertexPath, SHADER_VERTEX);
        fragmentShader = loadShader(fragmentPath, SHADER_FRAGMENT);

        // link the program
        programID = glCreateProgram();
        glAttachShader(programID, vertexShader);
        glAttachShader(programID, fragmentShader);
        glLinkProgram(programID);

        // check whether the link is successful
        int success;
        char infoLog[512];
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(programID, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            exit(1);
        }

        // delete the shaders
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    // activate the shaders
    void use()
    {
        glUseProgram(programID);
    }

    // locate uniform from shader program
    GLint locateUnifrom(const char *unifrom)
    {
        return glGetUniformLocation(programID, unifrom);
    }

    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const
    {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const
    {
        glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const glm::vec2 &value) const
    {
        glUniform2fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string &name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(programID, name.c_str()), x, y);
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value) const
    {
        glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string &name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(programID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string &name, float x, float y, float z, float w)
    {
        glUniform4f(glGetUniformLocation(programID, name.c_str()), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
};