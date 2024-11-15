/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mediascreenshot.hpp"
#include "gltexturerenderer.hpp"
#include <QImage>

/**************************************************************************************************************
 *
 * @brief  			MediaScreenshot Class
 *
 * @remarks 		Inherits MediaStream : Used to display video thumbnail screenshots inside application
 *
 **************************************************************************************************************/

MediaScreenshot::MediaScreenshot()
    : m_state(State::WAITING), m_positionPercent(-1.0f), m_isLoaded(false)
{
    m_autostart = false;
}

void MediaScreenshot::handleWindowChanged(QQuickWindow *window)
{
    MediaStream::handleWindowChanged(window);

    if (window) {
        connect(window, &QQuickWindow::beforeSynchronizing, this, &MediaScreenshot::postRendering,
                Qt::DirectConnection);
    }
    m_state = State::PENDING;
}

void MediaScreenshot::take()
{
    m_state = State::PENDING;
    if (m_isInitialized) {
        if (m_isReadyToRender == true) {
            if (m_positionPercent > 0.0f) {
                m_player->seekToPercent(m_positionPercent);
            }
            m_player->play();
            m_state = State::START;
        }
    }
}

void MediaScreenshot::onNewFrame()
{
    MediaStream::onNewFrame();
    if (m_state == State::START) {
        m_state = State::RENDER;
    }
}

void MediaScreenshot::onPrerollDone()
{
    MediaStream::onPrerollDone(); // set m_isReadyToRender to true

    if (m_state == State::PENDING) {
        take();
    }
}

void MediaScreenshot::paint()
{
    if (m_state != State::DONE) {
        MediaStream::paint();
    }
}

void MediaScreenshot::postRendering()
{
    if (m_state == State::RENDER && m_renderer->isFirstRenderDone()) {
        m_renderer->setTexture(m_renderer->getOffscreenTexture());
        m_renderer->setOffscreen(false);
        m_state = State::DONE;
        m_player->pause();
        m_player->deinit();
        m_isLoaded = true;
        Q_EMIT loadedChanged();
    }
}

void MediaScreenshot::init()
{
    MediaStream::init();
    if (m_isInitialized) {
        m_renderer->setOffscreen(true);
        m_isLoaded = false;
        Q_EMIT loadedChanged();
        take();
    }
}

float MediaScreenshot::getAtPercent()
{
    return m_positionPercent;
}

void MediaScreenshot::setAtPercent(float positionPercent)
{
    m_positionPercent = positionPercent;
}

bool MediaScreenshot::getLoaded()
{
    return m_isLoaded;
}
