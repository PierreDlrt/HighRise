package dlrt.pierre.highrise;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.NetworkInfo;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.text.format.Formatter;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class DroneActivity extends AppCompatActivity {

    public static final String TAG = DroneActivity.class.getSimpleName();

    TcpClient mTcpClient;
    //TextView conStatus;
    EditText editText;

    int cpt=0;
    public static String IP_addr;
    private int buf[] = new int[1000];

    private WifiManager wifiManager;
    WifiReceiver receiverWifi;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_drone);

        final ConnectTask connectTask = new ConnectTask();

        Button btnConnect = findViewById(R.id.btnConnect);
        Button btnDisconnect = findViewById(R.id.btnDisconnect);
        Button btnSend = findViewById(R.id.btnSend);
        editText = findViewById(R.id.editText);
        //conStatus = findViewById(R.id.conStatus);

        wifiManager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        receiverWifi = new WifiReceiver();
        registerReceiver(receiverWifi, intentFilter);

        if (!wifiManager.isWifiEnabled()) {
            Toast.makeText(getApplicationContext(), "Turning WiFi ON...", Toast.LENGTH_LONG).show();
            wifiManager.setWifiEnabled(true);
        }

        //while (wifiManager.getConnectionInfo()!=null)

        btnConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (wifiManager.getConnectionInfo()!=null) {
                    IP_addr = receiverWifi.IP_ADDR;
                    Log.d(TAG, "onClick: Button clicked");
                    if (!connectTask.getStatus().equals(AsyncTask.Status.RUNNING)) {
                        Log.d(TAG, "onClick: connectTask execution");
                        connectTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                    }
                } else{
                    Toast.makeText(getApplicationContext(),"No connected device", Toast.LENGTH_SHORT).show();
                }
            }
        });

        btnDisconnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.d(TAG, "onClick: disconnect clicked");
                if(connectTask.getStatus().equals(AsyncTask.Status.RUNNING) && mTcpClient!=null){
                    Log.d(TAG, "onClick: cancel task");
                    mTcpClient.stopClient();
                }
            }
        });

        btnSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (wifiManager.getConnectionInfo()!=null) {
                    IP_addr = receiverWifi.IP_ADDR;
                    String message = editText.getText().toString();
                    if (mTcpClient != null) {
                        Log.d(TAG, "onClick: SendMessageTask created");
                        new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, message);
                    }
                    editText.setText("");
                } else {
                    Toast.makeText(getApplicationContext(),"No connected device", Toast.LENGTH_SHORT).show();
                }
            }
        });
    }

    /**
     * Sends a message using a background task to avoid doing long/network operations on the UI thread
     */
    public class SendMessageTask extends AsyncTask<String, Void, Void> {
        @Override
        protected Void doInBackground(String... params) {
            Log.d(TAG, "SendMessageTask: executing");
            mTcpClient.sendMessage(params[0]);

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
                IP_ADDR = Formatter.formatIpAddress(wifiManager.getDhcpInfo().serverAddress);
            } else {
                if (mTcpClient!=null){
                    mTcpClient.stopClient();
                }
            }
        }
    }
}
