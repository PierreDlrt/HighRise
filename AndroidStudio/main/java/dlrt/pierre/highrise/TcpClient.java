package dlrt.pierre.highrise;

import android.util.Log;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.io.InputStreamReader;
import java.util.Calendar;

import android.os.Handler;


public class TcpClient {
    public static final String TAG = TcpClient.class.getSimpleName();
    public String SERVER_IP;// = "192.168.0.100";
    public static final int SERVER_PORT = 5001;
    private int mServerMessage;
    // sends message received notifications
    private OnMessageReceived mMessageListener = null;
    public boolean mRun = false;
    private DataOutputStream mBufferOut;
    private BufferedReader mBufferIn;

    private Handler mHandler;

    public TcpClient(OnMessageReceived listener) {
        mMessageListener = listener;
    }


    public void sendMessageByte(final char[] message) {
        if (mBufferOut != null) {
            byte[] b = new byte[message.length*2];
            for (int i=0; i<message.length; i++){
                b[2*i] = (byte) message[i];
                b[2*i+1] = (byte) (message[i] >> 8);
                //Log.d(TAG, "sendMessageByte "+i+": "+(int) message[i]);
                //Log.d(TAG, "sendMessageByte: "+message.length);
            }
            try {
                mBufferOut.write(b);
                mBufferOut.flush();
            } catch (IOException e){
                e.printStackTrace();
            }
            //Log.d(TAG, "run: "+ Calendar.getInstance().getTime());
        }
    }


    /**
     * Close the connection and release the members
     */
    public void stopClient() {
        mRun = false;

        if (mBufferOut != null) {
            try {
                mBufferOut.flush();
                mBufferOut.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        mMessageListener = null;
        mBufferIn = null;
        mBufferOut = null;
        //mServerMessage = null;
    }

    public void run() {

        mHandler = new Handler();
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
            socket.setTcpNoDelay(true);
            //Log.d(TAG, "run: delay :"+socket.getTcpNoDelay());
            //Log.d(TAG, "run: send buf size: "+socket.getSendBufferSize());
            try {
                mBufferOut = new DataOutputStream(socket.getOutputStream());
                mBufferIn = new BufferedReader(new InputStreamReader(socket.getInputStream()));

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