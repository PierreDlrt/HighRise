package dlrt.pierre.highrise;

import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class DroneActivity extends AppCompatActivity {

    public static final String TAG = DroneActivity.class.getSimpleName();

    TcpClient mTcpClient;
    TextView tv;
    EditText editText;

    public static String IP_addr;

    private int buf[] = new int[1000];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_drone);

        final ConnectTask connectTask = new ConnectTask();
        Button btnConnect = findViewById(R.id.btnConnect);
        Button btnSend = findViewById(R.id.btnSend);
        tv = findViewById(R.id.textView);
        editText = findViewById(R.id.editText);

        Intent intent = getIntent();
        IP_addr = intent.getStringExtra("IP_ADDR");

        btnConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (!connectTask.getStatus().equals(AsyncTask.Status.RUNNING)){
                    connectTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                    // connectTask.execute();
                }
            }
        });

        btnSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String message = editText.getText().toString();
                //sends the message to the server
                if (mTcpClient != null) {
                    Log.d(TAG, "onClick: SendMessageTask created");
                    //new SendMessageTask().execute(message);
                    new SendMessageTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, message);
                }
                editText.setText("");
            }
        });
    }

    /**
     * Sends a message using a background task to avoid doing long/network operations on the UI thread
     */
    public class SendMessageTask extends AsyncTask<String, Void, Void> {

        @Override
        protected Void doInBackground(String... params) {

            // send the message
            Log.d(TAG, "SendMessageTask: executing");
            mTcpClient.sendMessage(params[0]);

            return null;
        }
    }

    public class ConnectTask extends AsyncTask<Integer, Integer, TcpClient> {

        @Override
        protected TcpClient doInBackground(Integer... message) {

            //we create a TCPClient object
            mTcpClient = new TcpClient(new TcpClient.OnMessageReceived() {
                @Override
                //here the messageReceived method is implemented
                public void messageReceived(int message) {
                    //this method calls the onProgressUpdate
                    publishProgress(message);
                    Log.d(TAG, "messageReceived: "+message);

                }
            });
            mTcpClient.run();

            return null;
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

}
