#include <jni.h>
#include <string>
#include <sstream>
#include <android/log.h>
#include <pthread.h>
#include <iomanip>
#include <unistd.h>
#include <random>

#include "Timer.h"
#include "Laplacian.h"

static const char* kTAG = "c++Logger";
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

  // save all the JVM stuff so we don't have to suffer JNI overhead every call
typedef struct jni_context {
    JavaVM *javaVM;
    jobject mainActivityObj;
    jclass  mainActivityClass;
    pthread_mutex_t  lock;
    int      isThreadAvailable;
} jni_context;
jni_context g_ctx;
bool isContextSet = false;

void* randomTest(void* context) {
    jni_context *pctx = (jni_context*) context;

    // I only want one test running at a  -> check isThreadAvailable with mutex
    pthread_mutex_lock(&pctx->lock);
    if(!pctx->isThreadAvailable) {
        pthread_mutex_unlock(&pctx->lock);
        LOGI("Thread already running.");
        return context;
    } else {
        pctx->isThreadAvailable = 0;
        pthread_mutex_unlock(&pctx->lock);
    }
    LOGI("Thread started (castedTest).");

    // JNI stuff: attaching this c++ thread to the JVM so we can make calls to java
    JavaVM *javaVM = pctx->javaVM;
    JNIEnv *env;
    jint res = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (res != JNI_OK) {
        res = javaVM->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res) {
            LOGE("Failed to AttachCurrentThread, ErrorCode = %d", res);
            pthread_mutex_lock(&pctx->lock);
            pctx->isThreadAvailable = 1;
            pthread_mutex_unlock(&pctx->lock);
            return NULL;
        }
    }

    // now that everything is set up, get references to the java methods we will be using
    jmethodID getThreadCount = env->GetMethodID(pctx->mainActivityClass, "getThreadCount", "()I");
    jmethodID textAppender = env->GetMethodID(pctx->mainActivityClass, "appendToView", "(Ljava/lang/String;)V");
    jmethodID dataDisplayer = env->GetMethodID(pctx->mainActivityClass, "displayData", "([DI)V");

    // do the method call to get desired number of threads to use
    jint numThreads = env->CallIntMethod(pctx->mainActivityObj, getThreadCount);
    int nt = (int)numThreads;

    // android redirects stdout/stderr to /dev/nul so I'll just use stringstream and send that to
    // the java method that puts text on the screen
    std::stringstream cout;
    cout << "Executing (rand layout) with XDIM:" << XDIM << " and YDIM:" << YDIM << " on " << nt << " threads";
    jstring jstr = env->NewStringUTF(cout.str().c_str());
    env->CallVoidMethod(pctx->mainActivityObj, textAppender, jstr);
    env->DeleteLocalRef(jstr);  // explicit release to it is immediately available for GC by JVM

    // we can finally do the ComputeLaplacian task
    double resutls[TESTITRS];  // save results to send back all at once (reduce work for UI until test done)

    float **u = new float *[XDIM];
    float **Lu = new float *[XDIM];

    // Randomize allocation of minor array dimension
    std::vector<int> reorderMap;
    std::vector<int> tempMap;
    for (int i = 0; i < XDIM; i++) tempMap.push_back(i);
    std::random_device r; std::default_random_engine e(r());
    while (!tempMap.empty()) {
        std::uniform_int_distribution<int> uniform_dist(0, tempMap.size()-1);
        int j = uniform_dist(e);
        reorderMap.push_back(tempMap[j]); tempMap[j] = tempMap.back(); tempMap.pop_back();
    }
    for (int i = 0; i < XDIM; i++){
        u[reorderMap[i]] = new float [YDIM];
        Lu[reorderMap[i]] = new float [YDIM];
    }
    Timer timer;
    for(int test = 0; test < TESTITRS; test++)
    {
        timer.Start();
        ComputeLaplacianPtrArr(u, Lu, nt);
        timer.Stop();
        resutls[test] = timer.mostRecentElapsed;
    }
    // DONE

    // prepare to send back data
    jdoubleArray jdblarr= env->NewDoubleArray(TESTITRS);
    env->SetDoubleArrayRegion(jdblarr, 0, TESTITRS, resutls);
    // send back data
    env->CallVoidMethod(pctx->mainActivityObj, dataDisplayer, jdblarr, nt);
    env->DeleteLocalRef(jdblarr);

    // some cleanup before
    for (int i = 0; i < XDIM; i++){
        free(u[i]);
        free(Lu[i]);
    }
    free(u);
    free(Lu);
    pthread_mutex_lock(&pctx->lock);
    pctx->isThreadAvailable = 1;
    LOGI("Thread finished.");
    pthread_mutex_unlock(&pctx->lock);

    return context;
}

