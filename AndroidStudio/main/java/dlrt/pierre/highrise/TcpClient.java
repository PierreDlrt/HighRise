package dlrt.pierre.highrise;

import android.app.Activity;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.text.format.Formatter;
import android.util.Log;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.Socket;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;


public class TcpClient {
    public static final String TAG = TcpClient.class.getSimpleName();
    public String SERVER_IP;// = "192.168.0.100";
    public static final int SERVER_PORT = 5001;
    private int mServerMessage;
    // sends message received notifications
    private OnMessageReceived mMessageListener = null;
    // while this is true, the server will continue running
    public boolean mRun = false;
    // used to send messages
    private PrintWriter mBufferOut;
    // used to read messages from the server
    private BufferedReader mBufferIn;


    public TcpClient(OnMessageReceived listener) {
        mMessageListener = listener;
    }


    public void sendMessage(final String message) {
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "run: sendMessage running");
                if (mBufferOut != null) {
                    Log.d(TAG, "Sending: " + message);
                    mBufferOut.println(message);
                    mBufferOut.flush();
                }
            }
        };
        Thread thread = new Thread(runnable);
        thread.start();
    }

    /**
     * Close the connection and release the members
     */
    public void stopClient() {
        mRun = false;

        if (mBufferOut != null) {
            mBufferOut.flush();
            mBufferOut.close();
        }

        mMessageListener = null;
        mBufferIn = null;
        mBufferOut = null;
        //mServerMessage = null;
    }

    public void run() {

        mRun = true;
        SERVER_IP = DroneActivity.IP_addr;
        //Integer.toString( //wifiManager.getConnectionInfo().getIpAddress());
        Log.d(TAG, "run: IP="+SERVER_IP);

        try {
            //here you must put your computer's IP address.
            InetAddress serverAddr = InetAddress.getByName(SERVER_IP);
            Log.d("TCP Client", "C: Connecting...");
            //create a socket to make the connection with the server
            Socket socket = new Socket(serverAddr, SERVER_PORT);
            try {

                Log.d(TAG, "run: socket created");
                //sends the message to the server
                mBufferOut = new PrintWriter(new BufferedWriter(new OutputStreamWriter(socket.getOutputStream())), true);
                //receives the message which the server sends back
                mBufferIn = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                //in this while the client listens for the messages sent by the server
                while (mRun) {
                    //Log.d(TAG, "run: entered mRun");
                    if(mBufferIn.ready()) {
                        //Log.d(TAG, "run: ready");
                        //Log.d(TAG, "run: read:"+mBufferIn.read());
                        mServerMessage = mBufferIn.read();
                        if (mServerMessage != -1 && mMessageListener != null) {
                            //call the method messageReceived from MyActivity class
                            //Log.d(TAG, "run: message received");
                            mMessageListener.messageReceived(mServerMessage);
                        }
                        //Log.d(TAG, "run: message null");
                    }
                }
            } catch (Exception e) {
                Log.e("TCP", "S: Error", e);
            } finally {
                //the socket must be closed. It is not possible to reconnect to this socket
                // after it is closed, which means a new socket instance has to be created.
                socket.close();
            }
        } catch (Exception e) {
            Log.e("TCP", "C: Error", e);
        }
    }

    //Declare the interface. The method messageReceived(String message) will must be implemented in the Activity
    //class at on AsyncTask doInBackground
    public interface OnMessageReceived {
        public void messageReceived(int message);
    }
}
