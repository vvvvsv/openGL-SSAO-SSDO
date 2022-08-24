#pragma once

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>
#include <random>

#include "gl_env.h"
#include "utils_shader_program.h"
#include <stb_image.h>

class RendererImage
{
private:
    ShaderProgram shaderImage;
    // textures
    unsigned int off_free;
    unsigned int ssao_free;
    unsigned int ssdo_free;
    unsigned int both_free;
    unsigned int off_lock;
    unsigned int ssao_lock;
    unsigned int ssdo_lock;
    unsigned int both_lock;

    unsigned int loadTexture(const char *path)
    {
        // load texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            std::cout << "Failed to load texture" << std::endl;
        }
        stbi_image_free(data);
        return texture;
    }

    unsigned int quadVAO;
    unsigned int quadVBO;


public:
    RendererImage()
    {
        // shader
        //-------------------------------
        shaderImage = ShaderProgram(SRC_DIR"/src/shader/image/image.vs", SRC_DIR"/src/shader/image/image.fs");
        shaderImage.setInt("textureImg", 0);

        // VAO
        //-------------------------------
        float quadVertices[] = {
            // positions        // texture Coords
            -0.7f,  0.07f - 0.8f, 0.0f, 0.0f, 1.0f,
            -0.7f, -0.07f - 0.8f, 0.0f, 0.0f, 0.0f,
             0.7f,  0.07f - 0.8f, 0.0f, 1.0f, 1.0f,
             0.7f, -0.07f - 0.8f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        // images
        //-------------------------------
        off_free  = loadTexture(DATA_DIR"/image/off_free.png");
        ssao_free = loadTexture(DATA_DIR"/image/ssao_free.png");
        ssdo_free = loadTexture(DATA_DIR"/image/ssdo_free.png");
        both_free = loadTexture(DATA_DIR"/image/both_free.png");
        off_lock  = loadTexture(DATA_DIR"/image/off_lock.png");
        ssao_lock = loadTexture(DATA_DIR"/image/ssao_lock.png");
        ssdo_lock = loadTexture(DATA_DIR"/image/ssdo_lock.png");
        both_lock = loadTexture(DATA_DIR"/image/both_lock.png");
    }

    void draw(int renderMode, int cameraFree)
    {
        glActiveTexture(GL_TEXTURE0);
        if (cameraFree == 1)
        {
            if (renderMode == 1) glBindTexture(GL_TEXTURE_2D, off_free);
            else if (renderMode == 2) glBindTexture(GL_TEXTURE_2D, ssao_free);
            else if (renderMode == 3) glBindTexture(GL_TEXTURE_2D, ssdo_free);
            else if (renderMode == 4) glBindTexture(GL_TEXTURE_2D, both_free);
            else std::cout << "renderMode not found: " << renderMode << std::endl;
        }
        else if (cameraFree == 0)
        {
            if (renderMode == 1) glBindTexture(GL_TEXTURE_2D, off_lock);
            else if (renderMode == 2) glBindTexture(GL_TEXTURE_2D, ssao_lock);
            else if (renderMode == 3) glBindTexture(GL_TEXTURE_2D, ssdo_lock);
            else if (renderMode == 4) glBindTexture(GL_TEXTURE_2D, both_lock);
            else std::cout << "renderMode not found: " << renderMode << std::endl;
        }
        else std::cout << "cameraFree not found: " << cameraFree << std::endl;

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        shaderImage.use();
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }
};