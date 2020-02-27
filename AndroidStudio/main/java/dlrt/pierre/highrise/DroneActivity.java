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
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class DroneActivity extends AppCompatActivity {

    public static final String TAG = DroneActivity.class.getSimpleName();

    TcpClient mTcpClient;
    TextView textView1, textView2, textView3;
    LinearLayout myLayout;

    int cpt=0;
    public static String IP_addr;
    private char[] channels = {1500,1500,1500,1000,0,0,0,0,0,0,0,0,0,0,0,0, // RC channels (id 0 ->15)
                            1000,1500,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // my channels (id 16 -> 31)
    //private char[] sendBufTest = {15, 127, 128, 136, 195, 255, 240, 170};
    private char[] sendBufTest = {1, 255, 0xAA55, 256, 0xCCCC, 0, 0xFFFF};
    private float[] mAxes = new float[AxesMapping.values().length];
    public boolean gamePadConnected = true;

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

        myLayout.requestFocus();
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
            }
        });

        mInputManager.registerInputDeviceListener(new InputManager.InputDeviceListener() {
            @Override
            public void onInputDeviceAdded(int i) {
                Log.d(TAG, "onInputDeviceAdded: "+mInputManager.getInputDevice(i));
                /*if((mInputManager.getInputDevice(i).getSources() & InputDevice.SOURCE_GAMEPAD)!=0) {
                    gamePadConnected = true;
                    if (mTcpClient != null && mTcpClient.mRun && sendMessageTask.getStatus() == AsyncTask.Status.PENDING) {
                        sendMessageTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                    }
                }*/
            }

            @Override
            public void onInputDeviceRemoved(int i) {
                Log.d(TAG, "onInputDeviceRemoved: "+mInputManager.getInputDevice(i));
                /*if((mInputManager.getInputDevice(i).getSources() & InputDevice.SOURCE_GAMEPAD)!=0) {
                    //gamePadConnected = false;
                }*/
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
            }
            for (int i=16; i<=31; i++){
                textView2.append("myChannels["+(i-16)+"] = "+(short) channels[i]+"\n");
            }
            if (wifiManager.getConnectionInfo()!=null && mTcpClient != null) {
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sendBuf);
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, channels);
            }
            return true;
        }
        return super.dispatchGenericMotionEvent(ev);
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
                    break;
                case KeyEvent.ACTION_UP:
                    //textView1.setText("down "+keyCode);
                    channels[getButton(keyCode)] = 0;
                    break;
            }
            //channelsToPackets();
            textView1.setText("");
            textView2.setText("");
            for (int i=0; i<=15; i++){
                textView1.append("fcuChannels["+i+"] = "+(short) channels[i]+"\n");
            }
            for (int i=16; i<=31; i++){
                textView2.append("myChannels["+(i-16)+"] = "+(short) channels[i]+"\n");
            }
            if (wifiManager.getConnectionInfo()!=null && mTcpClient != null) {
                //new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, sendBuf);
            }
        }
        return super.dispatchKeyEvent(event);
    }

    private int getButton(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A: // Square
                return 20;
            case KeyEvent.KEYCODE_BUTTON_B: // Cross
                return 21;
            case KeyEvent.KEYCODE_BUTTON_C: // Circle
                return 22;
            case KeyEvent.KEYCODE_BUTTON_X: // Triangle
                return 23;
            case KeyEvent.KEYCODE_BUTTON_Y: // L1
                return 24;
            case KeyEvent.KEYCODE_BUTTON_Z: // R1
                return 25;
            case KeyEvent.KEYCODE_BUTTON_L1: // L2
                return 26;
            case KeyEvent.KEYCODE_BUTTON_R1: // R2
                return 27;
            case KeyEvent.KEYCODE_BUTTON_SELECT: // L3
                return 28;
            case KeyEvent.KEYCODE_BUTTON_L2: // Share
                return 29;
            case KeyEvent.KEYCODE_BUTTON_R2: // Option
                return 30;
            case KeyEvent.KEYCODE_BUTTON_THUMBL: // Big central button
                return 31;
            default:
                return -1;
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
                //textView3.setText(params[0][0]);
                //cpt++;
                //textView3.setText(cpt+" messages sent\nchannels[0]= "+channels[0]);
                //while (!gamePadConnected);
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                mTcpClient.sendMessageByte(channels);
                //return null;
            }
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
                }
            });
            while(true){
                while(wifiManager.getConnectionInfo()==null);
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
                    if(sendMessageTask.getStatus() == AsyncTask.Status.PENDING){
                        while (mTcpClient==null);
                        while (!mTcpClient.mRun);
                        sendMessageTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                    }
                }
            } else {
                if (mTcpClient!=null){
                    mTcpClient.stopClient();
                }
            }
        }
    }
}
