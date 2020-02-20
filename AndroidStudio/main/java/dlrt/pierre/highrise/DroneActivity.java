package dlrt.pierre.highrise;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.input.InputManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.text.format.Formatter;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Arrays;
import java.util.List;

public class DroneActivity extends AppCompatActivity {

    public static final String TAG = DroneActivity.class.getSimpleName();

    TcpClient mTcpClient;
    TextView textView1, textView2;
    EditText editText;

    int cpt=0;
    public static String IP_addr;
    char[] sendBuf = new char[] {};
    private long keyMap = 0;
    private float[] mAxes = new float[AxesMapping.values().length];

    private WifiManager wifiManager;
    WifiReceiver receiverWifi;
    private InputManager mInputManager;
    ConnectTask connectTask = new ConnectTask();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_drone);

        //final ConnectTask connectTask = new ConnectTask();

        Button btnSend = findViewById(R.id.btnSend);
        //editText = findViewById(R.id.editText);
        textView1 = findViewById(R.id.textView1);
        textView2 = findViewById(R.id.textView2);

        wifiManager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        receiverWifi = new WifiReceiver();
        registerReceiver(receiverWifi, intentFilter);

        mInputManager = (InputManager) getSystemService(Context.INPUT_SERVICE);

        if (!wifiManager.isWifiEnabled()) {
            Toast.makeText(getApplicationContext(), "Turning WiFi ON...", Toast.LENGTH_LONG).show();
            wifiManager.setWifiEnabled(true);
        }

        btnSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (wifiManager.getConnectionInfo()!=null) {
                    //String message = editText.getText().toString();
                    if (mTcpClient != null) {
                        Log.d(TAG, "onClick: SendMessageTask created");
                        //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, keyMap);
                        new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sendBuf);
                        //sendBuf[0]++;
                        Log.d(TAG, "onClick: keymap="+Long.toHexString(keyMap));
                    }
                    //editText.setText("");
                } else {
                    Toast.makeText(getApplicationContext(),"No connected device", Toast.LENGTH_SHORT).show();
                }
            }
        });

        mInputManager.registerInputDeviceListener(new InputManager.InputDeviceListener() {
            @Override
            public void onInputDeviceAdded(int i) {
                Log.d(TAG, "onInputDeviceAdded: "+mInputManager.getInputDevice(i));
            }

            @Override
            public void onInputDeviceRemoved(int i) {
                Log.d(TAG, "onInputDeviceRemoved: "+mInputManager.getInputDevice(i));
            }

            @Override
            public void onInputDeviceChanged(int i) {
                Log.d(TAG, "onInputDeviceChanged: "+mInputManager.getInputDevice(i));
            }
        }, null);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent ev) {
        // Log.d(TAG, "onGenericMotionEvent: " + ev);
        InputDevice device = ev.getDevice();
        // Only care about game controllers.
        if (device != null) {
            for (AxesMapping axesMapping : AxesMapping.values()) {
                mAxes[axesMapping.ordinal()] = getCenteredAxis(ev, device, axesMapping.getMotionEvent());
            }
            //updateAxes();
            updatePackets();
            if (wifiManager.getConnectionInfo()!=null && mTcpClient != null) {
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, keyMap);
            }
            return true;
        }
        return super.dispatchGenericMotionEvent(ev);
    }

    private float getCenteredAxis(MotionEvent event, InputDevice device, int axis) {
        InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());

        // A joystick at rest does not always report an absolute position of
        // (0,0). Use the getFlat() method to determine the range of values
        // bounding the joystick axis center.
        if (range != null) {
            float flat = range.getFlat();
            float value = event.getAxisValue(axis);

            // Ignore axis values that are within the 'flat' region of the
            // joystick axis center.
            if (Math.abs(value) > flat) {
                return value;
            }
        }
        return 0;
    }

    private void updatePackets() {

        int[] channels = {1024,1024,1024,1024,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        float in_min = -1, in_max = 1, out_min = 0, out_max = 2047;

        for(int i = 0; i< mAxes.length-2;i++) {
            channels[i] = (char) Math.round((mAxes[i] - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
        }
        channels[mAxes.length-2] = (int) mAxes[mAxes.length-2];
        channels[mAxes.length-1] = (int) mAxes[mAxes.length-1];

    }

    private void updateAxes() {

        float in_min = -1, in_max = 1, out_min = 0, out_max = 255;
        long keyMapAxes = 0;

        for(int i = 0; i< mAxes.length-2;i++) {
            keyMapAxes <<= 8;
            keyMapAxes |= (char) Math.round((mAxes[i] - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
        }
        keyMapAxes <<= 2;
        keyMapAxes |= (char) mAxes[mAxes.length-2]+1;
        keyMapAxes <<= 2;
        keyMapAxes |= (char) mAxes[mAxes.length-1]+1;
        //textView1.setText("Motion:\n"+Long.toHexString(keyMapAxes));
        keyMapAxes <<= 12;
        keyMap = (keyMap & 4095) | keyMapAxes; // 0x0000000000000FFF : erase axes
        textView2.setText("KeyMap:\n"+Long.toHexString(keyMap));
    }

    public enum AxesMapping {
        AXIS_X(MotionEvent.AXIS_X), // Left joystick, left/right
        AXIS_Y(MotionEvent.AXIS_Y), // Left joystick, up/down
        AXIS_Z(MotionEvent.AXIS_Z), // Right joystick, left/right
        AXIS_RZ(MotionEvent.AXIS_RZ), // Right joystick, up/down
        AXIS_RX(MotionEvent.AXIS_RX), // L2
        AXIS_RY(MotionEvent.AXIS_RY), // R2
        AXIS_HAT_X(MotionEvent.AXIS_HAT_X), // Arrows LEFT RIGHT
        AXIS_HAT_Y(MotionEvent.AXIS_HAT_Y); // Arrows UP DOWN

        private final int mMotionEvent;

        AxesMapping(int motionEvent) {
            mMotionEvent = motionEvent;
        }

        private int getMotionEvent() {
            return mMotionEvent;
        }
    }


    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        Log.d(TAG, "entered dispatchKeyEvent");
        // Update device state for visualization and logging.
        InputDevice dev = mInputManager.getInputDevice(event.getDeviceId());
        //TextView textView = findViewById(R.id.textView);

        if (dev != null) {
            switch (event.getAction()) {
                case KeyEvent.ACTION_DOWN:
                    keyMap |= updateButton(event.getKeyCode());
                    //Log.d(TAG, "dispatchKeyEvent: "+getNameFromCode(event.getKeyCode())+" pressed");
                    //textView.setText(getNameFromCode(event.getKeyCode()));
                    break;
                case KeyEvent.ACTION_UP:
                    keyMap &= ~updateButton(event.getKeyCode());
                    //Log.d(TAG, "dispatchKeyEvent: "+getNameFromCode(event.getKeyCode())+" released");
                    break;
            }
            //textView2.setText("Button:\n"+Long.toBinaryString(keyMap));
            if (wifiManager.getConnectionInfo()!=null && mTcpClient != null) {
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, keyMap);
            }
        }
        return super.dispatchKeyEvent(event);
    }

    private Long updateButton(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_Q:
                return (long) 1 << 12;
            case KeyEvent.KEYCODE_BUTTON_A: // Square
                return (long) 1 << 11;
            case KeyEvent.KEYCODE_BUTTON_B: // Cross
                return (long) 1 << 10;
            case KeyEvent.KEYCODE_BUTTON_C: // Circle
                return (long) 1 << 9;
            case KeyEvent.KEYCODE_BUTTON_X: // Triangle
                return (long) 1 << 8;
            case KeyEvent.KEYCODE_BUTTON_Y: // L1
                return (long) 1 << 7;
            case KeyEvent.KEYCODE_BUTTON_Z: // R1
                return (long) 1 << 6;
            case KeyEvent.KEYCODE_BUTTON_L1: // L2
                return (long) 1 << 5;
            case KeyEvent.KEYCODE_BUTTON_R1: // R2
                return (long) 1 << 4;
            case KeyEvent.KEYCODE_BUTTON_L2: // Share
                return (long) 1 << 3;
            case KeyEvent.KEYCODE_BUTTON_R2: // Option
                return (long) 1 << 2;
            case KeyEvent.KEYCODE_BUTTON_MODE: // PS button
                return (long) 1 << 1;
            case KeyEvent.KEYCODE_BUTTON_THUMBL: // Big central button
                return (long) 1;
            default:
                return (long) 0;
        }
    }

    /**
     * Sends a message using a background task to avoid doing long/network operations on the UI thread
     */
    public class SendMessageTask extends AsyncTask<char[], Void, Void> {
        @Override
        protected Void doInBackground(char[]... params) {
            Log.d(TAG, "SendMessageTask: executing");
            mTcpClient.sendMessageChar(params[0]);
            return null;
        }
    }


    public class ConnectTask extends AsyncTask<Integer, Integer, TcpClient> {
        @Override
        protected TcpClient doInBackground(Integer... message) {
            mTcpClient = new TcpClient(new TcpClient.OnMessageReceived() {
                @Override
                public void messageReceived(int message) {
                    //this method calls the onProgressUpdate
                    //publishProgress(message);
                    Log.d(TAG, "messageReceived: AsyncTask launched");
                    cpt++;
                }
            });
            while(true){
                while(wifiManager.getConnectionInfo()==null) {}
                mTcpClient.run();
            }
        }

        /*@Override
        protected void onProgressUpdate(Integer... values) {
            super.onProgressUpdate(values);
            //response received from server
            //
            Log.d("test", "response " + values[0]);
            tv.setText(values[0]);
            //process server response here....
        }*/
    }

    class WifiReceiver extends BroadcastReceiver {

        private static final String TAG = "WifiReceiver";
        public String IP_ADDR;

        //WifiManager wifiManager;

        public WifiReceiver(){//WifiManager wifiManager) {

            //this.wifiManager = wifiManager;
        }

        public void onReceive(Context context, Intent intent) {

            //String action = intent.getAction();

            //if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
            if (networkInfo.isConnected()) {
                IP_addr = Formatter.formatIpAddress(wifiManager.getDhcpInfo().serverAddress);
                if(mTcpClient==null){
                    connectTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                }
            } else {
                if (mTcpClient!=null){
                    mTcpClient.stopClient();
                }
            }
        }
    }
}
