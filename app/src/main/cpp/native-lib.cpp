#include <jni.h>
#include <string>
#include <stdio.h>
#include <errno.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <unistd.h>

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

ANativeWindow *native_window = nullptr;
const char *_path = nullptr;
jobject _surface;

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    av_jni_set_java_vm(vm, NULL);
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1Open(JNIEnv *env, jobject thiz, jstring path, jobject surface) {
    // TODO: implement Open()
    const char *_path = env->GetStringUTFChars(path, 0);
    FILE *fstream = fopen(_path, "r");
    if (fstream == NULL) {
        ALOGD("Open %s failed, %s\n", _path, strerror(errno));
    } else {
        ALOGD("Open %s success!\n", _path);
    }
    if (fstream)
        fclose(fstream);


    return true;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1setDataSource(JNIEnv *env, jobject thiz, jstring path) {
    // TODO: implement native_setDataSource()
    const char *tpath = env->GetStringUTFChars(path, 0);
    _path = av_strdup(tpath);
    ALOGD("path:%s\n", _path);
    env->ReleaseStringUTFChars(path, tpath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1setSurface(JNIEnv *env, jobject thiz, jobject surface) {
    // TODO: implement native_setSurface()
    jobject _surface = env->NewGlobalRef(surface);
    //_surface = surface;
    native_window = ANativeWindow_fromSurface(env, surface);
    ALOGD("native_window:%p _surface:%p\n", native_window, _surface);
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

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_miniplayer_MiniPlayer_native_1start(JNIEnv *env, jobject thiz) {
    //注册所有编解码器，解复用器，初始化网络
    av_log_set_level(AV_LOG_DEBUG);
    av_log_set_callback(ffp_log_callback_report);
    av_register_all();
    avformat_network_init();

    AVFormatContext *ic = NULL;
    int ret = avformat_open_input(&ic, _path, NULL, NULL);

    if (0 == ret) {
        ALOGD("avformat_open_input %s success!", _path);
    } else {
        ALOGD("avformat_open_input failed! %s %s", _path, av_err2str(ret));
        return false;
    }
    //查找音视频信息
    ret = avformat_find_stream_info(ic, NULL);
    av_dump_format(ic, 0, _path, 0);

    //获取音视频stream id并打印相应信息
    //使用循环遍历获取音视频stream index
    int videoStream = 0;
    int audioStream = 0;
    for (int i = 0; i < ic->nb_streams; i++) {
        AVStream *stream = ic->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            //ALOGD("video stream, fps:%d width:%d height:%d codec_type:%d pixformat:%d",
            //(int)a2d(stream->avg_frame_rate),
            //stream->codecpar->width,
            //stream->codecpar->height,
            //stream->codecpar->codec_type,
            //stream->codecpar->format);
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            //ALOGD("audio stream, sample_rate:%d channel:%d sample_format:%d",
            //stream->codecpar->sample_rate,
            //stream->codecpar->channels,
            //stream->codecpar->format);
        } else {
            //ALOGD("unkown stream type!");
        }
    }
    //使用av_find_best_stream获取音视频stream index
    audioStream = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    //打开视频解码器
    ALOGD("video codec_id:%d", ic->streams[videoStream]->codecpar->codec_id);
    //AVCodec *vcodec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);
    AVCodec *vcodec = avcodec_find_decoder_by_name("hevc_mediacodec");
    if (!vcodec) {
        ALOGD("avcodec_find_decoder video failed");
        return 0;
    }

    //初始化解码器上下文
    AVCodecContext *vc = avcodec_alloc_context3(vcodec);
    avcodec_parameters_to_context(vc, ic->streams[videoStream]->codecpar);
    //AVMediaCodecContext * media_codec_ctx = av_mediacodec_alloc_context();
   // av_mediacodec_default_init(vc, media_codec_ctx, _surface);
    vc->thread_count = 8;
    //打开解码器
    ret = avcodec_open2(vc, NULL, NULL);  //AVCodec->init()
    if (0 != ret) {
        ALOGD("avcodec_open2 video failed");
        return 0;
    }


    //打开音频解码器
    AVCodecContext *ac = NULL;
    if (audioStream > 0) {
        AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
        if (!acodec) {
            ALOGD("avcodec_find_decoder audio failed");
            return 0;
        }

        //初始化解码器上下文
        AVCodecContext *tac = avcodec_alloc_context3(acodec);
        avcodec_parameters_to_context(tac, ic->streams[audioStream]->codecpar);
        tac->thread_count = 1;
        //打开解码器
        ret = avcodec_open2(tac, NULL, NULL);
        if (0 != ret) {
            ALOGD("avcodec_open2 audio failed");
            return 0;
        }
        ac = tac;
    }

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int vframeCount = 0;
    SwsContext *vctx = NULL;
    char *rgb = (char *) malloc(19200 * 1080 * 4);
    ANativeWindow_Buffer wbuf;
//    ANativeWindow_setBuffersGeometry(native_window, curr_w, curr_h, WINDOW_FORMAT_RGBA_8888);
//    ANativeWindow_lock(native_window, &wbuf, 0);
//    ANativeWindow_unlockAndPost(native_window);
    for (;;) {
        ret = av_read_frame(ic, pkt);
        ALOGD("pkt.size:%d\n", pkt->size);


        if (0 != ret) {
            ALOGD("file end!");
            break;
            int64_t pos = 2 *
                          ic->streams[videoStream]->time_base.den;//a2d(ic->streams[videoStream]->time_base);
            //向后找关键帧
            ret = av_seek_frame(ic, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
            if (ret < 0) {
                ALOGD("av_seek_frame failed");
                break;
            }

        }

        AVCodecContext *cc = vc;
        if (pkt->stream_index == audioStream && audioStream > 0)
            //cc = ac;
            continue;

        //发送到线程中处理，pkt会被复制一份
        ret = avcodec_send_packet(cc, pkt);
        av_packet_unref(pkt);

        if (ret != 0) {
//            if (cc == vc) ALOGD("avcodec_send_packet video failed!");
//            else
//            ALOGD("avcodec_send_packet audio failed!");
            continue;
        }

        for (;;) {
            ret = avcodec_receive_frame(cc, frame);
            if (ret != 0) {
                break;
            }
            if (cc == vc) {

               // av_mediacodec_release_buffer((AVMediaCodecBuffer *)frame->data[3], 1);

                vframeCount++;
                static int init_once = 1;

                //ALOGD("avcodec_receive_frame video");
                ALOGD("sws_getCachedContext before, width:%d height:%d format:%d", frame->width, frame->height, frame->format);

                vctx = sws_getCachedContext(vctx,
                                            frame->width,
                                            frame->height,
                                            (AVPixelFormat) frame->format,
                                            frame->width,
                                            frame->height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_FAST_BILINEAR,
                                            0, 0, 0);
                ALOGD("sws_getCachedContext after");
                if (!vctx) {
                    ALOGD("sws_getCachedContext failed!");
                    return false;
                } else {
                    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
                    data[0] = (uint8_t *) rgb;
                    int lines[AV_NUM_DATA_POINTERS] = {0};
                    lines[0] = frame->width * 4;
                    ALOGD("sws_scale before");
                    int h = sws_scale(vctx,
                                      frame->data,
                                      frame->linesize, 0,
                                      frame->height,
                                      data, lines);
                    ALOGD("sws_scale after h:%d", h);
                    ANativeWindow_setBuffersGeometry(native_window, frame->width, frame->height, WINDOW_FORMAT_RGBA_8888);
                    ANativeWindow_lock(native_window, &wbuf, 0);
//    ANativeWindow_unlockAndPost(native_window);
                    for(int i = 0; i < frame->height; i++) {
                        memcpy((uint8_t *)wbuf.bits + i*wbuf.stride*4, rgb+i*frame->width*4, frame->width * 4);
                    }
                    ANativeWindow_unlockAndPost(native_window);
                    usleep(30000);
                    ALOGD("vframeCount = %d", vframeCount);

                }


            } else {
                //ALOGD("avcodec_receive_frame audio");
            }
        }

    }
    free(rgb);
    return true;
}