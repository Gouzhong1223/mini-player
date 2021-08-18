#include <jni.h>
#include <string>
#include <stdio.h>
#include <errno.h>
#include <android/log.h>

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
        ALOGD("Open %s failed, %s\n", _path, strerror(errno));
        //exit(1);
    }
    else {
        ALOGD("open file test.txt succeed!\n");
    }
    if(fstream)
        fclose(fstream);
    return true;
}