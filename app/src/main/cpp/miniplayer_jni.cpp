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
#define ALOGV(formate, ...) {__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "[%s:%d]\t" formate, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGD(formate, ...) {__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%s:%d]\t" formate, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGI(formate, ...) {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%s:%d]\t" formate, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGW(formate, ...) {__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%s:%d]\t" formate, __FILENAME__, __LINE__, ##__VA_ARGS__);}
#define ALOGE(formate, ...) {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%s:%d]\t" formate, __FILENAME__, __LINE__, ##__VA_ARGS__);}

static jclass g_class;

typedef struct _MiniPlayer {
    ANativeWindow *native_window = nullptr;
    const char *_path = nullptr;
    const char *logtag = nullptr;
    AMediaCodec *codec = NULL;
    pthread_t decode_thread;
    //java中MiniPlayer对象wake引用
    void *weak_thiz;
}MiniPlayer;

MiniPlayer *get_MiniPlayer(JNIEnv *env, jobject thiz);
void *get_weakthis(JNIEnv *env, jobject thiz);

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

static int convert_sps_pps( const uint8_t *p_buf, size_t i_buf_size,
                            uint8_t *p_out_buf, size_t i_out_buf_size,
                            size_t *p_sps_pps_size, size_t *p_nal_size)
{
    // int i_profile;
    uint32_t i_data_size = i_buf_size, i_nal_size, i_sps_pps_size = 0;
    unsigned int i_loop_end;

    /* */
    if( i_data_size < 7 )
    {
        ALOGE( "Input Metadata too small" );
        return -1;
    }

    /* Read infos in first 6 bytes */
    // i_profile    = (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
    if (p_nal_size)
        *p_nal_size  = (p_buf[4] & 0x03) + 1;
    p_buf       += 5;
    i_data_size -= 5;

    for ( unsigned int j = 0; j < 2; j++ )
    {
        /* First time is SPS, Second is PPS */
        if( i_data_size < 1 )
        {
            ALOGE( "PPS too small after processing SPS/PPS %u",
                   i_data_size );
            return -1;
        }
        i_loop_end = p_buf[0] & (j == 0 ? 0x1f : 0xff);
        p_buf++; i_data_size--;

        for ( unsigned int i = 0; i < i_loop_end; i++)
        {
            if( i_data_size < 2 )
            {
                ALOGE( "SPS is too small %u", i_data_size );
                return -1;
            }

            i_nal_size = (p_buf[0] << 8) | p_buf[1];
            p_buf += 2;
            i_data_size -= 2;

            if( i_data_size < i_nal_size )
            {
                ALOGE( "SPS size does not match NAL specified size %u",
                       i_data_size );
                return -1;
            }
            if( i_sps_pps_size + 4 + i_nal_size > i_out_buf_size )
            {
                ALOGE( "Output SPS/PPS buffer too small" );
                return -1;
            }

            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 1;

            memcpy( p_out_buf + i_sps_pps_size, p_buf, i_nal_size );
            i_sps_pps_size += i_nal_size;

            p_buf += i_nal_size;
            i_data_size -= i_nal_size;
        }
    }

    *p_sps_pps_size = i_sps_pps_size;

    return 0;
}

typedef struct H264ConvertState {
    uint32_t nal_len;
    uint32_t nal_pos;
} H264ConvertState;

static void convert_h264_to_annexb( uint8_t *p_buf, size_t i_len,
                                    size_t i_nal_size,
                                    H264ConvertState *state )
{
    if( i_nal_size < 3 || i_nal_size > 4 )
        return;

    /* This only works for NAL sizes 3-4 */
    while( i_len > 0 )
    {
        if( state->nal_pos < i_nal_size ) {
            unsigned int i;
            for( i = 0; state->nal_pos < i_nal_size && i < i_len; i++, state->nal_pos++ ) {
                state->nal_len = (state->nal_len << 8) | p_buf[i];
                p_buf[i] = 0;
            }
            if( state->nal_pos < i_nal_size )
                return;
            p_buf[i - 1] = 1;
            p_buf += i;
            i_len -= i;
        }
        if( state->nal_len > INT_MAX )
            return;
        if( state->nal_len > i_len )
        {
            state->nal_len -= i_len;
            return;
        }
        else
        {
            p_buf += state->nal_len;
            i_len -= state->nal_len;
            state->nal_len = 0;
            state->nal_pos = 0;
        }
    }
}

