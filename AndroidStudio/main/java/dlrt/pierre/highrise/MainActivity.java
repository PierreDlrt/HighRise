package dlrt.pierre.highrise;


import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.wifi.ScanResult;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.os.Build;
import android.widget.AdapterView;
import android.widget.ListView;
import android.content.Context;
import android.content.IntentFilter;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    public static int IP_addr;

    private ListView wifiList;
    private WifiManager wifiManager;
    WifiReceiver receiverWifi;

    String networkSSID, networkPass = "pierre12";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button buttonScan = findViewById(R.id.btnFindUnpairedDevices);

        wifiList = findViewById(R.id.lvNewDevices);
        wifiManager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        receiverWifi = new WifiReceiver(wifiManager, wifiList);

        final WifiConfiguration conf = new WifiConfiguration();

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        registerReceiver(receiverWifi, intentFilter);

        if (!wifiManager.isWifiEnabled()) {
            Toast.makeText(getApplicationContext(), "Turning WiFi ON...", Toast.LENGTH_LONG).show();
            wifiManager.setWifiEnabled(true);
        }
        buttonScan.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Toast.makeText(MainActivity.this, "start scanning", Toast.LENGTH_LONG).show();
                getWifi();
            }
        });

        wifiList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                networkSSID =  (String) adapterView.getAdapter().getItem(i);
                conf.SSID = "\"" + networkSSID + "\"";
                conf.preSharedKey = "\""+ networkPass +"\"";
                int netId = wifiManager.addNetwork(conf);
                wifiManager.disconnect();
                wifiManager.enableNetwork(netId, true);
                wifiManager.reconnect();
                Toast.makeText(MainActivity.this,"connecting", Toast.LENGTH_SHORT).show();
            }
        });
    }


    private void getWifi() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(MainActivity.this, new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, 1);
            } else {
                wifiManager.startScan();
            }
        } else {
            wifiManager.startScan();
        }
    }


    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            case 1:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                wifiManager.startScan();
            } else {
                return;
            }
            break;
        }
    }

    /*@Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(receiverWifi);
    }*/

    /*@Override
    protected void onPostResume() {
        super.onPostResume();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        registerReceiver(receiverWifi, intentFilter);
        wifiManager.startScan();
    }*/

}
