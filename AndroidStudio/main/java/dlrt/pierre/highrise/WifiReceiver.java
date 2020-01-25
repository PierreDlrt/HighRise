package dlrt.pierre.highrise;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.text.format.Formatter;

class WifiReceiver extends BroadcastReceiver {

    private static final String TAG = "WifiReceiver";
    public String IP_ADDR;

    WifiManager wifiManager;

    public WifiReceiver(WifiManager wifiManager) {

        this.wifiManager = wifiManager;
    }

    public void onReceive(Context context, Intent intent) {

        String action = intent.getAction();

        if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
            if(networkInfo.isConnected()) {
                IP_ADDR = Formatter.formatIpAddress(wifiManager.getDhcpInfo().serverAddress);
            }
        }
    }
}