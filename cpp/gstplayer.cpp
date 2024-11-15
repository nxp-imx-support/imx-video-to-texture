/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gstplayer.hpp"
#include <gstimxcommon.h>
#include <stdexcept>

namespace {
constexpr std::string_view DefaultPipeline =
        "playbin uri=testbin://video,pattern=videotestsrc,caps=[video/x-raw,format=BGRA]";
} // namespace

GstLib::GstLib()
{
    gst_init(nullptr, nullptr);
}

GstLib::~GstLib()
{
    gst_deinit();
}

GstLib GstPlayer::m_gst;

/**************************************************************************************************************
 *
 * @brief  			GstPlayer Class
 *
 * @remarks 		Class to maintain and control the gstreamer media pipeline for the application
 *
 **************************************************************************************************************/

GstPlayer::GstPlayer(EGLDisplay eglDisplay, EGLContext eglContext)
    : m_pipeline(nullptr),
      m_bus(nullptr),
      m_gstDisplay(nullptr),
      m_glContext(nullptr),
      m_isPrerollDone(false),
      m_initialized(false),
      m_bufferLast(nullptr),
      m_bufferRender(nullptr),
      m_looping(false),
      m_width(-1),
      m_height(-1),
      m_listener(nullptr)
{
    m_pipelineCommand = std::string(DefaultPipeline);
    m_texture.id = (guint)-1;
    m_texture.target = (guint)-1;

    // Get EGL display and context
    m_gstDisplay = gst_gl_display_egl_new_with_egl_display(eglDisplay);
    m_glContext = gst_gl_context_new_wrapped(GST_GL_DISPLAY(m_gstDisplay),
                                             reinterpret_cast<guintptr>(eglContext),
                                             GST_GL_PLATFORM_EGL, GST_GL_API_GLES2);
}

GstPlayer::~GstPlayer()
{
    deinit();
}

void GstPlayer::play()
{
    if (m_initialized) {
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    }
}

void GstPlayer::pause()
{
    if (m_initialized) {
        gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    }
}

void GstPlayer::toggleLooping()
{
    m_looping = !m_looping;
}

void GstPlayer::setLooping(bool loop)
{
    m_looping = loop;
}

bool GstPlayer::getLooping()
{
    return m_looping;
}

void GstPlayer::skip(int n_sec)
{
    if (m_initialized) {
        // get current pipeline position
        gint64 current_position = -1;
        gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &current_position);
        // get pipeline duration
        gint64 video_duration = -1;
        gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &video_duration);
        // calculate position to seek
        gint64 position = current_position + (n_sec * GST_SECOND); // position = current + n_sec
        GstSeekFlags flags = GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT);

        if (position < 0) {
            position = 0;
        }
        if (position >= video_duration) {
            position = video_duration - (300 * GST_MSECOND);
        }

        gst_element_seek_simple(m_pipeline, GST_FORMAT_TIME, flags, position);
    }
}

void GstPlayer::seekToPercent(float percent)
{
    if (m_initialized) {
        gint64 duration, position;
        if (gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &duration) && duration > 0) {
            position = (gint64)(duration * percent);
            gst_element_seek_simple(m_pipeline, GST_FORMAT_TIME,
                                    (GstSeekFlags)(GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_FLUSH),
                                    position);
        }
    }
}

void GstPlayer::setListener(GstPlayerListener *listener)
{
    m_listener = listener;
}

void GstPlayer::setVideoTestPattern()
{
    m_pipelineCommand = std::string(DefaultPipeline);
    reset();
}

void GstPlayer::setVideo(std::string pathToFile)
{
    pathToFile = "\"" + pathToFile + "\"";
    m_pipelineCommand = "playbin uri=" + pathToFile;
    reset();
}

float GstPlayer::getPercentage()
{
    float percentage = 0.0f;
    if (m_initialized) {
        gint64 current_position = -1;
        gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &current_position);
        // get pipeline duration
        gint64 video_duration = -1;
        gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &video_duration);

        percentage = ((float)(GST_TIME_AS_MSECONDS(current_position)))
                / ((float)(GST_TIME_AS_MSECONDS(video_duration)));
    }
    return percentage;
}