void* ptrArrTest(void* context){
    jni_context *pctx = (jni_context*) context;

    // I only want one test running at a  -> check isThreadAvailable with mutex
    pthread_mutex_lock(&pctx->lock);
    if(!pctx->isThreadAvailable) {
        pthread_mutex_unlock(&pctx->lock);
        LOGI("Thread already running.");
        return context;
    } else {
        pctx->isThreadAvailable = 0;
        pthread_mutex_unlock(&pctx->lock);
    }
    LOGI("Thread started (castedTest).");

    // JNI stuff: attaching this c++ thread to the JVM so we can make calls to java
    JavaVM *javaVM = pctx->javaVM;
    JNIEnv *env;
    jint res = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (res != JNI_OK) {
        res = javaVM->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res) {
            LOGE("Failed to AttachCurrentThread, ErrorCode = %d", res);
            pthread_mutex_lock(&pctx->lock);
            pctx->isThreadAvailable = 1;
            pthread_mutex_unlock(&pctx->lock);
            return NULL;
        }
    }

    // now that everything is set up, get references to the java methods we will be using
    jmethodID getThreadCount = env->GetMethodID(pctx->mainActivityClass, "getThreadCount", "()I");
    jmethodID textAppender = env->GetMethodID(pctx->mainActivityClass, "appendToView", "(Ljava/lang/String;)V");
    jmethodID dataDisplayer = env->GetMethodID(pctx->mainActivityClass, "displayData", "([DI)V");

    // do the method call to get desired number of threads to use
    jint numThreads = env->CallIntMethod(pctx->mainActivityObj, getThreadCount);
    int nt = (int)numThreads;

    // android redirects stdout/stderr to /dev/nul so I'll just use stringstream and send that to
    // the java method that puts text on the screen
    std::stringstream cout;
    cout << "Executing (ptr layout) with XDIM:" << XDIM << " and YDIM:" << YDIM << " on " << nt << " threads";
    jstring jstr = env->NewStringUTF(cout.str().c_str());
    env->CallVoidMethod(pctx->mainActivityObj, textAppender, jstr);
    env->DeleteLocalRef(jstr);  // explicit release to it is immediately available for GC by JVM

    // we can finally do the ComputeLaplacian task
    double resutls[TESTITRS];  // save results to send back all at once (reduce work for UI until test done)

    float **u = new float *[XDIM];
    float **Lu = new float *[XDIM];
    for (int i = 0; i < XDIM; i++){
        u[i] = new float [YDIM];
        Lu[i] = new float [YDIM];
    }
    Timer timer;
    for(int test = 0; test < TESTITRS; test++)
    {
        timer.Start();
        ComputeLaplacianPtrArr(u, Lu, nt);
        timer.Stop();
        resutls[test] = timer.mostRecentElapsed;
    }
    // DONE

    // prepare to send back data
    jdoubleArray jdblarr= env->NewDoubleArray(TESTITRS);
    env->SetDoubleArrayRegion(jdblarr, 0, TESTITRS, resutls);
    // send back data
    env->CallVoidMethod(pctx->mainActivityObj, dataDisplayer, jdblarr, nt);
    env->DeleteLocalRef(jdblarr);

    // some cleanup before returning
    for (int i = 0; i < XDIM; i++){
        free(u[i]);
        free(Lu[i]);
    }
    free(u);
    free(Lu);
    pthread_mutex_lock(&pctx->lock);
    pctx->isThreadAvailable = 1;
    LOGI("Thread finished.");
    pthread_mutex_unlock(&pctx->lock);

    return context;
}

