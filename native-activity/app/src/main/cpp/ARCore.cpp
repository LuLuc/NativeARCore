#include "ARCore.h"
#include <jni.h>
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

ARCore::ARCore(ANativeActivity* activity)
  : _activity(activity)
{
}

bool ARCore::checkAvailability(JNIEnv* env, jobject context)
{
    ArAvailability availability;
    ArCoreApk_checkAvailability(env, context, &availability);

    bool available = false;
    if (availability == AR_AVAILABILITY_UNKNOWN_CHECKING) {
        checkAvailability(env, context);
    }
    else if (availability == AR_AVAILABILITY_SUPPORTED_NOT_INSTALLED ||
             availability == AR_AVAILABILITY_SUPPORTED_APK_TOO_OLD ||
             availability == AR_AVAILABILITY_SUPPORTED_INSTALLED)
    {
        available = true;
    }
    return available;
}

ARCore::~ARCore()
{
    reset();
}

void ARCore::reset()
{
    if (_arSession != nullptr)
    {
        pause();
        ArSession_destroy(_arSession);
        ArFrame_destroy(_arFrame);
        _arSession = nullptr;
        glDeleteTextures(1, &_cameraTextureId);
    }
}

void ARCore::initCameraTexture()
{
    glGenTextures(1, &_cameraTextureId);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, _cameraTextureId);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

bool ARCore::init()
{
    if (_arSession != nullptr)
        reset();

    JNIEnv* env;
    _activity->vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    _activity->vm->AttachCurrentThread(&env, NULL);
    jobject activityObj = env->NewGlobalRef(_activity->clazz);

    if (!checkAvailability(env, activityObj))
    {
        env->DeleteGlobalRef(activityObj);
        _activity->vm->DetachCurrentThread();
        return false;
    }

    ArInstallStatus install_status;
    ArCoreApk_requestInstall(env, activityObj, true, &install_status);

    if (install_status != AR_INSTALL_STATUS_INSTALLED)
    {
        env->DeleteGlobalRef(activityObj);
        _activity->vm->DetachCurrentThread();
        return false;
    }

    if (ArSession_create(env, activityObj, &_arSession) != AR_SUCCESS)
    {
        env->DeleteGlobalRef(activityObj);
        _activity->vm->DetachCurrentThread();
        return false;
    }

    if (!_arSession)
    {
        env->DeleteGlobalRef(activityObj);
        _activity->vm->DetachCurrentThread();
        return false;
    }

    ArSession_setDisplayGeometry(_arSession, 0, 1920, 900);

    // ----- config -----
    ArConfig* arConfig = nullptr;
    ArConfig_create(_arSession, &arConfig);

    if (!arConfig)
    {
        ArSession_destroy(_arSession);
        _arSession = nullptr;
        env->DeleteGlobalRef(activityObj);
        _activity->vm->DetachCurrentThread();
        return false;
    }

    // Deph texture has values between 0 millimeters to 8191 millimeters. 8m is not enough in our case
    // https://developers.google.com/ar/reference/c/group/ar-frame#arframe_acquiredepthimage
    ArConfig_setDepthMode(_arSession, arConfig, AR_DEPTH_MODE_DISABLED);
    ArConfig_setLightEstimationMode(_arSession, arConfig, AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR);
    ArConfig_setInstantPlacementMode(_arSession, arConfig, AR_INSTANT_PLACEMENT_MODE_DISABLED);

    if (ArSession_configure(_arSession, arConfig) != AR_SUCCESS)
    {
        ArConfig_destroy(arConfig);
        ArSession_destroy(_arSession);
        _arSession = nullptr;
        env->DeleteGlobalRef(activityObj);
        _activity->vm->DetachCurrentThread();
        return false;
    }

    ArConfig_destroy(arConfig);

    // -- arFrame The world state resulting from an update
    // Create with: ArFrame_create
    // Allocate with: ArSession_update
    // Release with: ArFrame_destroy

    ArFrame_create(_arSession, &_arFrame);
    if (!_arFrame)
    {
        ArSession_destroy(_arSession);
        _arSession = nullptr;
        env->DeleteGlobalRef(activityObj);
        _activity->vm->DetachCurrentThread();
        return false;
    }

    initCameraTexture();
    ArSession_setCameraTextureName(_arSession, _cameraTextureId);

    //env->DeleteGlobalRef(activityObj);
    //_activity->vm->DetachCurrentThread();

    const ArStatus status = ArSession_resume(_arSession);
    if (status == AR_SUCCESS)
        _paused = false;

    env->DeleteGlobalRef(activityObj);
    _activity->vm->DetachCurrentThread();

    return true;
}

bool ARCore::update()
{
    LOGI("ARCORE UPDATE");
    if (!_arSession)
        return false;

    if (ArSession_update(_arSession, _arFrame) != AR_SUCCESS)
        return false;

    float mat[16];
    ArCamera* arCamera;
    ArFrame_acquireCamera(_arSession, _arFrame, &arCamera);
    ArCamera_getViewMatrix(_arSession, arCamera, mat);


    LOGI("ARCORE UPDATED");
    return true;
}

bool ARCore::resume()
{
    if (_paused && _arSession != nullptr)
    {
        const ArStatus status = ArSession_resume(_arSession);
        if (status == AR_SUCCESS)
            _paused = false;
    }
    return !_paused;
}

void ARCore::pause()
{
    _paused = true;
    if (_arSession != nullptr)
        ArSession_pause(_arSession);
}
