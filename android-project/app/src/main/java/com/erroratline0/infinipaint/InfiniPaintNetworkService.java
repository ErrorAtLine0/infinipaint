package com.erroratline0.infinipaint;

import android.app.ForegroundServiceStartNotAllowedException;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;
import android.view.KeyEvent;


public class InfiniPaintNetworkService extends Service {
    public static native void networkServiceStarted();
    public static native void networkServiceDestroyed();

    private static final String CHANNEL_ID = "NetworkServiceChannel";

    @Override
    public void onCreate() {
        createNotificationChannel();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Notification notification = new Notification.Builder(this, CHANNEL_ID)
                .setContentTitle("Online canvas")
                .setContentText("Currently online")
                .build();
        startForeground(1, notification);
        networkServiceStarted();
        // For now, don't try restarting the connection after we've been disconnected
        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        // We don't provide binding, so return null
        return null;
    }

    @Override
    public void onDestroy() {
        networkServiceDestroyed();
    }

    private void createNotificationChannel() {
        NotificationChannel serviceChannel = new NotificationChannel(
                CHANNEL_ID,
                "Network Service Channel",
                NotificationManager.IMPORTANCE_DEFAULT
        );
        NotificationManager manager = getSystemService(NotificationManager.class);
        manager.createNotificationChannel(serviceChannel);
    }
}