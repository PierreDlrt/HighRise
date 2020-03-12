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
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Calendar;

public class DroneActivity extends AppCompatActivity {

    public static final String TAG = DroneActivity.class.getSimpleName();

    TcpClient mTcpClient;
    TextView textView1, textView2, textView3;
    LinearLayout myLayout;
    ImageView isConnected;

    int cpt=0;
    public static String IP_addr;
    private char[] channels = {992,992,992,192,0,0,0,0,0,0,0,0,0,0,0,0, // RC channels (id 0 ->15)
                            192,1792,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // my channels (id 16 -> 31)
    char[] sendBuf = new char[25];
    private float[] mAxes = new float[AxesMapping.values().length];
    public boolean gamePadConnected = false;

    private WifiManager wifiManager;
    WifiReceiver receiverWifi;
    private InputManager mInputManager;
    ConnectTask connectTask = new ConnectTask();
    SendMessageTask sendMessageTask = new SendMessageTask();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_drone);

        Button btnSend = findViewById(R.id.btnSend);
        //editText = findViewById(R.id.editText);
        textView1 = findViewById(R.id.textView1);
        textView2 = findViewById(R.id.textView2);
        textView3 = findViewById(R.id.textView3);
        myLayout =  findViewById(R.id.my_layout);
        isConnected = findViewById(R.id.isConnected);

        myLayout.requestFocus();
        wifiManager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        receiverWifi = new WifiReceiver();
        registerReceiver(receiverWifi, intentFilter);

        mInputManager = (InputManager) getSystemService(Context.INPUT_SERVICE);

        for(int i : mInputManager.getInputDeviceIds()){
            if((mInputManager.getInputDevice(i).getSources() & InputDevice.SOURCE_GAMEPAD)!=0){
                gamePadConnected = true;
            }
        }

        if (!wifiManager.isWifiEnabled()) {
            Toast.makeText(getApplicationContext(), "Turning WiFi ON...", Toast.LENGTH_LONG).show();
            wifiManager.setWifiEnabled(true);
        }

        btnSend.setOnClickListener(view -> {
            channelsToSBUS();
            if (wifiManager.getConnectionInfo()!=null) {
                if (mTcpClient != null) {
                    Log.d(TAG, "onClick: SendMessageTask created");
                    //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sendBufTest);
                    /*textView1.setText("");
                    textView2.setText("");
                    for (int i=0; i<sendBufTest.length; i++){
                        textView1.append("sendBufTest = "+Integer.toBinaryString(sendBufTest[i])+"\n");
                    }*/
                }
            } else {
                Toast.makeText(getApplicationContext(),"No connected device", Toast.LENGTH_SHORT).show();
            }
        });

