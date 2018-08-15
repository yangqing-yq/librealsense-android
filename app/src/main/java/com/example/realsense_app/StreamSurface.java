package com.example.realsense_app;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.annotation.Nullable;
import android.support.constraint.ConstraintLayout;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Spinner;

import java.util.Locale;

public class StreamSurface extends FrameLayout {

    public StreamSurface(Context context) {
        this(context, null);
    }

    public StreamSurface(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public StreamSurface(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        this.mCurrentConfig = new StreamConfig();

        init(context, attrs, defStyleAttr);
    }

    private void init(Context context, AttributeSet attrs, int defStyle) {

        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        inflater.inflate(R.layout.stream_surface, this);

        this.mStreamEnable = this.findViewById(R.id.checkBoxEnable);
        this.mStreamTypeList = this.findViewById(R.id.spinnerStream);
        this.mStreamWidth = this.findViewById(R.id.spinnerWidth);
        this.mStreamHeight = this.findViewById(R.id.spinnerHeight);
        this.mStreamFPS = this.findViewById(R.id.spinnerFPS);
        this.mStreamFormat = this.findViewById(R.id.spinnerFormat);

        this.mStreamView = this.findViewById(R.id.surfaceView);

        mDefaultConfig = new StreamConfig();

        // Load attributes
        final TypedArray a = getContext().obtainStyledAttributes(
                attrs, R.styleable.StreamSurface, defStyle, 0);

        this.mDefaultConfig.mType = a.getString(R.styleable.StreamSurface_DefaultStream);
        this.mDefaultConfig.mFormat = a.getString(R.styleable.StreamSurface_DefaultFormat);
        this.mDefaultConfig.mWidth = a.getString(R.styleable.StreamSurface_DefaultWidth);
        this.mDefaultConfig.mHeight = a.getString(R.styleable.StreamSurface_DefaultHeight);
        this.mDefaultConfig.mFPS = a.getString(R.styleable.StreamSurface_DefaultFPS);


        this.mStreamEnable.setChecked(a.getBoolean(R.styleable.StreamSurface_DefaultEnable, false));

        this.initSpinner(context, this.mStreamTypeList, a.getString(R.styleable.StreamSurface_StreamList), this.mDefaultConfig.mType);
        this.initSpinner(context, this.mStreamFormat, a.getString(R.styleable.StreamSurface_FormatList), this.mDefaultConfig.mFormat);
        this.initSpinner(context, this.mStreamFPS, a.getString(R.styleable.StreamSurface_FPSList), this.mDefaultConfig.mFPS);
        this.initSpinner(context, this.mStreamHeight, a.getString(R.styleable.StreamSurface_HeightList), this.mDefaultConfig.mHeight);
        this.initSpinner(context, this.mStreamWidth, a.getString(R.styleable.StreamSurface_WidthList), this.mDefaultConfig.mWidth);

        //this.mStreamWidth.setText(String.format(Locale.getDefault(), "%d", this.mDefaultConfig.mWidth));
        //this.mStreamHeight.setText(String.format(Locale.getDefault(), "%d", this.mDefaultConfig.mHeight));
        //this.mStreamFPS.setText(String.format(Locale.getDefault(), "%d", this.mDefaultConfig.mFPS));

        ViewGroup.LayoutParams params = this.mStreamView.getLayoutParams();
        params.width = Integer.parseInt(this.mDefaultConfig.mWidth);
        params.height = Integer.parseInt(this.mDefaultConfig.mHeight);
        this.mStreamView.setLayoutParams(params);

        a.recycle();
    }

    private void initSpinner(Context context, Spinner spinner, @Nullable String list, String defValue) {
        if (list == null) return;

        String[] _list = list.split(",");
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(context, android.R.layout.simple_spinner_dropdown_item, _list);
        spinner.setAdapter(adapter);

        int defPosition = adapter.getPosition(defValue);
        spinner.setSelection(defPosition);
    }

    public boolean isStreamEnable() {
        return this.mStreamEnable.isChecked();
    }

    public StreamConfig getCurrentConfig() {
        this.mCurrentConfig.mType = this.mStreamTypeList.getSelectedItem().toString();
        this.mCurrentConfig.mFormat = this.mStreamFormat.getSelectedItem().toString();
        this.mCurrentConfig.mWidth = this.mStreamWidth.getSelectedItem().toString();
        this.mCurrentConfig.mHeight =this.mStreamHeight.getSelectedItem().toString();
        this.mCurrentConfig.mFPS = this.mStreamFPS.getSelectedItem().toString();

        ViewGroup.LayoutParams params = this.mStreamView.getLayoutParams();
        params.width = Integer.parseInt(this.mCurrentConfig.mWidth);
        params.height = Integer.parseInt(this.mCurrentConfig.mHeight);
        this.mStreamView.setLayoutParams(params);

        return this.mCurrentConfig;
    }

    public SurfaceView getStreamView() {
        return this.mStreamView;
    }

    StreamConfig mDefaultConfig;
    StreamConfig mCurrentConfig;

    CheckBox mStreamEnable;
    Spinner mStreamTypeList;
    Spinner mStreamFormat;
    Spinner mStreamWidth;
    Spinner mStreamHeight;
    Spinner mStreamFPS;
    SurfaceView mStreamView;

    public class StreamConfig {
        public String mType;
        public String mFormat;
        public String mWidth;
        public String mHeight;
        public String mFPS;
    }
}
