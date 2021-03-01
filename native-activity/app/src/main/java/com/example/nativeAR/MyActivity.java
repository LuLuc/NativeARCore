package com.example.nativeAR;

import android.Manifest;
import android.app.NativeActivity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

public class MyActivity extends NativeActivity implements ActivityCompat.OnRequestPermissionsResultCallback  {

    private static final int PERMISSIONS_MULTIPLE_REQUEST = 123;

    native static void notifyPermission(boolean cameraGranted);

    private final String[] accessPermissions = new String[] {
            Manifest.permission.CAMERA,
            Manifest.permission.ACCESS_FINE_LOCATION
    };

    public void RequestCamera() {

        boolean needRequire  = false;

        for(String access : accessPermissions) {
            if(ActivityCompat.checkSelfPermission(this, access) != PackageManager.PERMISSION_GRANTED) {
                needRequire = true;
                break;
            }
        }

        if (needRequire) {
            ActivityCompat.requestPermissions(
                    this,
                    accessPermissions,
                    PERMISSIONS_MULTIPLE_REQUEST);
        }
        else {
            notifyPermission(true);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {

        if (PERMISSIONS_MULTIPLE_REQUEST == requestCode && grantResults.length == accessPermissions.length) {
            notifyPermission(grantResults[0] == PackageManager.PERMISSION_GRANTED);
        } else {
            super.onRequestPermissionsResult(requestCode,
                    permissions,
                    grantResults);
        }
    }

    static {
        System.loadLibrary("native-activity");
    }
}