        mInputManager.registerInputDeviceListener(new InputManager.InputDeviceListener() {
            @Override
            public void onInputDeviceAdded(int i) {
                Log.d(TAG, "onInputDeviceAdded: "+mInputManager.getInputDevice(i));
                if((mInputManager.getInputDevice(i).getSources() & InputDevice.SOURCE_GAMEPAD)!=0) {
                    gamePadConnected = true;
                    if (mTcpClient != null && mTcpClient.mRun && sendMessageTask.getStatus() == AsyncTask.Status.PENDING) {
                        sendMessageTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                    }
                }
            }

            @Override
            public void onInputDeviceRemoved(int i) {
                Log.d(TAG, "onInputDeviceRemoved: "+mInputManager.getInputDevice(i));
                for(int j : mInputManager.getInputDeviceIds()){
                    if((mInputManager.getInputDevice(j).getSources() & InputDevice.SOURCE_GAMEPAD)!=0){
                        gamePadConnected = false;
                    }
                    //textView3.append(j+" ");
                }
            }

            @Override
            public void onInputDeviceChanged(int i) {
                Log.d(TAG, "onInputDeviceChanged: "+mInputManager.getInputDevice(i));
            }
        }, null);
    }


    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent ev) {
        InputDevice device = ev.getDevice();
        // Only care about game controllers.
        if (device != null) {
            for (AxesMapping axesMapping : AxesMapping.values()) {
                mAxes[axesMapping.ordinal()] = getCenteredAxis(ev, device, axesMapping.getMotionEvent());
            }
            updateChannels();
            textView1.setText("");
            textView2.setText("");
            for (int i=0; i<=15; i++){
                textView1.append("fcuChannels["+i+"] = "+(short) channels[i]+"\n");
                //textView2.append("["+i+"] = "+Integer.toBinaryString(channels[i])+"\n");
            }
            //textView2.setText(channelsToSBUS());
            channelsToSBUS();
            /*for (int i=16; i<=31; i++){
                textView2.append("myChannels["+(i-16)+"] = "+(short) channels[i]+"\n");
            }*/
            if (wifiManager.getConnectionInfo()!=null && mTcpClient != null) {
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sendBuf);
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, channels);
            }
            return true;
        }
        return super.dispatchGenericMotionEvent(ev);
    }

    void channelsToSBUS(){

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

        textView2.setText("");
        for (int k=0; k<sendBuf.length; k++) {
            textView2.append("packet["+k+"] = "+String.format("%02X\n",(byte) sendBuf[k]));
        }
    }


    float map(float x, float out_min, float out_max) {
        return (x + 1) * (out_max - out_min) /2 + out_min;
    }

    private void updateChannels() {

        float CHANNEL_RANGE_MIN = 192, CHANNEL_RANGE_MAX = 1792, CHANNEL_RANGE_CENTER = (CHANNEL_RANGE_MAX - CHANNEL_RANGE_MIN)/2;

        channels[0] = (char) Math.round(map(mAxes[0], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));
        channels[1] = (char) Math.round(map(mAxes[1], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));
        channels[2] = (char) Math.round(map(mAxes[2], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));
        channels[3] = (char) Math.round(map(mAxes[5], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));

        channels[16] = (char) Math.round(map(mAxes[4], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));
        channels[17] = (char) Math.round(map(mAxes[3], CHANNEL_RANGE_MIN, CHANNEL_RANGE_MAX));
        channels[18] = (char) (mAxes[6]+1);
        channels[19] = (char) (mAxes[7]+1);
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
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return super.onKeyUp(keyCode, event);
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
                    //textView2.setText("up "+keyCode);
                    channels[getButton(keyCode)] = 1;
                    //super.onKeyDown(keyCode, event);
                    break;
                case KeyEvent.ACTION_UP:
                    //textView1.setText("down "+keyCode);
                    channels[getButton(keyCode)] = 0;
                    //super.onKeyUp(keyCode, event);
                    break;
            }
            //channelsToPackets();
            textView1.setText("");
            //textView2.setText("");
            for (int i=0; i<=15; i++){
                textView1.append("fcuChannels["+i+"] = "+(short) channels[i]+"\n");
            }
            /*for (int i=16; i<=31; i++){
                textView2.append("myChannels["+(i-16)+"] = "+(short) channels[i]+"\n");
            }*/
            if (wifiManager.getConnectionInfo()!=null && mTcpClient != null) {
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sendBuf);
            }
        }
        return super.dispatchKeyEvent(event);
    }

    private int getButton(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A:         return 20; // Square
            case KeyEvent.KEYCODE_BUTTON_B:         return 21; // Cross
            case KeyEvent.KEYCODE_BUTTON_C:         return 22; // Circle
            case KeyEvent.KEYCODE_BUTTON_X:         return 23; // Triangle
            case KeyEvent.KEYCODE_BUTTON_Y:         return 24; // L1
            case KeyEvent.KEYCODE_BUTTON_Z:         return 25; // R1
            case KeyEvent.KEYCODE_BUTTON_L1:        return 26; // L2
            case KeyEvent.KEYCODE_BUTTON_R1:        return 27; // R2
            case KeyEvent.KEYCODE_BUTTON_SELECT:    return 28; // L3
            case KeyEvent.KEYCODE_BUTTON_L2:        return 29; // Share
            case KeyEvent.KEYCODE_BUTTON_R2:        return 30; // Option
            case KeyEvent.KEYCODE_BUTTON_THUMBL:    return 31; // Big central button
            default:                                return -1;
        }
    }

    /**
     * Sends a message using a background task to avoid doing long/network operations on the UI thread
     */
    public class SendMessageTask extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... voids) {
            Log.d(TAG, "SendMessageTask: executing");
            while (true) {
                try {
                    Thread.sleep(50);
                    //Log.d(TAG, "doInBackground: "+ Calendar.getInstance().getTime());
                } catch (InterruptedException e) {
                    e.printStackTrace();
                } finally {
                    mTcpClient.sendMessageByte(channels);
                }
                if (gamePadConnected) {
                    //mTcpClient.sendMessageByte(channels);
                }
            }
        }
    }


    public class ConnectTask extends AsyncTask<Integer, Integer, TcpClient> {
        @Override
        protected TcpClient doInBackground(Integer... message) {
            mTcpClient = new TcpClient(message1 -> {
                //this method calls the onProgressUpdate
                //publishProgress(message);
                Log.d(TAG, "messageReceived: AsyncTask launched");
            });
            while(true){
                while(!receiverWifi.networkInfo.isConnected());
                Log.d(TAG, "doInBackground: running connect task");
                isConnected.setImageDrawable(getResources().getDrawable(R.drawable.circle_green));
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
        public NetworkInfo networkInfo;

        //WifiManager wifiManager;

        public WifiReceiver(){//WifiManager wifiManager) {

            //this.wifiManager = wifiManager;
        }

        public void onReceive(Context context, Intent intent) {

            //String action = intent.getAction();

            //if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
            //Log.d(TAG, "onReceive: "+networkInfo);
            if (networkInfo.isConnected()) {
                IP_addr = Formatter.formatIpAddress(wifiManager.getDhcpInfo().serverAddress);
                if(mTcpClient==null){
                    connectTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                    if(gamePadConnected && sendMessageTask.getStatus() == AsyncTask.Status.PENDING){
                        while (mTcpClient==null);
                        while (!mTcpClient.mRun);
                        sendMessageTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                    }
                }
            } else {
                if (mTcpClient!=null){
                    isConnected.setImageDrawable(getResources().getDrawable(R.drawable.circle_red));
                    mTcpClient.stopClient();
                }
            }
        }
    }
}
