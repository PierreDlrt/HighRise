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
    private char[] channels = {1500,1500,1500,1000,0,0,0,0,0,0,0,0,0,0,0,0}; // RC channels (id 0 ->15)
    private char[] keyMap = {127,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // my channels (id 0 -> 14)
    private char[] sendBuf = new char[40];
    //private char[] sendBufTest = {15, 127, 128, 136, 195, 255, 240, 170};
    private char[] sendBufTest = {1, 255, 0xAA55, 256, 0xCCCC, 0, 0xFFFF};
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
                        /*for(int i=0; i<sendBufTest.length; i++){
                            if(sendBufTest[i]<255){

                            }
                        }*/
                        new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sendBufTest);
                        //sendBuf[0]++;
                        textView1.setText("");
                        textView2.setText("");
                        for (int i=0; i<sendBufTest.length; i++){
                            textView1.append("sendBufTest = "+Integer.toBinaryString(sendBufTest[i])+"\n");
                            /*textView1.append("sendBufTest = "+
                                    ((sendBufTest[i] & 1 << 15) >> 15 ) + "" +
                                    ((sendBufTest[i] & 1 << 14) >> 14 ) + "" +
                                    ((sendBufTest[i] & 1 << 13) >> 13 ) + "" +
                                    ((sendBufTest[i] & 1 << 12) >> 12 ) + "" +
                                    ((sendBufTest[i] & 1 << 11) >> 11 ) + "" +
                                    ((sendBufTest[i] & 1 << 10) >> 10 ) + "" +
                                    ((sendBufTest[i] & 1 << 9) >> 9 ) + "" +
                                    ((sendBufTest[i] & 1 << 8) >> 8 ) + "" +
                                    ((sendBufTest[i] & 1 << 7) >> 7 ) + "" +
                                    ((sendBufTest[i] & 1 << 6) >> 6 ) + "" +
                                    ((sendBufTest[i] & 1 << 5) >> 5 ) + "" +
                                    ((sendBufTest[i] & 1 << 4) >> 4 ) + "" +
                                    ((sendBufTest[i] & 1 << 3) >> 3 ) + "" +
                                    ((sendBufTest[i] & 1 << 2) >> 2 ) + "" +
                                    ((sendBufTest[i] & 1 << 1) >> 1 ) + "" +
                                    ((sendBufTest[i] & 1))+"\n");*/
                            //textView2.append(Integer.toHexString(sendBufTest[i])+"\n");
                        }
                        //Log.d(TAG, "onClick: keymap="+Long.toHexString(keyMap));
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
            updateChannels();
            if (wifiManager.getConnectionInfo()!=null && mTcpClient != null) {
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sendBuf);
                new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, channels);
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

    float map(float x, float out_min, float out_max) {
        return (x + 1) * (out_max - out_min) /2 + out_min;
    }

    private void updateChannels() {

        float CHANNEL_RANGE_MIN = 1000, CHANNEL_RANGE_MAX = 2000, CHANNEL_RANGE_CENTER = (CHANNEL_RANGE_MAX - CHANNEL_RANGE_MIN)/2;

        channels[0] = (char) Math.round(map(mAxes[0], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));
        channels[1] = (char) Math.round(map(mAxes[1], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));
        channels[2] = (char) Math.round(map(mAxes[2], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));
        channels[3] = (char) Math.round(map(mAxes[5], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));

        keyMap[0] = (char) Math.round(map(mAxes[3], 0, 255));
        keyMap[1] = (char) (mAxes[6]+1);
        keyMap[2] = (char) (mAxes[7]+1);

        //channelsToPackets();
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
        int keyCode = event.getKeyCode();

        if (dev != null && getButton(keyCode) != -1) {
            switch (event.getAction()) {
                case KeyEvent.ACTION_DOWN:
                    keyMap[getButton(keyCode)] = 1;
                    break;
                case KeyEvent.ACTION_UP:
                    keyMap[getButton(keyCode)] = 0;
                    break;
            }
            //channelsToPackets();
            if (wifiManager.getConnectionInfo()!=null && mTcpClient != null) {
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sendBuf);
            }
        }
        return super.dispatchKeyEvent(event);
    }

    private int getButton(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A: // Square
                return 3;
            case KeyEvent.KEYCODE_BUTTON_B: // Cross
                return 4;
            case KeyEvent.KEYCODE_BUTTON_C: // Circle
                return 5;
            case KeyEvent.KEYCODE_BUTTON_X: // Triangle
                return 6;
            case KeyEvent.KEYCODE_BUTTON_Y: // L1
                return 7;
            case KeyEvent.KEYCODE_BUTTON_Z: // R1
                return 8;
            case KeyEvent.KEYCODE_BUTTON_L1: // L2
                return 9;
            case KeyEvent.KEYCODE_BUTTON_R1: // R2
                return 10;
            case KeyEvent.KEYCODE_BUTTON_SELECT: // L3
                return 11;
            case KeyEvent.KEYCODE_BUTTON_L2: // Share
                return 12;
            case KeyEvent.KEYCODE_BUTTON_R2: // Option
                return 13;
            case KeyEvent.KEYCODE_BUTTON_THUMBL: // Big central button
                return 14;
            default:
                return -1;
        }
    }

    private void channelsToPackets(){

        sendBuf[0] = 0x0F;
        sendBuf[1] = (char) ((channels[0] & 0x07FF));
        sendBuf[2] = (char) ((channels[0] & 0x07FF)>>8 | (channels[1] & 0x07FF)<<3);
        sendBuf[3] = (char) ((channels[1] & 0x07FF)>>5 | (channels[2] & 0x07FF)<<6);
        sendBuf[4] = (char) ((channels[2] & 0x07FF)>>2);
        sendBuf[5] = (char) ((channels[2] & 0x07FF)>>10 | (channels[3] & 0x07FF)<<1);
        sendBuf[6] = (char) ((channels[3] & 0x07FF)>>7 | (channels[4] & 0x07FF)<<4);
        sendBuf[7] = (char) ((channels[4] & 0x07FF)>>4 | (channels[5] & 0x07FF)<<7);
        sendBuf[8] = (char) ((channels[5] & 0x07FF)>>1);
        sendBuf[9] = (char) ((channels[5] & 0x07FF)>>9 | (channels[6] & 0x07FF)<<2);
        sendBuf[10] = (char) ((channels[6] & 0x07FF)>>6 | (channels[7] & 0x07FF)<<5);
        sendBuf[11] = (char) ((channels[7] & 0x07FF)>>3);
        sendBuf[12] = (char) ((channels[8] & 0x07FF));
        sendBuf[13] = (char) ((channels[8] & 0x07FF)>>8 | (channels[9] & 0x07FF)<<3);
        sendBuf[14] = (char) ((channels[9] & 0x07FF)>>5 | (channels[10] & 0x07FF)<<6);
        sendBuf[15] = (char) ((channels[10] & 0x07FF)>>2);
        sendBuf[16] = (char) ((channels[10] & 0x07FF)>>10 | (channels[11] & 0x07FF)<<1);
        sendBuf[17] = (char) ((channels[11] & 0x07FF)>>7 | (channels[12] & 0x07FF)<<4);
        sendBuf[18] = (char) ((channels[12] & 0x07FF)>>4 | (channels[13] & 0x07FF)<<7);
        sendBuf[19] = (char) ((channels[13] & 0x07FF)>>1);
        sendBuf[20] = (char) ((channels[13] & 0x07FF)>>9 | (channels[14] & 0x07FF)<<2);
        sendBuf[21] = (char) ((channels[14] & 0x07FF)>>6 | (channels[15] & 0x07FF)<<5);
        sendBuf[22] = (char) ((channels[15] & 0x07FF)>>3);
        sendBuf[23] = 0x00;
        sendBuf[24] = 0x00;

        textView1.setText("");
        for (int j=0; j<25; j++){
            textView1.append("packet["+j+"] = "+String.format("%02X\n",(byte) sendBuf[j]));
        }

        textView2.setText("");
        for (int i=0; i<15; i++){
            sendBuf[i+25] = keyMap[i];
            textView2.append("myChannels["+i+"] = "+String.format("%02X\n",(byte) sendBuf[i+25]));
        }

        textView2.append("\n");
        for (int k=0; k<sendBufTest.length; k++) {
            textView2.append("sendBufTest["+k+"] = "+String.format("%02X\n",sendBufTest[k]+"\n"));
        }
    }

    /**
     * Sends a message using a background task to avoid doing long/network operations on the UI thread
     */
    public class SendMessageTask extends AsyncTask<char[], Void, Void> {
        @Override
        protected Void doInBackground(char[]... params) {
            Log.d(TAG, "SendMessageTask: executing");
            mTcpClient.sendMessageByte(params[0]);
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
