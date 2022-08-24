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

#include <glm/gtc/matrix_transform.hpp>

#include "utils_shader_program.h"
#include "utils_mesh.h"
#include "utils_model.h"

#include "renderer_cube_quad.h"

#ifndef MY_LERP
#define MY_LERP(a, b, f) ((a) + (f) * ((b) - (a)))
#endif

class RendererSSAO
{
private:
    // shader programs
    ShaderProgram shaderGeometryPass;
    ShaderProgram shaderGeometryPlainPass;
    ShaderProgram shaderSSAO;
    ShaderProgram shaderBlur;
    ShaderProgram shaderLightingPass;
    ShaderProgram shaderLightBox;
    ShaderProgram shaderSkyBox;

    // g-buffer
    GLuint gBuffer, gPosition, gNormal, gAlbedo;

    // framebuffer to hold ssao & blur output
    GLuint ssaoFBO, ssaoBlurFBO;
    GLuint ssaoColorBuffer, ssaoColorBufferBlur;

    // the ssao kernel & noise
    std::vector<glm::vec3> ssaoKernel;
    GLuint noiseTexture;

    // skybox
    GLuint skyBoxTexture;

    RendererCubeQuad rendererCubeQuad;

public:
    RendererSSAO()
    {
        // load, compile and link shaders
        // ------------------------------
        shaderGeometryPass      = ShaderProgram(SRC_DIR"/src/shader/ssao/geometry.vs", SRC_DIR"/src/shader/ssao/geometry.fs");
        shaderGeometryPlainPass = ShaderProgram(SRC_DIR"/src/shader/ssao/geometry.vs", SRC_DIR"/src/shader/ssao/geometry_plain.fs");
        shaderSSAO              = ShaderProgram(SRC_DIR"/src/shader/ssao/ssao.vs", SRC_DIR"/src/shader/ssao/ssao.fs");
        shaderBlur              = ShaderProgram(SRC_DIR"/src/shader/ssao/blur.vs", SRC_DIR"/src/shader/ssao/blur.fs");
        shaderLightingPass      = ShaderProgram(SRC_DIR"/src/shader/ssao/lighting.vs", SRC_DIR"/src/shader/ssao/lighting.fs");
        shaderLightBox          = ShaderProgram(SRC_DIR"/src/shader/ssao/light_box.vs", SRC_DIR"/src/shader/ssao/light_box.fs");
        shaderSkyBox            = ShaderProgram(SRC_DIR"/src/shader/ssao/sky_box.vs", SRC_DIR"/src/shader/ssao/sky_box.fs");


        // configure g-buffer framebuffer
        // ------------------------------
        glGenFramebuffers(1, &gBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        // position color buffer
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 800, 800, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
        // normal color buffer
        glGenTextures(1, &gNormal);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 800, 800, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
        // color + specular color buffer
        glGenTextures(1, &gAlbedo);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 800, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
        // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
        unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, attachments);
        // create and attach depth buffer (renderbuffer)
        unsigned int rboDepth;
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 800, 800);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
        // finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // also create framebuffer to hold SSAO processing stage
        // -----------------------------------------------------
        glGenFramebuffers(1, &ssaoFBO);  glGenFramebuffers(1, &ssaoBlurFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        // SSAO color buffer
        glGenTextures(1, &ssaoColorBuffer);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "SSAO Framebuffer not complete!" << std::endl;
        // and blur stage
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glGenTextures(1, &ssaoColorBufferBlur);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // generate sample kernel
        // ----------------------
        std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
        std::default_random_engine generator;
        for (unsigned int i = 0; i < 32; ++i)
        {
            glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
            sample = glm::normalize(sample);
            sample *= randomFloats(generator);
            float scale = float(i) / 32.0f;

            // scale samples s.t. they're more aligned to center of kernel
            scale = MY_LERP(0.1f, 1.0f, scale * scale);
            sample *= scale;
            ssaoKernel.push_back(sample);
        }

        // generate noise texture
        // ----------------------
        std::vector<glm::vec3> ssaoNoise;
        for (unsigned int i = 0; i < 16; i++)
        {
            glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
            ssaoNoise.push_back(noise);
        }
        glGenTextures(1, &noiseTexture);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // shader configuration
        // --------------------
        shaderLightingPass.use();
        shaderLightingPass.setInt("gPosition", 0);
        shaderLightingPass.setInt("gNormal", 1);
        shaderLightingPass.setInt("gAlbedo", 2);
        shaderLightingPass.setInt("ssao", 3);
        shaderSSAO.use();
        shaderSSAO.setInt("gPosition", 0);
        shaderSSAO.setInt("gNormal", 1);
        shaderSSAO.setInt("texNoise", 2);
        shaderBlur.use();
        shaderBlur.setInt("ssaoInput", 0);
        shaderSkyBox.use();
        shaderSkyBox.setInt("skybox", 0);

        // skybox
        // --------------------
        std::vector<std::string> skyBoxFaces = {
            DATA_DIR"/skybox/right.jpg",
            DATA_DIR"/skybox/left.jpg",
            DATA_DIR"/skybox/top.jpg",
            DATA_DIR"/skybox/bottom.jpg",
            DATA_DIR"/skybox/front.jpg",
            DATA_DIR"/skybox/back.jpg"};
        skyBoxTexture = rendererCubeQuad.loadCubemap(skyBoxFaces);
    }

