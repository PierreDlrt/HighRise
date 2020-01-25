package dlrt.pierre.highrise;



import android.content.Intent;
import android.net.wifi.WifiManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.content.Context;
import android.widget.Button;
import android.widget.Toast;


public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private WifiManager wifiManager;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button buttonStart = findViewById(R.id.btnStart);

        wifiManager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);

        buttonStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                if (!wifiManager.isWifiEnabled()) {
                    Toast.makeText(getApplicationContext(), "Turning WiFi ON...", Toast.LENGTH_LONG).show();
                    wifiManager.setWifiEnabled(true);
                }

                Intent myIntent = new Intent(getApplicationContext(), DroneActivity.class);
                getApplication().startActivity(myIntent);
            }
        });
    }
}