//extern "C"
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
    ALOGD("nb_streams:%d\n", ic->nb_streams);

    //使用av_find_best_stream获取音视频stream index
    int audioStream = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    int videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    //打开视频解码器
    AVCodec *vcodec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);
    if (!vcodec) {
        ALOGD("avcodec_find_decoder video failed");
        return NULL;
    }

    // mediacodec csd0未设置
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
    AMediaFormat_setString(format, "mime", "video/avc");
    AMediaFormat_setInt32(format, "height", 720);
    AMediaFormat_setInt32(format, "width", 1280);
    //OMX.qcom.video.decoder.avc
    codec = AMediaCodec_createCodecByName("OMX.qcom.video.decoder.avc");

    size_t   nal_size = 0;
    size_t   sps_pps_size   = 0;
    size_t   convert_size   = ic->streams[videoStream]->codecpar->extradata_size + 20;
    uint8_t *convert_buffer = (uint8_t *)calloc(1, convert_size);
    convert_sps_pps(ic->streams[videoStream]->codecpar->extradata, ic->streams[videoStream]->codecpar->extradata_size,
                    convert_buffer, convert_size,
                    &sps_pps_size, &nal_size);

    for(int i = 0; i < sps_pps_size; i+=4) {
        ALOGE("csd-0[%d]: %02x%02x%02x%02x\n", sps_pps_size, (int)convert_buffer[i+0], (int)convert_buffer[i+1], (int)convert_buffer[i+2], (int)convert_buffer[i+3]);
    }

    AMediaFormat_setBuffer(format, "csd-0", convert_buffer, sps_pps_size);

    //codec = AMediaCodec_createDecoderByType("video/avc");
    ALOGD("codec:%p", codec);
    media_status_t s= AMediaCodec_configure(codec, format, mpp->native_window, NULL, 0);
    ALOGD("media_status_t:%d extrasize:%d", s, ic->streams[videoStream]->codecpar->extradata_size);
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
        av_packet_split_side_data(pkt);
        unsigned char *p = pkt->data;
        int msize = pkt->size;
        H264ConvertState convert_state = {0,0};

        ALOGD("before data[0]:%02x %02x %02x %02x %02x %02x %02x %02x data[%d]:%02x %02x %02x %02x %02x %02x %02x %02x\n", \
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], msize-8, p[msize - 8], p[msize - 7], p[msize - 6], p[msize - 5], p[msize - 4] \
            , p[msize - 3], p[msize - 2], p[msize - 1]);
//        av_packet_split_side_data(pkt);
        convert_h264_to_annexb(pkt->data, pkt->size, 4, &convert_state);
        ALOGD("after data[0]:%02x %02x %02x %02x %02x %02x %02x %02x data[%d]:%02x %02x %02x %02x %02x %02x %02x %02x\n", \
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], msize-8, p[msize - 8], p[msize - 7], p[msize - 6], p[msize - 5], p[msize - 4] \
            , p[msize - 3], p[msize - 2], p[msize - 1]);
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
            ALOGD("format changed to: %s", AMediaFormat_toString(format));+
            AMediaFormat_delete(format);
            //while(1);
        } else if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            ALOGD("no output buffer right now %zd", status);
        } else {
            ALOGD("unexpected info code: %zd", status);
        }

        usleep(20000);
    }
    AMediaCodec_delete(codec);
    return NULL;
}

void post_event(JNIEnv *env, jobject thiz, int what, int arg1, int arg2, jobject obj)
{
    //ALOGD("post_event enter, what:%d arg1:%d arg2:%d", what, arg1, arg2);
    char *method_name     = "postEventFromNative";
    char *method_sign     = "(Ljava/lang/Object;IIILjava/lang/Object;)V";
    jobject weak_this = (jobject)get_weakthis(env, thiz);
    jmethodID method_postEventFromNative = env->GetStaticMethodID(g_class, method_name, method_sign);
    env->CallStaticVoidMethod(g_class, method_postEventFromNative, weak_this, what, arg1, arg2, obj);
    //ALOGD("post_event leave");
}

MiniPlayer *get_MiniPlayer(JNIEnv *env, jobject thiz)
{
    //class, member_name, type， "J":long
    jfieldID mNativeMiniPlayer = env->GetFieldID(g_class, "mNativeMiniPlayer", "J");
    MiniPlayer *miniplayer = (MiniPlayer *)env->GetLongField(thiz, mNativeMiniPlayer);
    return miniplayer;
}

void *get_weakthis(JNIEnv *env, jobject thiz)
{
    MiniPlayer *mpp = get_MiniPlayer(env, thiz);
    return mpp->weak_thiz;
}

