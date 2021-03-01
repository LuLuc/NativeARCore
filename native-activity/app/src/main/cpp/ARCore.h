#ifndef SENS_NDK_ARCORE_H
#define SENS_NDK_ARCORE_H

#include <android_native_app_glue.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "arcore_c_api.h"

class ARCore
{
public:
    ARCore(ANativeActivity* activity);
    ~ARCore();

    bool init();
    bool resume();
    void reset();
    void pause();
    bool update();

private:

    void initCameraTexture();

    ANativeActivity* _activity  = nullptr;
    ArSession*       _arSession = nullptr;
    ArFrame*         _arFrame   = nullptr;

    GLuint _cameraTextureId;
    bool _paused;
    bool checkAvailability(JNIEnv* env, jobject context);
};

#endif