    void render(Model &my3DModel, Camera &camera, std::vector<Light> &lights, int width, int height, int plainModel)
    {
        // 1. geometry pass: render scene's geometry/color data into gbuffer
        // -----------------------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glm::mat4 projection = glm::perspective(glm::radians(camera.getFov()), (float)width / (float)height, 0.1f, 100.0f);
            glm::mat4 view = camera.getView();
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
            model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
            if (plainModel == 1)
            {
                shaderGeometryPlainPass.use();
                shaderGeometryPlainPass.setMat4("projection", projection);
                shaderGeometryPlainPass.setMat4("view", view);
                shaderGeometryPlainPass.setMat4("model", model);
                my3DModel.Draw(shaderGeometryPlainPass);
            }
            else
            {
                shaderGeometryPass.use();
                shaderGeometryPass.setMat4("projection", projection);
                shaderGeometryPass.setMat4("view", view);
                shaderGeometryPass.setMat4("model", model);
                my3DModel.Draw(shaderGeometryPass);
            }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. generate SSAO
        // ------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderSSAO.use();
            // Send kernel + rotation
            for (unsigned int i = 0; i < 32; ++i)
                shaderSSAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
            shaderSSAO.setMat4("projection", projection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, noiseTexture);
            rendererCubeQuad.renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // 3. blur SSAO texture to remove noise
        // ------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderBlur.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
            rendererCubeQuad.renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // 4. lighting pass: traditional deferred Blinn-Phong lighting with added screen-space ambient occlusion
        // -----------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderLightingPass.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
        glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
        // send light relevant uniforms
        for (unsigned int i = 0; i < lights.size(); i++)
        {
            glm::vec3 lightPosView = glm::vec3(camera.getView() * glm::vec4(lights[i].lightPos, 1.0));
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Position", lightPosView);
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Color", lights[i].lightColor);
            // update attenuation parameters and calculate radius
            const float linear = 0.7f;
            const float quadratic = 1.8f;
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
        }
        // finally render quad
        rendererCubeQuad.renderQuad();

        // 4.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
        // ----------------------------------------------------------------------------------
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
        // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
        // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the
        // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 5. render lights on top of scene
        // --------------------------------
        shaderLightBox.use();
        shaderLightBox.setMat4("projection", projection);
        shaderLightBox.setMat4("view", view);
        for (unsigned int i = 0; i < lights.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, lights[i].lightPos);
            model = glm::scale(model, glm::vec3(0.05f));
            shaderLightBox.setMat4("model", model);
            shaderLightBox.setVec3("lightColor", lights[i].lightColor);
            rendererCubeQuad.renderCube();
        }

        // 6. draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        shaderSkyBox.use();
        view = glm::mat4(glm::mat3(camera.getView())); // remove translation from the view matrix
        shaderSkyBox.setMat4("view", view);
        shaderSkyBox.setMat4("projection", projection);
        // skybox cube
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTexture);
        rendererCubeQuad.renderCube();
        glDepthFunc(GL_LESS); // set depth function back to default

    }
};