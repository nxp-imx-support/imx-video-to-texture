/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "mediastream.hpp"

class GlTextureRenderer;

class MediaScreenshot : public MediaStream
{
    Q_OBJECT
    Q_PROPERTY(float atPercent READ getAtPercent WRITE setAtPercent)
    Q_PROPERTY(bool loaded READ getLoaded NOTIFY loadedChanged)
    QML_ELEMENT

public:
    MediaScreenshot();

    // Inherited from GstPlayerListener
    virtual void onNewFrame() override;
    virtual void onPrerollDone() override;

Q_SIGNALS:
    void loadedChanged();

public Q_SLOTS:
    virtual void paint() override;
    float getAtPercent();
    void setAtPercent(float positionPercent);
    bool getLoaded();

protected:
    void take();
    void postRendering();
    virtual void init();

protected Q_SLOTS:
    virtual void handleWindowChanged(QQuickWindow *win) override;

protected:
    enum class State { WAITING, PENDING, START, RENDER, DONE };
    State m_state;
    float m_positionPercent;
    bool m_isLoaded;

private:
    using MediaStream::pause;
    using MediaStream::play;
};