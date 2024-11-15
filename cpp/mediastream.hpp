/*
 * Copyright 2024 NXP
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QQuickItem>
#include <QQuickWindow>
#include <QString>
#include "gstplayer.hpp"

class GlTextureRenderer;

class MediaStream : public QQuickItem, public GstPlayerListener
{
    Q_OBJECT
    Q_PROPERTY(QString source READ getSource WRITE setSource)
    Q_PROPERTY(float position READ getPosition WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool looping READ getLooping WRITE setLooping NOTIFY loopingChanged)
    Q_PROPERTY(bool playing READ getPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(float ratio READ getRatio NOTIFY ratioChanged)
    QML_ELEMENT

public:
    MediaStream();
    QString getSource();
    void setSource(QString source);
    float getPosition();
    void setPosition(float percent);
    bool getLooping();
    void setLooping(bool loop);
    bool getPlaying();
    void setPlaying(bool playing);
    float getRatio();
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *);

public Q_SLOTS:
    virtual void paint();
    void cleanup();

    void pause();
    void play();
    void skip(int n_sec);
    void updateStreamPositionPercentage();
    void toggleLoop();

    // Inherited from GstPlayerListener
    virtual void onNewFrame() override;
    virtual void onPrerollDone() override;

Q_SIGNALS:
    void newFrame();
    void positionChanged();
    void loopingChanged();
    void playingChanged();
    void ratioChanged();

protected Q_SLOTS:
    virtual void handleWindowChanged(QQuickWindow *win);

protected:
    virtual void init();
    void updateRatio();
    void releaseResources() override;

    GlTextureRenderer *m_renderer;
    GstPlayer *m_player;
    bool m_isInitialized;
    bool m_isReadyToRender;
    bool m_playerHasFrame;
    bool m_autostart;
    bool m_looping;
    bool m_playing;
    float m_streamPositionPercentage;
    int m_width;
    int m_height;
    float m_ratio;
    QString m_source;
};