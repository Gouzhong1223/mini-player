#include <jni.h>
#include <string>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <unistd.h>

#include "media/NdkMediaCodec.h"
#include "media/NdkMediaExtractor.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/jni.h>
#include <libavcodec/mediacodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
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

typedef struct _MiniPlayer {
    ANativeWindow *native_window = nullptr;
    const char *_path = nullptr;
    AMediaCodec *codec = NULL;
    pthread_t decode_thread;
    int loglevel = AV_LOG_INFO;
}MiniPlayer;


extern "C"
JNIEXPORT void JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1setDataSource(JNIEnv *env, jobject thiz, jlong mp, jstring path) {
    // TODO: implement native_setDataSource()
    MiniPlayer *mpp = (MiniPlayer *)mp;
    const char *tpath = env->GetStringUTFChars(path, 0);
    mpp->_path = av_strdup(tpath);
    ALOGD("path:%s\n", mpp->_path);
    env->ReleaseStringUTFChars(path, tpath);
    //AMEDIA_OK
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1setSurface(JNIEnv *env, jobject thiz, jlong mp, jobject jsurface) {
    // TODO: implement native_setSurface()
    //jobject _surface = env->NewGlobalRef(jsurface);
    //_surface = surface;
   // jclass clazz=env->FindClass("android/view/Surface");

   // jfieldID field_surface = env->GetFieldID(clazz, "mNativeObject","J");

   // if(field_surface==NULL) {
   //     ALOGD("field_surface=NULL\n");
   //     return;
   // }

    //_surface = (void*) env->GetIntField(jsurface, field_surface);
    MiniPlayer *mpp = (MiniPlayer *)mp;
    mpp->native_window = ANativeWindow_fromSurface(env, jsurface);
    ALOGD("native_window:%p _surface:%p\n", mpp->native_window);
}

extern "C"
void ffp_log_callback_report(void *ptr, int level, const char *fmt, va_list vl)
{
    if (level > av_log_get_level())
        return;

    va_list vl2;
    char line[1024];
    static int print_prefix = 1;

    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
    va_end(vl2);

    ALOGD("%s", line);
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    av_jni_set_java_vm(vm, NULL);
    //注册所有编解码器，解复用器，初始化网络
    av_log_set_level(AV_LOG_INFO);
    av_log_set_callback(ffp_log_callback_report);
    av_register_all();
    avformat_network_init();
    return JNI_VERSION_1_6;
}

extern "C"
void *decode_thread(void *mp)
{
    MiniPlayer *mpp = (MiniPlayer *)mp;
    AVFormatContext *ic = NULL;
    int ret = avformat_open_input(&ic, mpp->_path, NULL, NULL);

    if (0 == ret) {
        ALOGD("avformat_open_input %s success!", mpp->_path);
    } else {
        ALOGD("avformat_open_input failed! %s %s", mpp->_path, av_err2str(ret));
        return NULL;
    }
    //查找音视频信息
    ret = avformat_find_stream_info(ic, NULL);
    av_dump_format(ic, 0, mpp->_path, 0);

    //使用av_find_best_stream获取音视频stream index
    int audioStream = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    int videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    //打开视频解码器
    AVCodec *vcodec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);
    if (!vcodec) {
        ALOGD("avcodec_find_decoder video failed");
        return NULL;
    }

    //初始化解码器上下文
    AVCodecContext *vc = avcodec_alloc_context3(vcodec);
    avcodec_parameters_to_context(vc, ic->streams[videoStream]->codecpar);
    vc->thread_count = 8;
    //打开解码器
    ret = avcodec_open2(vc, NULL, NULL);  //AVCodec->init()
    if (0 != ret) {
        ALOGD("avcodec_open2 video failed");
        return NULL;
    }

    AVPacket *pkt = av_packet_alloc();
    AMediaCodec *codec = NULL;
    AMediaFormat *format = AMediaFormat_new();
    AMediaFormat_setString(format, "mime", "video/hevc");
    AMediaFormat_setInt32(format, "height", 2160);
    AMediaFormat_setInt32(format, "width", 3840);
    codec = AMediaCodec_createCodecByName("OMX.qcom.video.decoder.hevc");

    //codec = AMediaCodec_createDecoderByType("video/avc");
    ALOGD("codec:%p", codec);
    AMediaCodec_configure(codec, format, mpp->native_window, NULL, 0);
    AMediaCodec_start(codec);

    for (;;) {
        ret = av_read_frame(ic, pkt);

        if (0 != ret) {
            ALOGD("file end!");
            break;
        }

        AVCodecContext *cc = vc;
        if (pkt->stream_index == videoStream) {
            //ALOGD("read video packet");
        }
        else if (pkt->stream_index == audioStream) {
            //ALOGD("read audio packet");
            continue;
        }
        else {
            ALOGD("unkown packet type! size:%d index:%d", pkt->size, pkt->stream_index);
            continue;
        }

        //av_packet_split_side_data(pkt);
        ALOGD("size:%d", pkt->size);
//        ALOGD("split after size:%d", pkt->size);
//        if(pkt->size > 0) {
//            for(int i = pkt->size - 20; i < pkt->size; i++) {
//                ALOGD("data[%d]:%02x", i, pkt->data[i]);
//            }
//        }
        ssize_t bufidx = -1;
        bufidx = AMediaCodec_dequeueInputBuffer(codec, 2000);
        ALOGD("input buffer %zd", bufidx);
        if (bufidx >= 0) {
            size_t bufsize;
            uint8_t* buf = AMediaCodec_getInputBuffer(codec, bufidx, &bufsize);
            //ALOGD("bufsize:%d\n", bufsize);
            memcpy(buf, pkt->data, pkt->size);
            AMediaCodec_queueInputBuffer(codec, bufidx, 0, pkt->size, pkt->pts, 0);
        }

        AMediaCodecBufferInfo info;
        ssize_t status = AMediaCodec_dequeueOutputBuffer(codec, &info, 0);
        if (status >= 0) {
            ALOGD("outindex:%zd", status);
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                ALOGD("output EOS");
            }
            AMediaCodec_releaseOutputBuffer(codec, status, info.size != 0);
            // usleep(1000000);
        } else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            ALOGD("output buffers changed");
        } else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            auto format = AMediaCodec_getOutputFormat(codec);
            ALOGD("format changed to: %s", AMediaFormat_toString(format));
            AMediaFormat_delete(format);
            //while(1);
        } else if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            //ALOGD("no output buffer right now");
        } else {
            //ALOGD("unexpected info code: %zd", status);
        }

        usleep(20000);
    }
    AMediaCodec_delete(codec);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1start(JNIEnv *env, jobject thiz, jlong mp) {
    MiniPlayer *mpp = (MiniPlayer *)mp;

    pthread_create(&mpp->decode_thread, NULL, decode_thread, mpp);

    return true;




//    //打开音频解码器
//    AVCodecContext *ac = NULL;
//    if (audioStream > 0) {
//        AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
//        if (!acodec) {
//            ALOGD("avcodec_find_decoder audio failed");
//            return 0;
//        }
//
//        //初始化解码器上下文
//        AVCodecContext *tac = avcodec_alloc_context3(acodec);
//        avcodec_parameters_to_context(tac, ic->streams[audioStream]->codecpar);
//        tac->thread_count = 1;
//        //打开解码器
//        ret = avcodec_open2(tac, NULL, NULL);
//        if (0 != ret) {
//            ALOGD("avcodec_open2 audio failed");
//            return 0;
//        }
//        ac = tac;
//    }


    //AMediaFormat_setInt32(format, "decoder", 0);
    //AMediaFormat_setInt32(format, "max-input-size", 0);

    //for(int i=0;i < 20; i++)


        //AMediaFormat_getString
//        //发送到线程中处理，pkt会被复制一份
//        ret = avcodec_send_packet(cc, pkt);
//        av_packet_unref(pkt);
//
//        if (ret != 0) {
//            continue;
//        }
//
//        for (;;) {
//            ret = avcodec_receive_frame(cc, frame);
//            if (ret != 0) {
//                break;
//            }
//            if (cc == vc) {
//               // ALOGD("avcodec_receive_frame video");
//                vframeCount++;
//                static int init_once = 1;
//                vctx = sws_getCachedContext(vctx,
//                                            frame->width,
//                                            frame->height,
//                                            (AVPixelFormat) frame->format,
//                                            frame->width,
//                                            frame->height,
//                                            AV_PIX_FMT_RGBA,
//                                            SWS_FAST_BILINEAR,
//                                            0, 0, 0);
//                if (!vctx) {
//                    ALOGD("sws_getCachedContext failed!");
//                    return false;
//                } else {
//                    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
//                    data[0] = (uint8_t *) rgb;
//                    int lines[AV_NUM_DATA_POINTERS] = {0};
//                    lines[0] = frame->width * 4;
//                    int h = sws_scale(vctx,
//                                      frame->data,
//                                      frame->linesize, 0,
//                                      frame->height,
//                                      data, lines);
//                    ANativeWindow_setBuffersGeometry(native_window, frame->width, frame->height, WINDOW_FORMAT_RGBA_8888);
//                    ANativeWindow_lock(native_window, &wbuf, 0);
//                    for(int i = 0; i < frame->height; i++) {
//                        memcpy((uint8_t *)wbuf.bits + i*wbuf.stride*4, rgb+i*frame->width*4, frame->width * 4);
//                    }
//                    ANativeWindow_unlockAndPost(native_window);
//                    usleep(80000);
//                    //ALOGD("vframeCount = %d", vframeCount);
//                }
//            } else {
//               // ALOGD("avcodec_receive_frame audio");
//            }
//        }


//    free(rgb);

    // convert Java string to UTF-8
//    ALOGD("opening %s %d", _path, __ANDROID_API__);
//
//    FILE* testFile = fopen(_path, "r");
//    if(!testFile)
//        ALOGD("file open error");
//    int fd = fileno(testFile);
//    ALOGD("fd:%d", fd);
//
//    off64_t outStart=0, outLen=0;
//    AMediaExtractor *ex = AMediaExtractor_new();
//   // media_status_t err = AMediaExtractor_setDataSource(ex, _path);
//    media_status_t err = AMediaExtractor_setDataSourceFd(ex, fd,
//                                                         static_cast<off64_t>(outStart),
//                                                         static_cast<off64_t>(outLen));
//    close(fd);
//    if (err != AMEDIA_OK) {
//        ALOGD("setDataSource error: %d", err);
//        //return JNI_FALSE;
//    }
//
//    int numtracks = AMediaExtractor_getTrackCount(ex);
//
//
//    ALOGD("input has %d tracks", numtracks);
//    for (int i = 0; i < numtracks; i++) {
//        AMediaFormat *format = AMediaExtractor_getTrackFormat(ex, i);
//        const char *s = AMediaFormat_toString(format);
//        ALOGD("track %d format: %s", i, s);
//        const char *mime;
//        if (!AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime)) {
//            ALOGD("no mime type");
//            return JNI_FALSE;
//        } else if (!strncmp(mime, "video/", 6)) {
//            // Omitting most error handling for clarity.
//            // Production code should check for errors.
//            AMediaExtractor_selectTrack(ex, i);
//            codec = AMediaCodec_createDecoderByType(mime);
//            AMediaCodec_configure(codec, format, native_window, NULL, 0);
//            AMediaCodec_start(codec);
//        }
//        AMediaFormat_delete(format);
//    }


    return true;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1release(JNIEnv *env, jobject thiz, jlong mp) {
    // TODO: implement native_release()
    MiniPlayer *mpp = (MiniPlayer *)mp;
    ANativeWindow_release(mpp->native_window);
}extern "C"

JNIEXPORT void JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1setLoglevel(JNIEnv *env, jobject thiz, jlong mp, jint level) {
    // TODO: implement native_setLoglevel()
    MiniPlayer *mpp = (MiniPlayer *)mp;
    mpp->loglevel = level;
}extern "C"

extern "C"
JNIEXPORT void JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1setup(JNIEnv *env, jobject thiz) {
    // TODO: implement native_setup()
    jlong mp = (jlong)malloc(sizeof(MiniPlayer));
    jclass clazz = env->FindClass("com/example/miniplayer/MiniPlayer");
    //class, member_name, type， "J":long
    jfieldID mNativeMiniPlayer = env->GetFieldID(clazz, "mNativeMiniPlayer", "J");
    env->SetLongField(thiz, mNativeMiniPlayer, mp);
}