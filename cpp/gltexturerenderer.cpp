/*
 * Copyright 2024 NXP
 * Copyright (C) 2017 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gltexturerenderer.hpp"

#include <QOpenGLFunctions>

namespace {
const GLint TextureUnit = 0;

const std::array<GLfloat, 4 * 2> OffscreenVertices = { -1.0f, -1.0f, 1.0f, -1.0f,
                                                       -1.0f, 1.0f,  1.0f, 1.0f };
const std::array<GLfloat, 4 * 2> OffscreenTexcoords = { 0.0f, 0.0f, 1.0f, 0.0f,
                                                        0.0f, 1.0f, 1.0f, 1.0f };

constexpr std::string_view VertexSource = "attribute highp vec4 a_vertices;\n"
                                          "attribute highp vec2 a_coords;\n"
                                          "varying highp vec2 v_coords;\n"
                                          "uniform highp mat4 u_matrix;\n"
                                          "void main() {\n"
                                          "    v_coords = a_coords;\n"
                                          "    gl_Position = u_matrix * a_vertices;\n"
                                          "}\n";

constexpr std::string_view FragmentSource2D =
        "uniform sampler2D u_texture;\n"
        "uniform lowp float u_opacity;\n"
        "varying highp vec2 v_coords;\n"
        "void main() {\n"
        "    gl_FragColor = u_opacity * texture2D(u_texture, v_coords);\n"
        "}\n";

constexpr std::string_view FragmentSourceExtOES =
        "#extension GL_OES_EGL_image_external: require\n"
        "uniform samplerExternalOES u_texture;\n"
        "uniform lowp float u_opacity;\n"
        "varying highp vec2 v_coords;\n"
        "void main() {\n"
        "    gl_FragColor = u_opacity * texture2D(u_texture, v_coords);\n"
        "}\n";

} // namespace

/**************************************************************************************************************
 *
 * @brief  			GlTextureProgram Class
 *
 * @remarks 		QOpenGLShaderProgram Class to handle vertex and fragment shader programs used by the 
 *                  GlTextureRenderer Class
 *
 **************************************************************************************************************/

GlTextureProgram::GlTextureProgram(std::string_view vertex, std::string_view fragment)
{
    addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vertex.data());
    addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fragment.data());

    bindAttributeLocation("a_vertices", 0);
    bindAttributeLocation("a_coords", 1);
    link();

    m_textureUniform = uniformLocation("u_texture");
    m_matrixUniform = uniformLocation("u_matrix");
    m_opacityUniform = uniformLocation("u_opacity");
}

GlTextureProgram::~GlTextureProgram() { }

void GlTextureProgram::setTextureUnit(GLuint textureUnit)
{
    setUniformValue(m_textureUniform, textureUnit);
}

void GlTextureProgram::setMatrix(const QMatrix4x4 &matrix)
{
    setUniformValue(m_matrixUniform, matrix);
}

void GlTextureProgram::setOpacity(GLfloat opacity)
{
    setUniformValue(m_opacityUniform, opacity);
}

/**************************************************************************************************************
 *
 * @brief  			GlTextureRenderer Class
 *
 * @remarks 		QSGRenderNode Class responsible for rendering openGL texture inside QT application
 *
 **************************************************************************************************************/

GlTextureRenderer::~GlTextureRenderer()
{
    releaseResources();
}

void GlTextureRenderer::releaseResources()
{
    if (m_program2D != nullptr) {
        delete m_program2D;
    }
    if (m_programExtOES != nullptr) {
        delete m_programExtOES;
    }
    if (m_textureOffscreenId != GL_INVALID_ID) {
        glDeleteTextures(1, &m_textureOffscreenId);
    }
    if (m_fboId != GL_INVALID_ID) {
        glDeleteFramebuffers(1, &m_fboId);
    }
}

void GlTextureRenderer::init()
{
    // Program using GL_TEXTURE_2D
    m_program2D = new GlTextureProgram(VertexSource, FragmentSource2D);

    // Program using GL_TEXTURE_EXTERNAL_OES
    m_programExtOES = new GlTextureProgram(VertexSource, FragmentSourceExtOES);
}

