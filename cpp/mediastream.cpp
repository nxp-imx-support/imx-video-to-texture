/*
 * Copyright 2024 NXP
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mediastream.hpp"
#include "gltexturerenderer.hpp"
#include <QRunnable>
#include <QOpenGLContext>
#include <QDebug>

/**************************************************************************************************************
 *
 * @brief  			MediaStream Class
 *
 * @remarks 		QQuickItem that manages the Media viewport for the application.
 *                  Members: 
 *                      GlTextureRenderer *m_renderer : Responsible for rendering the frame
 *                      GstPlayer *m_player : Responsible for controlling gstreamer playback pipeline 
 *
 **************************************************************************************************************/

MediaStream::MediaStream()
    : m_renderer(nullptr),
      m_player(nullptr),
      m_isInitialized(false),
      m_isReadyToRender(false),
      m_playerHasFrame(false),
      m_autostart(true),
      m_looping(false),
      m_source(""),
      m_streamPositionPercentage(0.0f),
      m_width(-1),
      m_height(-1),
      m_ratio(1.0f)
{
    if (m_autostart) {
        m_playing = true;
    }
    // Call handleWindowChanged() when window item is entered into the scene
    connect(this, &QQuickItem::windowChanged, this, &MediaStream::handleWindowChanged);
    connect(this, &MediaStream::newFrame, this, &MediaStream::update);
    connect(this, &MediaStream::newFrame, this, &MediaStream::updateStreamPositionPercentage);
    setFlag(QQuickItem::ItemHasContents, true);
}

void MediaStream::handleWindowChanged(QQuickWindow *window)
{
    if (window) {
        // Call cleanup() when the scene graph has been invalidated.
        // User resources tied to graphics rendering context must be released.
        connect(window, &QQuickWindow::sceneGraphInvalidated, this, &MediaStream::cleanup,
                Qt::DirectConnection);
        // Use Qt::DirectConnection to ensure that callbacks are called in rendering thread, within
        // OpenGL context.
    }
}

QSGNode *MediaStream::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    if (m_isInitialized == false) {
        init();
    }
    m_renderer->setSize(width(), height());
    return m_renderer;
}

void MediaStream::paint()
{
    if (m_isInitialized == true && m_playerHasFrame == true) {
        GstPlayer::Texture texture = m_player->getTexture();
        m_renderer->setTexture(texture.id, texture.target);
    }
}

void MediaStream::cleanup()
{
    // QSGRenderNode m_renderer resource is managed by the scene graph.
    // So it is not released here.
    if (m_player) {
        delete m_player;
        m_player = nullptr;
    }
    m_isInitialized = false;
}

QString MediaStream::getSource()
{
    return m_source;
}

void MediaStream::setSource(QString source)
{
    m_source = source;
    if (m_isInitialized == true) {
        m_player->setVideo(m_source.toStdString());

        if (m_autostart) {
            m_player->play();
        }
    }
}

void MediaStream::pause()
{
    if (m_isInitialized == true) {
        m_player->pause();
        m_playing = false;
        Q_EMIT playingChanged();
    }
}
void MediaStream::play()
{
    if (m_isInitialized == true) {
        m_player->play();
        m_playing = true;
        Q_EMIT playingChanged();
    }
}

void MediaStream::onNewFrame()
{
    Q_EMIT newFrame();
    m_playerHasFrame = true;
    updateRatio();
}

void MediaStream::onPrerollDone()
{
    m_isReadyToRender = true;
    if (!m_playing) {
        m_player->pause();
    }
}

void MediaStream::init()
{
    if (m_isInitialized == false) {
        m_renderer = new GlTextureRenderer();
        m_renderer->init();

        auto glContext = static_cast<QOpenGLContext *>(window()->rendererInterface()->getResource(
                window(), QSGRendererInterface::OpenGLContextResource));
        auto eglContext = glContext->nativeInterface<QNativeInterface::QEGLContext>();
        m_player = new GstPlayer(eglContext->display(), eglContext->nativeContext());

        m_player->setListener(this);

        if (m_source != "") {
            m_player->setVideo(m_source.toStdString());
        } else {
            m_player->setVideoTestPattern();
        }

        if (m_autostart) {
            m_player->play();
        }

        m_isInitialized = true;

        connect(window(), &QQuickWindow::beforeRenderPassRecording, this, &MediaStream::paint,
                Qt::DirectConnection);
    }
}

void MediaStream::updateRatio()
{
    if (m_player != nullptr) {
        int width = m_player->getWidth();
        int height = m_player->getHeight();

        if ((width != -1 && m_width != width) || (height != -1 && m_height != height)) {
            m_width = width;
            m_height = height;
            m_ratio = (float)m_width / (float)m_height;
            Q_EMIT ratioChanged();
        }
    }
}

void MediaStream::skip(int n_sec)
{
    m_player->skip(n_sec);
    // Set state to play to get the progress bar to update. (stream will be paused by the progress
    // bar update callback if mediastream state is paused)
    m_player->play();
    Q_EMIT positionChanged();
}

void MediaStream::updateStreamPositionPercentage()
{
    m_streamPositionPercentage = m_player->getPercentage();
    // If stream is paused make sure the state of the player is changed (allows progress bar and
    // frame to update when stream is paused by user)
    if (!m_playing) {
        m_player->pause();
    }
    Q_EMIT positionChanged();
}

float MediaStream::getPosition()
{
    return m_streamPositionPercentage;
}

void MediaStream::setPosition(float percent)
{
    m_streamPositionPercentage = percent;
    m_player->seekToPercent(m_streamPositionPercentage);
    // Set state to play to get the progress bar to update. (stream will be paused by the progress
    // bar update callback if mediastream state is paused)
    m_player->play();
    Q_EMIT positionChanged();
}

bool MediaStream::getLooping()
{
    return m_looping;
}

void MediaStream::setLooping(bool looping)
{
    m_looping = looping;
    m_player->setLooping(m_looping);
    Q_EMIT loopingChanged();
}

void MediaStream::toggleLoop()
{
    m_looping = !m_looping;
    m_player->toggleLooping();
    Q_EMIT loopingChanged();
}

void MediaStream::setPlaying(bool playing)
{
    m_playing = playing;
    if (m_playing) {
        play();
    } else {
        pause();
    }
    Q_EMIT playingChanged();
}

bool MediaStream::getPlaying()
{
    return m_playing;
}

float MediaStream::getRatio()
{
    return m_ratio;
}

void MediaStream::releaseResources()
{
    cleanup();
}
