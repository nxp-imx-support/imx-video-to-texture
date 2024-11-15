/*
 * Copyright 2024 NXP
 * Copyright (C) 2017 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QSGRenderNode>
#include <QOpenGLShaderProgram>

#define GL_INVALID_ID ((GLuint) - 1)

class GlTextureProgram : public QOpenGLShaderProgram
{
public:
    GlTextureProgram(std::string_view vertex, std::string_view fragment);
    ~GlTextureProgram();

    void setTextureUnit(GLuint textureUnit);
    void setMatrix(const QMatrix4x4 &matrix);
    void setOpacity(GLfloat opacity);

protected:
    int m_textureUniform;
    int m_matrixUniform;
    int m_opacityUniform;
};

class GlTextureRenderer : public QSGRenderNode
{
public:
    ~GlTextureRenderer();

    // Inherited from QSGRenderNode
    void render(const RenderState *state) override;
    void releaseResources() override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;

    void setSize(int width, int height);
    void init();
    void setTexture(GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);
    void setOffscreen(bool isOffscreen);
    GLuint getOffscreenTexture();
    bool isFirstRenderDone() { return m_isFirstRenderDone; }

protected:
    void enableFramebuffer();

private:
    int m_width = 0;
    int m_height = 0;
    GlTextureProgram *m_program2D = nullptr;
    GlTextureProgram *m_programExtOES = nullptr;
    GLuint m_textureId = GL_INVALID_ID;
    GLenum m_textureTarget = GL_TEXTURE_2D;
    std::array<GLfloat, 4 * 2> m_vertices = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
    std::array<GLfloat, 4 * 2> m_texcoords = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
    GLuint m_isOffscreen = false;
    GLuint m_fboId = GL_INVALID_ID;
    GLuint m_textureOffscreenId = GL_INVALID_ID;
    bool m_isFirstRenderDone = false;
};