void MiniPlayer_start(JNIEnv *env, jobject thiz) {
    MiniPlayer *mpp = get_MiniPlayer(env, thiz);

    pthread_create(&mpp->decode_thread, NULL, decode_thread, mpp);
}

void MiniPlayer_release(JNIEnv *env, jobject thiz) {
    MiniPlayer *mpp = get_MiniPlayer(env, thiz);
    ANativeWindow_release(mpp->native_window);
    free(mpp);
}

void MiniPlayer_setup(JNIEnv *env, jobject thiz, jobject weak_this) {
    //ALOGD("MiniPlayer_setup");
    //把java中miniplayer对象里面的mNativeMiniPlayer赋值为mp
    jlong mp = (jlong)malloc(sizeof(MiniPlayer));
    //class, member_name, type， "J":long
    jfieldID mNativeMiniPlayer = env->GetFieldID(g_class, "mNativeMiniPlayer", "J");
    env->SetLongField(thiz, mNativeMiniPlayer, mp);
    //保存java对象weak_this到mpp->weak_thiz
    MiniPlayer *mpp = get_MiniPlayer(env, thiz);
    mpp->weak_thiz = env->NewGlobalRef(weak_this);
    post_event(env, thiz, 100, 200, 300, NULL);
    //ALOGD("MiniPlayer_leave");
}

void MiniPlayer_setLoglevel(JNIEnv *env, jclass clazz, jint level)
{
    av_log_set_level(level);
}

void MiniPlayer_setLogTag(JNIEnv *env, jobject thiz, jstring tag)
{
    MiniPlayer *mpp = get_MiniPlayer(env, thiz);
    const char *tlogtag = env->GetStringUTFChars(tag, 0);
    mpp->logtag = av_strdup(tlogtag);
    env->ReleaseStringUTFChars(tag, tlogtag);
}

void MiniPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path)
{
    MiniPlayer *mpp = get_MiniPlayer(env, thiz);
    const char *tpath = env->GetStringUTFChars(path, 0);
    mpp->_path = av_strdup(tpath);
    env->ReleaseStringUTFChars(path, tpath);
}

void MiniPlayer_setSurface(JNIEnv *env, jobject thiz, jobject jsurface)
{
    MiniPlayer *mpp = get_MiniPlayer(env, thiz);
    mpp->native_window = ANativeWindow_fromSurface(env, jsurface);
    int w = ANativeWindow_getWidth(mpp->native_window);
    int h = ANativeWindow_getHeight(mpp->native_window);
    ALOGD("native_window:%p w:%d h:%d\n", mpp->native_window, w, h);
}


static JNINativeMethod g_methods[] = {
        { "_setDataSource",         "(Ljava/lang/String;)V",     (void *) MiniPlayer_setDataSource },
        { "_setSurface",            "(Landroid/view/Surface;)V", (void *) MiniPlayer_setSurface },
        { "_start",                 "()V",                       (void *) MiniPlayer_start },
        { "_release",               "()V",                       (void *) MiniPlayer_release },
        { "native_setup",           "(Ljava/lang/Object;)V",     (void *) MiniPlayer_setup },
        { "_setLogLevel",           "(I)V",                      (void *) MiniPlayer_setLoglevel },
        { "_setLogTag",             "(Ljava/lang/String;)V",     (void *) MiniPlayer_setLogTag },
};

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    char *class_name = "com/example/miniplayer/MiniPlayer";

    jclass player_clazz = env->FindClass(class_name);
    if (!(player_clazz)) {
        ALOGE("FindClass failed: %s", class_name);
        return -1;
    }
    //全局引用，所有线程都有效
    g_class = static_cast<jclass>(env->NewGlobalRef(player_clazz));
    if (!g_class) {
        ALOGE("FindClass::NewGlobalRef failed: %s", class_name);
        env->DeleteLocalRef(g_class);
        return -1;
    }
    env->DeleteLocalRef(player_clazz);
    //注册native方法和java方法的对应关系
    env->RegisterNatives(g_class, g_methods,  (int) (sizeof(g_methods) / sizeof((g_methods)[0])));

    av_jni_set_java_vm(vm, NULL);
    //注册所有编解码器，解复用器，初始化网络
    av_log_set_level(AV_LOG_INFO);
    av_log_set_callback(ffp_log_callback_report);
    av_register_all();
    avformat_network_init();

    return JNI_VERSION_1_6;
}