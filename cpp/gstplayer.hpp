/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <EGL/egl.h>
#include <gst/gl/egl/gstgldisplay_egl.h>
#include <gst/gl/gl.h>
#include <gst/gst.h>
#include <string>
#include <mutex>

class GstLib
{
public:
    GstLib();
    ~GstLib();
};

class GstPlayerListener
{
public:
    virtual void onNewFrame() = 0;
    virtual void onPrerollDone() = 0;
};

class GstPlayer
{
public:
    struct Texture
    {
        guint id;
        guint target;
    };

    GstPlayer(EGLDisplay eglDisplay, EGLContext eglContext);
    virtual ~GstPlayer();

    void play();
    void pause();
    void skip(int n_sec);
    void toggleLooping();
    void setLooping(bool loop);
    bool getLooping();

    void seekToPercent(float percentPosition);
    void deinit();

    void setListener(GstPlayerListener *listener);

    void setVideoTestPattern();
    void setVideo(std::string pathToFile);

    Texture getTexture();
    float getPercentage();
    int getWidth() { return m_width; };
    int getHeight() { return m_height; };
    bool isPrerollDone() { return m_isPrerollDone; };

protected:
    static GstFlowReturn onNewSample(GstElement *appsink, gpointer data);
    static GstPadProbeReturn onQuery(GstPad *pad, GstPadProbeInfo *info, gpointer data);
    static GstPadProbeReturn onEvent(GstPad *pad, GstPadProbeInfo *info, gpointer data);
    static gboolean onBusMessage(GstBus *bus, GstMessage *msg, gpointer data);

    void init();
    void reset();

    void notifyNewFrame();
    void notifyPrerollDone();

private:
    std::string m_pipelineCommand;
    GstElement *m_pipeline;
    GstBus *m_bus;
    GstGLDisplayEGL *m_gstDisplay;
    GstGLContext *m_glContext;
    bool m_isPrerollDone;
    bool m_initialized;

    std::mutex m_bufferLock;
    GstBuffer *m_bufferLast;
    GstBuffer *m_bufferRender;
    Texture m_texture;

    bool m_looping;
    gint m_width;
    gint m_height;

    static GstLib m_gst;
    GstPlayerListener *m_listener;
};