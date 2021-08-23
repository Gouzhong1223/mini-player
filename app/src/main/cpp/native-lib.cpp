#include <jni.h>
#include <string>
#include <stdio.h>
#include <errno.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define LOG_TAG "MiniPlayer"
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#define ALOGV(formate, ...) {__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGD(formate, ...) {__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGI(formate, ...) {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGW(formate, ...) {__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGE(formate, ...) {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1Open(JNIEnv *env, jobject thiz, jstring path, jobject surface) {
    // TODO: implement Open()
    const char *_path = env->GetStringUTFChars(path, 0);
    FILE *fstream = fopen(_path, "r");
    if (fstream == NULL) {
        ALOGD("MiniPlayer Open %s failed, %s\n", _path, strerror(errno));
    } else {
        ALOGD("MiniPlayer Open file %s succeed!\n", _path);
    }
    if (fstream)
        fclose(fstream);

    int m_Width=0, m_Height=0, m_Channels=0;
    stbi_set_flip_vertically_on_load(false);
    //uint8_t  *m_data = stbi_load("/storage/emulated/0/vrtest/1919x1080.png", &m_Width, &m_Height, &m_Channels, 0);
    uint8_t  *m_data = stbi_load("/sdcard/video/ww.png", &m_Width, &m_Height, &m_Channels, 0);
    if (!m_data) {
        ALOGD("%s load fail\n", path);
    }
    ALOGD("Width:%d Height:%d Channel:%d\n", m_Width, m_Height, m_Channels);

    ANativeWindow_Buffer wbuf;
    ANativeWindow *native_window = ANativeWindow_fromSurface(env, surface);
    int src_lineSize = m_Width*m_Channels;
    int32_t re = ANativeWindow_setBuffersGeometry(native_window, m_Width, m_Height, WINDOW_FORMAT_RGBA_8888);
    int curr_format = ANativeWindow_getFormat(native_window);
    ALOGD("curr_w:%d curr_h:%d curr_format:0x%x re:%d nWindow:%p\n", m_Width, m_Height, curr_format, re, native_window);

    ANativeWindow_lock(native_window, &wbuf, 0);
    //把buffer中的数据进行赋值（修改）
    uint8_t *dst_data = static_cast<uint8_t *>(wbuf.bits);
    int dst_lineSize = wbuf.stride * m_Channels;//ARGB
    //逐行拷贝
    ALOGD("buffer.height:%d buffer.width:%d buffer.stride:%d dst_ineSize : %d  src_lineSize : %d", wbuf.height, wbuf.width, wbuf.stride, dst_lineSize, src_lineSize);
    for (int i = 0; i < wbuf.height; ++i) {
        memcpy(dst_data + i * dst_lineSize, m_data + i * src_lineSize, src_lineSize);
    }
    ANativeWindow_unlockAndPost(native_window);
    return true;
}
