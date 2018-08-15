#include <jni.h>

#include <string>
#include <thread>
#include <array>
#include <cstdlib>

#include <android/native_window_jni.h>
#include <android/log.h>

#include "librealsense2/rs.hpp"

class RealSenseWrapper {
public:
    RealSenseWrapper() {
        //rs2::log_to_file(rs2_log_severity::RS2_LOG_SEVERITY_DEBUG, "/sdcard/Download/rs2.log");
        rs2::log_to_console(rs2_log_severity::RS2_LOG_SEVERITY_WARN);

        for (auto it = stream_window_map.begin(); it != stream_window_map.end(); ++it) {
            *it = nullptr;
        }

        std::stringstream ss;

        ctx = std::make_shared<rs2::context>();

        auto list = ctx->query_devices();

        __android_log_print(ANDROID_LOG_DEBUG, "RS NATIVE", "%d\n", list.size());

        rs2::device dev = list.front();
        sensors = dev.query_sensors();

        log = ss.str();

        openStereo = true;
        //pipe = std::make_shared<rs2::pipeline>(*ctx);
        //cfg = std::make_shared<rs2::config>();

    }
    ~RealSenseWrapper() {
        Stop();
        //pipe.reset();
        //cfg.reset();
        ctx.reset();
        sensors.clear();
    }

    void EnableStream(int stream, int width, int height, int fps, int format, ANativeWindow * surface) {
        bool profileFound = false;
        for (rs2::sensor sensor : sensors) {
            __android_log_print(ANDROID_LOG_DEBUG, "RS NATIVE", "%s\n", sensor.get_info(RS2_CAMERA_INFO_NAME));
            std::vector<rs2::stream_profile> stream_profiles = sensor.get_stream_profiles();
            for (rs2::stream_profile stream_profile : stream_profiles)
            {
                rs2_stream stream_data_type = stream_profile.stream_type();
                if (stream_data_type != stream) {
                    continue;
                }
                int stream_index = stream_profile.stream_index();
                std::string stream_name = stream_profile.stream_name();
                int unique_stream_id = stream_profile.unique_id();
                if (stream_profile.is<rs2::video_stream_profile>())
                {
                    rs2::video_stream_profile video_stream_profile = stream_profile.as<rs2::video_stream_profile>();
                    std::ostringstream sout;
                    sout << " (Video Stream: " << video_stream_profile.format() << " " <<
                              video_stream_profile.width() << "x" << video_stream_profile.height() << "@ " << video_stream_profile.fps() << "Hz)";
                    __android_log_print(ANDROID_LOG_DEBUG, "RS NATIVE", "%s\n", sout.str().c_str());
                    if (stream_data_type == stream && video_stream_profile.format() == format && video_stream_profile.width() == width && video_stream_profile.height() == height && video_stream_profile.fps() == fps) {
                        if (stream == RS2_STREAM_COLOR) {
                            profileColor.push_back(stream_profile);
                        } else {
                            profileStereo.push_back(stream_profile);
                        }
                        profileFound = true;
                        break;
                    }
                }
            }
        }
        stream_window_map[stream] = surface;
        ANativeWindow_setBuffersGeometry(surface, width, height, WINDOW_FORMAT_RGBX_8888);
    }

    int Play() {
        isRunning = true;

        worker = std::thread(&RealSenseWrapper::FrameLoop, this);

        return 1;
    }

    void OpenSensor0() {
        if (mtx.try_lock()) {
            if (openStereo == false) {
                openStereo = true;
                if (profileStereo.size() > 0) {
                    sensors[0].open(profileStereo);
                }
                __android_log_print(ANDROID_LOG_DEBUG, "RS NATIVE", "Sensor opened.");

                if (profileStereo.size() > 0) {
                    sensors[0].start(
                            [&](rs2::frame frame) {
                                if (frame.get_profile().stream_type() ==
                                    rs2_stream::RS2_STREAM_DEPTH) {
                                    framesDepth.enqueue(frame);
                                } else {
                                    framesIR.enqueue(frame);
                                }
                            }
                    );
                }
                __android_log_print(ANDROID_LOG_DEBUG, "RS NATIVE", "Sensor started.");
            }
            mtx.unlock();
        }
    }

    void CloseSensor0() {
        if (mtx.try_lock()) {
            if (openStereo == true) {
                if (profileStereo.size() > 0) {
                    sensors[0].stop();
                }
                __android_log_print(ANDROID_LOG_DEBUG, "RS NATIVE", "Sensor stopped.");
                //std::this_thread::sleep_for(std::chrono::seconds(1));
                if (profileStereo.size() > 0) {
                    sensors[0].close();
                }
                __android_log_print(ANDROID_LOG_DEBUG, "RS NATIVE", "Sensor closed.");

                openStereo = false;
            }
            mtx.unlock();
        }
    }