void GlTextureRenderer::render(const RenderState *state)
{
    QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();

    // Don't try to render an invalid texture
    if (m_textureId == GL_INVALID_ID) {
        return;
    }

    enableFramebuffer();

    GlTextureProgram *program;
    if (m_textureTarget == GL_TEXTURE_EXTERNAL_OES) {
        program = m_programExtOES;
    } else {
        program = m_program2D;
    }
    program->bind();

    program->enableAttributeArray(0);
    program->enableAttributeArray(1);

    if (m_isOffscreen == true) {
        program->setAttributeArray(0, GL_FLOAT, OffscreenVertices.data(), 2);
        program->setAttributeArray(1, GL_FLOAT, OffscreenTexcoords.data(), 2);
        program->setMatrix(QMatrix4x4());
        gl->glDisable(GL_SCISSOR_TEST);
        gl->glDisable(GL_STENCIL_TEST);
        gl->glDisable(GL_DEPTH_TEST);
        gl->glDisable(GL_BLEND);
        program->setOpacity(1.0f);
    } else {
        program->setAttributeArray(0, GL_FLOAT, m_vertices.data(), 2);
        program->setAttributeArray(1, GL_FLOAT, m_texcoords.data(), 2);
        program->setMatrix(*state->projectionMatrix() * *matrix());

        if (state->scissorEnabled()) {
            gl->glEnable(GL_SCISSOR_TEST);
            const QRect r = state->scissorRect(); // already bottom-up
            gl->glScissor(r.x(), r.y(), r.width(), r.height());
        } else {
            gl->glDisable(GL_SCISSOR_TEST);
        }

        if (state->stencilEnabled()) {
            gl->glEnable(GL_STENCIL_TEST);
            gl->glStencilFunc(GL_EQUAL, state->stencilValue(), 0xFF);
            gl->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        } else {
            gl->glDisable(GL_STENCIL_TEST);
        }

        // Regardless of flags() returning DepthAwareRendering or not,
        // we have to test against what's in the depth buffer already.
        gl->glEnable(GL_DEPTH_TEST);

        program->setOpacity(float(inheritedOpacity()));

        gl->glEnable(GL_BLEND);
        gl->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Bind texture
    program->setTextureUnit(TextureUnit);
    gl->glActiveTexture(GL_TEXTURE0 + TextureUnit);
    gl->glBindTexture(m_textureTarget, m_textureId);

    // Render states
    gl->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    program->disableAttributeArray(0);
    program->disableAttributeArray(1);
    program->release();

    m_isFirstRenderDone = true;
}

QSGRenderNode::StateFlags GlTextureRenderer::changedStates() const
{
    return BlendState | ScissorState | StencilState | DepthState;
}

QSGRenderNode::RenderingFlags GlTextureRenderer::flags() const
{
    return BoundedRectRendering | DepthAwareRendering;
}

QRectF GlTextureRenderer::rect() const
{
    return QRect(0, 0, m_width, m_height);
}

void GlTextureRenderer::setSize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_vertices = { 0.0f,           0.0f,           m_width - 1.0f, 0.0f, 0.0f, m_height - 1.0f,
                   m_width - 1.0f, m_height - 1.0f };
}

void GlTextureRenderer::setTexture(GLuint textureId, GLenum textureTarget)
{
    m_textureId = textureId;
    m_textureTarget = textureTarget;
}

void GlTextureRenderer::setOffscreen(bool isOffscreen)
{
    m_isFirstRenderDone = (m_isOffscreen == isOffscreen);
    m_isOffscreen = isOffscreen;
}

GLuint GlTextureRenderer::getOffscreenTexture()
{
    return m_textureOffscreenId;
}

void GlTextureRenderer::enableFramebuffer()
{
    QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    if (m_isOffscreen == true) {
        if (m_fboId == GL_INVALID_ID) {
            gl->glGenTextures(1, &m_textureOffscreenId);
            gl->glBindTexture(GL_TEXTURE_2D, m_textureOffscreenId);

            gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB,
                             GL_UNSIGNED_BYTE, nullptr);

            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            gl->glGenFramebuffers(1, &m_fboId);
            gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
            gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                       m_textureOffscreenId, 0);

            GLenum status = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                qInfo() << "ERROR: FBO is incomplete. Status is " << status;
            }
        } else {
            gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
        }
        gl->glViewport(0, 0, m_width, m_height);
    }
}