GstPlayer::Texture GstPlayer::getTexture()
{
    m_texture.id = (guint)-1;
    m_texture.target = (guint)-1;

    if (m_initialized == true) {
        m_bufferLock.lock();
        if (m_bufferRender != nullptr && m_bufferLast != m_bufferRender) {
            // New buffer available, release previously rendered buffer
            gst_buffer_unref(m_bufferRender);
        }
        m_bufferRender = m_bufferLast;
    
        if (m_bufferRender != nullptr) {
            // Get OpenGL texture ID
            GstMemory *memory = gst_buffer_peek_memory(m_bufferRender, 0);
            if (gst_is_gl_memory(memory) != 0) {
                m_texture.id = (reinterpret_cast<GstGLMemory *>(memory))->tex_id;
                m_texture.target = gst_gl_texture_target_to_gl(
                        (reinterpret_cast<GstGLMemory *>(memory))->tex_target);
            } else {
                throw std::runtime_error(
                        "Input from appsink is not an OpenGL texture. Consider using "
                        "glupload in the pipeline.");
            }
        }
        m_bufferLock.unlock();
    }

    return m_texture;
}

GstFlowReturn GstPlayer::onNewSample(GstElement *appsink, gpointer data)
{
    auto *ctx = static_cast<GstPlayer *>(data);

    //  Get buffer from GStreamer
    GstSample *sample = nullptr;
    g_signal_emit_by_name(appsink, "pull-sample", &sample);

    if (sample != nullptr) {
        ctx->m_bufferLock.lock();

        GstBuffer *buffer = gst_sample_get_buffer(sample);

        if (ctx->m_bufferLast != nullptr && ctx->m_bufferLast != ctx->m_bufferRender) {
            // Previous stored buffer has not been rendered, release it.
            gst_buffer_unref(ctx->m_bufferLast);
        }
        // Lock new buffer
        ctx->m_bufferLast = gst_buffer_ref(buffer);

        gst_sample_unref(sample);

        ctx->m_bufferLock.unlock();

        ctx->notifyNewFrame();
    }

    return GST_FLOW_OK;
}

GstPadProbeReturn GstPlayer::onQuery(GstPad *pad, GstPadProbeInfo *info, gpointer data)
{
    auto *ctx = static_cast<GstPlayer *>(data);
    GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_CONTEXT:
        if (gst_gl_handle_context_query(ctx->m_pipeline, query,
                                        reinterpret_cast<GstGLDisplay *>(ctx->m_gstDisplay),
                                        nullptr, ctx->m_glContext)
            != 0) {
            return GST_PAD_PROBE_HANDLED;
        }
        break;
    default:
        break;
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn GstPlayer::onEvent(GstPad *pad, GstPadProbeInfo *info, gpointer data)
{
    auto *ctx = static_cast<GstPlayer *>(data);
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);

    if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
        GstCaps *caps;
        gst_event_parse_caps(event, &caps);

        GstStructure *properties = gst_caps_get_structure(caps, 0);
        if (gst_structure_get_int(properties, "width", &ctx->m_width) != 0
            && gst_structure_get_int(properties, "height", &ctx->m_height) != 0) {
            return GST_PAD_PROBE_HANDLED;
        } else {
            g_print("GStreamer Error: Could not find stream dimensions\n");
        }
    }

    return GST_PAD_PROBE_OK;
}

gboolean GstPlayer::onBusMessage(GstBus *bus, GstMessage *msg, gpointer data)
{
    auto *ctx = static_cast<GstPlayer *>(data);

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
        // Restart pipeline
        if (ctx->m_looping) {
            if (!gst_element_seek(ctx->m_pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                                  GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
                g_print("GStreamer Error: Failed to restart pipeline\n");
            }
        }

        break;
    case GST_MESSAGE_ERROR: {
        gchar *debug = nullptr;
        GError *error = nullptr;

        gst_message_parse_error(msg, &error, &debug);

        g_print("GStreamer Error message: %s\n", error->message);
        g_error_free(error);

        if (debug != nullptr) {
            g_print("GStreamer Debug message: %s\n", debug);
            g_free(debug);
        }

        break;
    }
    case GST_MESSAGE_WARNING: {
        gchar *debug = nullptr;
        GError *error = nullptr;

        gst_message_parse_warning(msg, &error, &debug);

        g_print("GStreamer Warning message: %s\n", error->message);
        g_error_free(error);

        if (debug != nullptr) {
            g_print("GStreamer Debug message: %s\n", debug);
            g_free(debug);
        }
        break;
    }
    case GST_MESSAGE_ASYNC_DONE: {
        if (ctx->m_isPrerollDone == false) {
            ctx->m_isPrerollDone = true;
            ctx->notifyPrerollDone();
        }
        break;
    }
    default:
        break;
    }

    return TRUE;
}

