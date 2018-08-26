package com.freiyer.cclient;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.ColorStateList;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.os.NetworkOnMainThreadException;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.EditText;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

import java.io.ByteArrayOutputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity {

    TextView stateText;
    CClientApplication app;
    Thread[] threads;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION );

        stateText = findViewById(R.id.stateText);
        app = (CClientApplication)getApplication();
        threads = new Thread[10];
        stateText.setText(new StringBuilder(app.getServerName() + "\n")
                .append(app.getModeName(app.getMode()))
                .append(" mode\n").toString());

    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int len = event.getPointerCount();
        StringBuilder sb = new StringBuilder(app.getServerName() + "\n");
        sb
                .append(app.getModeName(app.getMode()))
                .append(" mode\n");

        for(int i= 0; i < len && i < 10; i++) {
            final float x = event.getX(i), y = event.getY(i);
            final int idx = i, down;
            switch(event.getAction()) {
                case MotionEvent.ACTION_UP:
                    down = 0; break;
                case MotionEvent.ACTION_DOWN:
                    down = 1; break;
                default:
                    down = 2;
            }
            sb.append(String.format("%d: %f %f %d\n", i, x, y, down));
            new Thread(new Runnable() {
                @Override
                public void run() {
                    if(!app.sendPacket(new TouchPacket(idx, x, y, down))) {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                finish();
                            }
                        });
                    }
                }
            }).start();
        }
        stateText.setText(sb.toString());

        return false;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        app.destroyNetwork();
    }
}
