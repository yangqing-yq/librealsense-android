package com.example.realsense_app;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity {
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        String className = getClass().getName().replace('.', '/');
        System.out.println(className);

        //resetPermission();

        this.mStreamDepth = this.findViewById(R.id.streamDepth);
        this.mStreamColor = this.findViewById(R.id.streamColor);
        this.mStreamIR = this.findViewById(R.id.streamIR);

        this.mLogView = this.findViewById(R.id.logView);


        this.mSwitchButtonSwitch = this.findViewById(R.id.toggleButtonSwitch);
        this.mSwitchButtonSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    openSensor0();
                } else {
                    closeSensor0();
                }
            }
        });


        this.mSwitchButtonPower = this.findViewById(R.id.toggleButtonPower);
        this.mSwitchButtonPower.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    init();
                    MainActivity.this.mLogView.setText(logMessage());
                } else {
                    if (MainActivity.this.mSwitchButtonPlay.isChecked()) {
                        MainActivity.this.mSwitchButtonPlay.setChecked(false);
                    }
                    cleanup();
                }
            }
        });
        this.mSwitchButtonPlay = this.findViewById(R.id.toggleButtonPlay);
        this.mSwitchButtonPlay.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    MainActivity.this.mSwitchButtonSwitch.setEnabled(true);
                    if (!MainActivity.this.mSwitchButtonPower.isEnabled()) {
                        MainActivity.this.mSwitchButtonPower.setEnabled(true);
                    }
                    if (!MainActivity.this.mSwitchButtonPower.isChecked()) {
                        MainActivity.this.mSwitchButtonPower.setChecked(true);
                    }
                    Play();
                } else {
                    MainActivity.this.mSwitchButtonSwitch.setChecked(true);
                    MainActivity.this.mSwitchButtonSwitch.setEnabled(false);
                    Stop();
                }
            }
        });


    }
//android:screenOrientation="portrait" \
    Timer t = new Timer();
    Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (!MainActivity.this.mSwitchButtonPower.isEnabled()) {
                MainActivity.this.mSwitchButtonPower.setEnabled(true);
            }
            if (!MainActivity.this.mSwitchButtonPower.isChecked()) {
                MainActivity.this.mSwitchButtonPower.setChecked(true);
            }
            Play();

            //MainActivity.this.mSwitchButtonPlay.setChecked(true);
        }
    };

    @Override
    protected void onResume() {
        super.onResume();
        t.schedule(new TimerTask() {
            @Override
            public void run() {
                handler.sendEmptyMessage(0);
            }
        }, 50);
    }

    @Override
    protected void onStop() {
        Stop();
        super.onStop();
    }

    private void Play() {
        if (!this.mSwitchButtonPower.isEnabled()) {
            this.mSwitchButtonPower.setEnabled(true);
        }
        if (!this.mSwitchButtonPower.isChecked()) {
            this.mSwitchButtonPower.setChecked(true);
        }

        this.EnableStream(this.mStreamDepth);
        this.EnableStream(this.mStreamColor);
        this.EnableStream(this.mStreamIR);

        play();
    }
    private void Stop() {
        stop();
    }

    private void EnableStream(StreamSurface surface) {
        if (!surface.isStreamEnable()) return;

        StreamSurface.StreamConfig config = surface.getCurrentConfig();
        //qing

        if ((Float.valueOf(config.mHeight) / Float.valueOf(config.mWidth) != 0.75) && ((Float.valueOf(config.mHeight) / Float.valueOf(config.mWidth) != 0.5625)))
        {
            float ss=Integer.parseInt(config.mHeight) / Integer.parseInt(config.mWidth);
            Toast toast = Toast.makeText(MainActivity.this, "Resolution configuration is not supported"+ String.valueOf(ss), Toast.LENGTH_SHORT);
            //toast.show();
            //return;
        }

        //qing-end


        int type =  rs2.StreamFromString(config.mType);
        //surface.getStreamView().getHolder().getSurface();
        Log.d("MAINACTIVITY", String.format("Stream: %s w %s h %s fps %s Format: %s",
                config.mType, config.mWidth, config.mHeight, config.mFPS, config.mFormat));
        enableStream(rs2.StreamFromString(config.mType),
                Integer.parseInt(config.mWidth), Integer.parseInt(config.mHeight), Integer.parseInt(config.mFPS),
                rs2.FormatFromString(config.mFormat),
                surface.getStreamView().getHolder().getSurface()
        );

    }

    StreamSurface mStreamDepth;
    StreamSurface mStreamColor;
    StreamSurface mStreamIR;

    TextView mLogView;

    ToggleButton mSwitchButtonPower;
    ToggleButton mSwitchButtonPlay;
    ToggleButton mSwitchButtonSwitch;

    public native static void init();
    public native static void cleanup();
    public native static void enableStream(int stream, int width, int height, int fps, int format, Surface surface);
    public native static void play();
    public native static void stop();
    public native static void openSensor0();
    public native static void closeSensor0();
    public native static String logMessage();
}
