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

#define LOG_TAG "MiniPlayer"
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#define ALOGV(formate, ...) {__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGD(formate, ...) {__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGI(formate, ...) {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGW(formate, ...) {__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGE(formate, ...) {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%s %s:%d]\t" formate, __FUNCTION__, __FILENAME__, __LINE__, ##__VA_ARGS__);}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_miniplayer_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    hello += avcodec_configuration();
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_miniplayer_MainActivity_Open(JNIEnv *env, jobject thiz, jstring path) {
    // TODO: implement Open()
    const char *_path = env->GetStringUTFChars(path, 0);
    FILE* fstream = fopen(_path,"r");
    if(fstream==NULL) {
        ALOGD("MainActivity Open %s failed, %s\n", _path, strerror(errno));
    }
    else {
        ALOGD("MainActivity Open file %s succeed!\n", _path);
    }
    if(fstream)
        fclose(fstream);
    return true;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_miniplayer_MiniPlayer_Open(JNIEnv *env, jobject thiz, jstring path, jobject surface) {
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
    ANativeWindow_Buffer wbuf;
    ANativeWindow *native_window = ANativeWindow_fromSurface(env, surface);
    int curr_w = ANativeWindow_getWidth(native_window);
    int curr_h = ANativeWindow_getHeight(native_window);
    int curr_format = ANativeWindow_getFormat(native_window);
    int32_t re = ANativeWindow_setBuffersGeometry(native_window, curr_w, curr_h, WINDOW_FORMAT_RGBA_8888);

    ALOGD("curr_w:%d curr_h:%d curr_format:0x%x re:%d nWindow:%p\n", curr_w, curr_h, curr_format, re, native_window);
    while (1) {
        int32_t re1 = ANativeWindow_lock(native_window, &wbuf, 0);
        uint32_t *screen = (uint32_t *) wbuf.bits;
        for (int i = 0; i < curr_w * curr_h; i++) {
            screen[i] = 0x000000ff;
        }
        int32_t re2 = ANativeWindow_unlockAndPost(native_window);
        //ALOGD("post re1:%d re2:%d\n", re1, re2);
    }


    return true;
}