    void Stop() {
        if (mtx.try_lock()) {
            isRunning = false;
            if (worker.joinable()) worker.join();
            if (openStereo && profileStereo.size() > 0) {
                sensors[0].stop();
            }

            if (profileColor.size() > 0) {
                sensors[1].stop();
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (openStereo && profileStereo.size() > 0) {
                sensors[0].close();
            }

            if (profileColor.size() > 0) {
                sensors[1].close();
            }

            int streams = 0;
            for (auto profile : profileColor) {
                auto s = profile.stream_type();
                streams |= (1 << s);
            }
            for (auto profile : profileStereo) {
                auto s = profile.stream_type();
                streams |= (1 << s);
            }

            if ((streams & (1 << rs2_stream::RS2_STREAM_DEPTH))) {
                ANativeWindow_release(stream_window_map[rs2_stream::RS2_STREAM_DEPTH]);
            }
            if ((streams & (1 << rs2_stream::RS2_STREAM_COLOR))) {
                ANativeWindow_release(stream_window_map[rs2_stream::RS2_STREAM_COLOR]);
            }
            if ((streams & (1 << rs2_stream::RS2_STREAM_INFRARED))) {
                ANativeWindow_release(stream_window_map[rs2_stream::RS2_STREAM_INFRARED]);
            }

            profileStereo.clear();
            profileColor.clear();

            mtx.unlock();
        }
    }

    void FrameLoop() {
        openStereo = true;
        __android_log_print(ANDROID_LOG_DEBUG, "RS native", "FrameLoop BEGIN");
#if 0
        {
            for(auto dev:ctx->query_devices()){
                dev.hardware_reset();
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));

            rs2::device_hub hub(*ctx);
            hub.wait_for_device();
        }
#endif
        //auto stream_profiles = pipe->start(*cfg);

        int streams = 0;
        for (auto profile : profileColor) {
            auto s = profile.stream_type();
            streams |= (1 << s);
        }
        for (auto profile : profileStereo) {
            auto s = profile.stream_type();
            streams |= (1 << s);
        }

        if(profileStereo.size() > 0) {
            sensors[0].open(profileStereo);
            sensors[0].start(
                    [&](rs2::frame frame) {
                        if (frame.get_profile().stream_type() == rs2_stream::RS2_STREAM_DEPTH) {
                            framesDepth.enqueue(frame);
                        } else {
                            framesIR.enqueue(frame);
                        }
                    }
            );
        }
        if(profileColor.size() > 0) {
            sensors[1].open(profileColor);
            sensors[1].start(
                    [&](rs2::frame frame) {
                        framesColor.enqueue(frame);
                    }
            );
        }

        rs2::colorizer colorizer;
        while (isRunning) {
            rs2::frame depth;
            framesDepth.poll_for_frame(&depth);
            if ((streams & (1 << rs2_stream::RS2_STREAM_DEPTH)) && depth) {
                auto colorize_depth = colorizer(depth);
                Draw(stream_window_map[rs2_stream::RS2_STREAM_DEPTH], colorize_depth);
            }

            rs2::frame color;
            framesColor.poll_for_frame(&color);
            if ((streams & (1 << rs2_stream::RS2_STREAM_COLOR)) && color) {
                Draw(stream_window_map[rs2_stream::RS2_STREAM_COLOR], color);
            }
            rs2::frame ir;
            framesIR.poll_for_frame(&ir);
            if ((streams & (1 << rs2_stream::RS2_STREAM_INFRARED)) && ir) {
                Draw(stream_window_map[rs2_stream::RS2_STREAM_INFRARED], ir);
            }
        }
        __android_log_print(ANDROID_LOG_DEBUG, "RS native", "FrameLoop END");
    }

    void Draw(ANativeWindow* window, rs2::frame& frame) {
        if (!window) return;

        auto vframe = frame.as<rs2::video_frame>();
        auto profile = vframe.get_profile().as<rs2::video_stream_profile>();
        ANativeWindow_Buffer buffer = {};
        ARect rect = {0, 0, profile.width(), profile.height()};

        if (ANativeWindow_lock(window, &buffer, &rect) == 0) {

            if (profile.format() == rs2_format::RS2_FORMAT_RGB8) {
                DrawRGB8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
            } else if (profile.format() == rs2_format::RS2_FORMAT_Y8) {
                DrawY8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
            }

            ANativeWindow_unlockAndPost(window);
        } else {
            __android_log_print(ANDROID_LOG_DEBUG, "RS NATIVE", "ANativeWindow_lock FAILED");
        }
    }

    void DrawRGB8(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 3 + 0];
                out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 3 + 1];
                out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 3 + 2];
                out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
            }
        }
    }

    void DrawY8(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_width + x];
                out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_width + x];
                out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_width + x];
                out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
            }
        }
    }

    std::string& GetLogMessage() {
        return log;
    }