void* castedTest(void* context) {
    jni_context *pctx = (jni_context*) context;

    // I only want one test running at a  -> check isThreadAvailable with mutex
    pthread_mutex_lock(&pctx->lock);
    if(!pctx->isThreadAvailable) {
        pthread_mutex_unlock(&pctx->lock);
        LOGI("Thread already running.");
        return context;
    } else {
        pctx->isThreadAvailable = 0;
        pthread_mutex_unlock(&pctx->lock);
    }
    LOGI("Thread started (castedTest).");

    // JNI stuff: attaching this c++ thread to the JVM so we can make calls to java
    JavaVM *javaVM = pctx->javaVM;
    JNIEnv *env;
    jint res = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (res != JNI_OK) {
        res = javaVM->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res) {
            LOGE("Failed to AttachCurrentThread, ErrorCode = %d", res);
            pthread_mutex_lock(&pctx->lock);
            pctx->isThreadAvailable = 1;
            pthread_mutex_unlock(&pctx->lock);
            return NULL;
        }
    }

    // now that everything is set up, get references to the java methods we will be using
    jmethodID getThreadCount = env->GetMethodID(pctx->mainActivityClass, "getThreadCount", "()I");
    jmethodID textAppender = env->GetMethodID(pctx->mainActivityClass, "appendToView", "(Ljava/lang/String;)V");
    jmethodID dataDisplayer = env->GetMethodID(pctx->mainActivityClass, "displayData", "([DI)V");

    // do the method call to get desired number of threads to use
    jint numThreads = env->CallIntMethod(pctx->mainActivityObj, getThreadCount);
    int nt = (int)numThreads;

    // android redirects stdout/stderr to /dev/nul so I'll just use stringstream and send that to
    // the java method that puts text on the screen
    std::stringstream cout;
    cout << "Executing (cast layout) with XDIM:" << XDIM << " and YDIM:" << YDIM << " on " << nt << " threads";
    jstring jstr = env->NewStringUTF(cout.str().c_str());
    env->CallVoidMethod(pctx->mainActivityObj, textAppender, jstr);
    env->DeleteLocalRef(jstr);  // explicit release to it is immediately available for GC by JVM

    // we can finally do the ComputeLaplacian task
    double resutls[TESTITRS];  // save results to send back all at once (reduce work for UI until test done)

    using array_t = float (&) [XDIM][YDIM];

    float *uRaw = new float [XDIM*YDIM];
    float *LuRaw = new float [XDIM*YDIM];
    array_t u = reinterpret_cast<array_t>(*uRaw);
    array_t Lu = reinterpret_cast<array_t>(*LuRaw);
    Timer timer;
    for(int test = 0; test < TESTITRS; test++)
    {
        timer.Start();
        ComputeLaplacian(u, Lu, nt);
        timer.Stop();
        resutls[test] = timer.mostRecentElapsed;
    }
    // DONE

    // prepare to send back data
    jdoubleArray jdblarr= env->NewDoubleArray(TESTITRS);
    env->SetDoubleArrayRegion(jdblarr, 0, TESTITRS, resutls);
    // send back data
    env->CallVoidMethod(pctx->mainActivityObj, dataDisplayer, jdblarr, nt);
    env->DeleteLocalRef(jdblarr);

    // some cleanup before returning
    free(uRaw);
    free(LuRaw);
    pthread_mutex_lock(&pctx->lock);
    pctx->isThreadAvailable = 1;
    LOGI("Thread finished.");
    pthread_mutex_unlock(&pctx->lock);

    return context;
}
void* flipTest(void* context) {
    jni_context *pctx = (jni_context*) context;

    // I only want one test running at a  -> check isThreadAvailable with mutex
    pthread_mutex_lock(&pctx->lock);
    if(!pctx->isThreadAvailable) {
        pthread_mutex_unlock(&pctx->lock);
        LOGI("Thread already running.");
        return context;
    } else {
        pctx->isThreadAvailable = 0;
        pthread_mutex_unlock(&pctx->lock);
    }
    LOGI("Thread started (castedTest).");

    // JNI stuff: attaching this c++ thread to the JVM so we can make calls to java
    JavaVM *javaVM = pctx->javaVM;
    JNIEnv *env;
    jint res = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (res != JNI_OK) {
        res = javaVM->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res) {
            LOGE("Failed to AttachCurrentThread, ErrorCode = %d", res);
            pthread_mutex_lock(&pctx->lock);
            pctx->isThreadAvailable = 1;
            pthread_mutex_unlock(&pctx->lock);
            return NULL;
        }
    }

    // now that everything is set up, get references to the java methods we will be using
    jmethodID getThreadCount = env->GetMethodID(pctx->mainActivityClass, "getThreadCount", "()I");
    jmethodID textAppender = env->GetMethodID(pctx->mainActivityClass, "appendToView", "(Ljava/lang/String;)V");
    jmethodID dataDisplayer = env->GetMethodID(pctx->mainActivityClass, "displayData", "([DI)V");

    // do the method call to get desired number of threads to use
    jint numThreads = env->CallIntMethod(pctx->mainActivityObj, getThreadCount);
    int nt = (int)numThreads;

    // android redirects stdout/stderr to /dev/nul so I'll just use stringstream and send that to
    // the java method that puts text on the screen
    std::stringstream cout;
    cout << "Executing (flip xy) with XDIM:" << XDIM << " and YDIM:" << YDIM << " on " << nt << " threads";
    jstring jstr = env->NewStringUTF(cout.str().c_str());
    env->CallVoidMethod(pctx->mainActivityObj, textAppender, jstr);
    env->DeleteLocalRef(jstr);  // explicit release to it is immediately available for GC by JVM

    // we can finally do the ComputeLaplacian task
    double resutls[TESTITRS];  // save results to send back all at once (reduce work for UI until test done)

    using array_t = float (&) [XDIM][YDIM];

    float *uRaw = new float [XDIM*YDIM];
    float *LuRaw = new float [XDIM*YDIM];
    array_t u = reinterpret_cast<array_t>(*uRaw);
    array_t Lu = reinterpret_cast<array_t>(*LuRaw);
    Timer timer;
    for(int test = 0; test < TESTITRS; test++)
    {
        timer.Start();
        ComputeLaplacianFlip(u, Lu, nt);
        timer.Stop();
        resutls[test] = timer.mostRecentElapsed;
    }
    // DONE

    // prepare to send back data
    jdoubleArray jdblarr= env->NewDoubleArray(TESTITRS);
    env->SetDoubleArrayRegion(jdblarr, 0, TESTITRS, resutls);
    // send back data
    env->CallVoidMethod(pctx->mainActivityObj, dataDisplayer, jdblarr, nt);
    env->DeleteLocalRef(jdblarr);

    // some cleanup before returning
    free(uRaw);
    free(LuRaw);
    pthread_mutex_lock(&pctx->lock);
    pctx->isThreadAvailable = 1;
    LOGI("Thread finished.");
    pthread_mutex_unlock(&pctx->lock);

    return context;
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    memset(&g_ctx, 0, sizeof(g_ctx));

    g_ctx.javaVM = vm;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // JNI version not supported.
    }

    g_ctx.isThreadAvailable = 1;
    g_ctx.mainActivityObj = NULL;

    return  JNI_VERSION_1_6;
}


