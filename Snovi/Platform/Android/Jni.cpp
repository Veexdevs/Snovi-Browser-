#include <jni.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include "Platform/Bridge.h"

extern "C" JNIEXPORT jlong JNICALL
Java_com_snovi_browser_NativeEngine_create(JNIEnv* env, jclass, jstring path) {
    const char* p=env->GetStringUTFChars(path,nullptr);
    if(!p) return 0;
    int ok=setenv("SNOVI_CA_FILE",p,1);
    env->ReleaseStringUTFChars(path,p);
    return ok==0 ? reinterpret_cast<jlong>(snovi_create()) : 0;
}
extern "C" JNIEXPORT void JNICALL
Java_com_snovi_browser_NativeEngine_destroy(JNIEnv*,jclass,jlong handle) {
    snovi_destroy(reinterpret_cast<void*>(handle));
}
extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_snovi_browser_NativeEngine_execute(JNIEnv* env,jclass,jlong handle,jbyteArray a,jbyteArray v) {
    if(!handle || !a || !v) return nullptr;
    auto read=[env](jbyteArray arr) {
        jsize len=env->GetArrayLength(arr); std::string out(static_cast<size_t>(len),'\0');
        if(len) env->GetByteArrayRegion(arr,0,len,reinterpret_cast<jbyte*>(&out[0]));
        return out;
    };
    auto action=read(a), value=read(v);
    if(env->ExceptionCheck()) return nullptr;
    char* result=snovi_execute(reinterpret_cast<void*>(handle),action.c_str(),value.c_str());
    if(!result) return nullptr;
    size_t size=std::strlen(result);
    jbyteArray bytes=env->NewByteArray(static_cast<jsize>(size));
    if(bytes) env->SetByteArrayRegion(bytes,0,static_cast<jsize>(size),reinterpret_cast<const jbyte*>(result));
    snovi_free(result);
    return bytes;
}
