package dlrt.pierre.highrise;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.Parcelable;
import android.text.format.Formatter;
import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;
import java.util.ArrayList;
import java.util.List;

class WifiReceiver extends BroadcastReceiver {

    private static final String TAG = "WifiReceiver";

    WifiManager wifiManager;
    ListView wifiDeviceList;

    public WifiReceiver(WifiManager wifiManager, ListView wifiDeviceList) {

        this.wifiManager = wifiManager;
        this.wifiDeviceList = wifiDeviceList;
    }

    public void onReceive(Context context, Intent intent) {

        String action = intent.getAction();
        Log.d(TAG, "onReceive: "+action);
        if (WifiManager.SCAN_RESULTS_AVAILABLE_ACTION.equals(action)) {
            List<ScanResult> wifiList = wifiManager.getScanResults();

            ArrayList<String> deviceList = new ArrayList<>();
            for (ScanResult scanResult : wifiList) {
                deviceList.add(scanResult.SSID);// + " - " + scanResult.capabilities);
            }
            ArrayAdapter arrayAdapter = new ArrayAdapter(context, android.R.layout.simple_list_item_1, deviceList.toArray());
            wifiDeviceList.setAdapter(arrayAdapter);
        }
        if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            //Log.d(TAG, "onReceive: entered NETWORK_STATE_CHANGED_ACTION 1");
            NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
            if(networkInfo.isConnected()) {
                //Log.d(TAG, "onReceive: entered NETWORK_STATE_CHANGED_ACTION 2");
                Intent myIntent = new Intent(context, DroneActivity.class);
                //myIntent.putExtra("WifiManager", (Parcelable) WifiManager);
                myIntent.putExtra("IP_ADDR", Formatter.formatIpAddress(wifiManager.getDhcpInfo().serverAddress)); // wifiManager.getConnectionInfo().getIpAddress()));
                Log.d(TAG, "onReceive: Server IP="+Formatter.formatIpAddress(wifiManager.getDhcpInfo().serverAddress));
                //int ipDec = wifiManager.getConnectionInfo().getIpAddress();
                context.startActivity(myIntent);
            }
            else {
                Toast.makeText(context, "Error : unable to connect", Toast.LENGTH_SHORT).show();
            }
        }
    }
}