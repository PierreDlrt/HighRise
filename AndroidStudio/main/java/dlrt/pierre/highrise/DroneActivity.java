package dlrt.pierre.highrise;

import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class DroneActivity extends AppCompatActivity {

    TcpClient mTcpClient;
    TextView tv;

    public static String IP_addr;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_drone);

        final ConnectTask connectTask = new ConnectTask();
        Button btnConnect = findViewById(R.id.btnConnect);
        Button btnSend = findViewById(R.id.btnSend);
        tv = findViewById(R.id.textView);

        Intent intent = getIntent();
        IP_addr = intent.getStringExtra("IP_ADDR");

        btnConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                connectTask.execute("");
            }
        });

        btnSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mTcpClient != null) {
                    mTcpClient.sendMessage("testing");
                }
            }
        });
    }

    public class ConnectTask extends AsyncTask<String, String, TcpClient> {

        @Override
        protected TcpClient doInBackground(String... message) {

            //we create a TCPClient object
            mTcpClient = new TcpClient(new TcpClient.OnMessageReceived() {
                @Override
                //here the messageReceived method is implemented
                public void messageReceived(String message) {
                    //this method calls the onProgressUpdate
                    publishProgress(message);
                }
            });
            mTcpClient.run();

            return null;
        }

        @Override
        protected void onProgressUpdate(String... values) {
            super.onProgressUpdate(values);
            //response received from server
            Log.d("test", "response " + values[0]);
            tv.setText(values[0]);
            //process server response here....
        }
    }

}