void GstPlayer::init()
{
    // Launch pipeline
    GError *error = nullptr;
    g_print("Loading GStreamer pipeline: %s\n", m_pipelineCommand.data());
    m_pipeline = gst_parse_launch(m_pipelineCommand.data(), &error);
    if (error != nullptr) {
        g_print("gst_parse_launch fails with error: %s\n", error->message);
        g_clear_error(&error);
        throw std::runtime_error("Failed to load GStreamer pipeline");
    }

    // Watch bus
    m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
    gst_bus_add_watch(m_bus, GstPlayer::onBusMessage, static_cast<gpointer>(this));

    // Create a bin for video-sink containing glupload and appsink elements
    // Input of appsink must be an OpenGL texture, so the pipeline must contain glupload element.
    GstElement *bin = gst_bin_new("video_sink_bin");
    GstElement *glupload = gst_element_factory_make("glupload", "glupload");
    GstElement *sink = gst_element_factory_make("appsink", "GstPlayerSink");
    gst_bin_add_many(GST_BIN(bin), glupload, sink, NULL);
    gst_element_link(glupload, sink);

    // Use imxvideoconvert_g2d with Amphion VPU
    GstPad *pad;
    if (IS_AMPHION() == true) {
        GstElement *imxvideoconvert =
                gst_element_factory_make("imxvideoconvert_g2d", "imxvideoconvert_g2d");
        gst_bin_add(GST_BIN(bin), imxvideoconvert);
        gst_element_link(imxvideoconvert, glupload);
        pad = gst_element_get_static_pad(imxvideoconvert, "sink");
    } else {
        pad = gst_element_get_static_pad(glupload, "sink");
    }

    // Create ghost pad from first element's pad to connect the video_sink_bin to the rest of the
    // pipeline.
    GstPad *ghostPad;
    ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, TRUE);
    gst_element_add_pad(bin, ghostPad);
    gst_object_unref(GST_OBJECT(pad));

    g_object_set(GST_OBJECT(m_pipeline), "video-sink", bin, NULL);

    // Enable sink's signals emission.
    g_object_set(sink, "emit-signals", TRUE, nullptr);

    // Call onNewSample() every time the sink receives a buffer.
    g_signal_connect(G_OBJECT(sink), "new-sample", G_CALLBACK(GstPlayer::onNewSample),
                     static_cast<gpointer>(this));

    // Get notify on state changed of sink's pad.
    GstPad *sinkPad = gst_element_get_static_pad(sink, "sink");
    gst_pad_add_probe(sinkPad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, GstPlayer::onQuery,
                      static_cast<gpointer>(this), nullptr);
    gst_pad_add_probe(sinkPad, (GstPadProbeType)(GST_PAD_PROBE_TYPE_EVENT_BOTH), GstPlayer::onEvent,
                      static_cast<gpointer>(this), nullptr);

    // Start pipeline
    GstStateChangeReturn stateReturn = gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    if (stateReturn == GST_STATE_CHANGE_FAILURE) {
        throw std::runtime_error("Failed to play GStreamer pipeline");
    }

    m_initialized = true;
}

void GstPlayer::deinit()
{
    if (m_initialized) {
        GstStateChangeReturn stateReturn = gst_element_set_state(m_pipeline, GST_STATE_NULL);
        if (stateReturn == GST_STATE_CHANGE_FAILURE) {
            throw std::runtime_error("Failed to deinit GStreamer pipeline");
        }

        // Release buffers
        m_bufferLock.lock();
        if (m_bufferLast != nullptr && m_bufferLast != m_bufferRender) {
            gst_buffer_unref(m_bufferLast);
        }
        m_bufferLast = nullptr;
        if (m_bufferRender != nullptr) {
            gst_buffer_unref(m_bufferRender);
            m_bufferRender = nullptr;
        }
        m_bufferLock.unlock();

        m_texture.id = (guint)-1;
        m_texture.target = (guint)-1;
        gst_bus_remove_watch(m_bus);
        gst_object_unref(GST_OBJECT(m_bus));
        gst_object_unref(GST_OBJECT(m_pipeline));
        m_initialized = false;
        m_isPrerollDone = false;
    }
}

void GstPlayer::reset()
{
    deinit();
    init();
}

void GstPlayer::notifyNewFrame()
{
    if (m_listener != nullptr) {
        m_listener->onNewFrame();
    }
}

void GstPlayer::notifyPrerollDone()
{
    if (m_listener != nullptr) {
        m_listener->onPrerollDone();
    }
}