extern "C" JNIEXPORT void JNICALL
Java_com_example_CS639Playground_MainActivity_executeCPP(
        JNIEnv *env,
        jobject thiz,
        jint mode) {

    int m = (int)mode;

    pthread_t threadInfo_;
    pthread_attr_t threadAttr_;

    pthread_attr_init(&threadAttr_);
    pthread_attr_setdetachstate(&threadAttr_, PTHREAD_CREATE_DETACHED);

    if (!isContextSet) {
        pthread_mutex_init(&g_ctx.lock, NULL);

        jclass clz = env->GetObjectClass(thiz);
        // global ref guaranteed valid until explicitly released
        g_ctx.mainActivityObj = env->NewGlobalRef(thiz);
        g_ctx.mainActivityClass = (jclass) env->NewGlobalRef(clz);

        isContextSet = true;
    }

    int result;
    switch(m) {
        case 0 : result = pthread_create( &threadInfo_, &threadAttr_, castedTest, &g_ctx); break;
        case 1 : result = pthread_create( &threadInfo_, &threadAttr_, ptrArrTest, &g_ctx); break;
        case 2 : result = pthread_create( &threadInfo_, &threadAttr_, randomTest, &g_ctx); break;
        case 3 : result = pthread_create( &threadInfo_, &threadAttr_, flipTest, &g_ctx); break;
    }
    assert(result == 0);
    pthread_attr_destroy(&threadAttr_);
    (void)result;
}