private:
    std::shared_ptr<rs2::context> ctx;
    //std::shared_ptr<rs2::pipeline> pipe;
    //std::shared_ptr<rs2::config> cfg;

    std::array<ANativeWindow*, rs2_stream::RS2_STREAM_COUNT> stream_window_map;

    std::thread worker;
    bool isRunning;
    std::string log;

    std::vector<rs2::sensor> sensors;

    std::vector<rs2::stream_profile> profileStereo;
    std::vector<rs2::stream_profile> profileColor;

    rs2::frame_queue framesIR;
    rs2::frame_queue framesDepth;
    rs2::frame_queue framesColor;

    std::mutex mtx;

    bool openStereo;
public:
    static RealSenseWrapper * GetInstance() {
        return instance.get();
    }
    static void Init() {
        if (!instance) {
            instance = std::make_shared<RealSenseWrapper>();
        }
    }
    static void Cleanup() {
        if (instance) {
            instance.reset();
        }
    }
private:
    static std::shared_ptr<RealSenseWrapper> instance;
};

std::shared_ptr<RealSenseWrapper> RealSenseWrapper::instance;

JNIEXPORT jstring JNICALL
Java_com_example_realsense_1app_rs2_StreamToString(JNIEnv *env, jclass type, jint s) {

    const char * returnValue = "NULL";
    rs2_stream stream = static_cast<rs2_stream>(s);
    returnValue = rs2_stream_to_string(stream);
    return env->NewStringUTF(returnValue);
}

JNIEXPORT jstring JNICALL
Java_com_example_realsense_1app_rs2_FormatToString(JNIEnv *env, jclass type, jint f) {

    const char * returnValue = "NULL";
    rs2_format format = static_cast<rs2_format>(f);
    returnValue = rs2_format_to_string(format);
    return env->NewStringUTF(returnValue);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_realsense_1app_rs2_StreamFromString(JNIEnv *env, jclass type, jstring s_) {
    const char *s = env->GetStringUTFChars(s_, 0);

    int returnValue = -1;

    for (int i = rs2_stream::RS2_STREAM_ANY; i < rs2_stream::RS2_STREAM_COUNT; ++i) {
        if (strcmp(s, rs2_stream_to_string(static_cast<rs2_stream>(i))) == 0) {
            returnValue = i;
            break;
        }
    }

    env->ReleaseStringUTFChars(s_, s);

    return returnValue;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_realsense_1app_rs2_FormatFromString(JNIEnv *env, jclass type, jstring f_) {
    const char *f = env->GetStringUTFChars(f_, 0);

    int returnValue = -1;

    for (int i = rs2_format::RS2_FORMAT_ANY; i < rs2_format::RS2_FORMAT_COUNT; ++i) {
        if (strcmp(f, rs2_format_to_string(static_cast<rs2_format>(i))) == 0) {
            returnValue = i;
            break;
        }
    }

    env->ReleaseStringUTFChars(f_, f);

    return returnValue;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_init(JNIEnv *env, jclass type) {

    RealSenseWrapper::Init();

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_cleanup(JNIEnv *env, jclass type) {

    RealSenseWrapper::Cleanup();

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_enableStream(JNIEnv *env, jclass type,
                                                          jint stream,
                                                          jint width, jint height, jint fps,
                                                          jint format, jobject surface) {
    __android_log_print(ANDROID_LOG_DEBUG, "RS native", "MainActivity_enableStream %d, %d %d %d, %d", stream, width, height, fps, format);

    ANativeWindow* _surface = ANativeWindow_fromSurface(env, surface);
    __android_log_print(ANDROID_LOG_DEBUG, "RS native", "ANativeWindow_fromSurface %d", (int)(long)_surface);

    RealSenseWrapper::GetInstance()->EnableStream(
            stream, width, height, fps, format, _surface
    );
    __android_log_print(ANDROID_LOG_DEBUG, "RS native", "MainActivity_enableStream END");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_play(JNIEnv *env, jclass type) {

    __android_log_print(ANDROID_LOG_DEBUG, "RS native", "MainActivity_play BEGIN");
    RealSenseWrapper::GetInstance()->Play();
    __android_log_print(ANDROID_LOG_DEBUG, "RS native", "MainActivity_play END");

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_stop(JNIEnv *env, jclass type) {
    RealSenseWrapper::GetInstance()->Stop();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_realsense_1app_MainActivity_logMessage(JNIEnv *env, jclass type) {

    std::string& log = RealSenseWrapper::GetInstance()->GetLogMessage();
    return env->NewStringUTF(log.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_closeSensor0(JNIEnv *env, jclass type) {
    RealSenseWrapper::GetInstance()->CloseSensor0();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_openSensor0(JNIEnv *env, jclass type) {
    RealSenseWrapper::GetInstance()->OpenSensor0();
}