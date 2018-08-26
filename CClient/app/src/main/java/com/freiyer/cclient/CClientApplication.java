package com.freiyer.cclient;

import android.app.Application;
import android.os.Build;
import android.support.annotation.NonNull;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashMap;
import java.util.Map;

public class CClientApplication extends Application {

    public static final String LOG_TAG = "CClientApplication";
    public static final int MODE_TOUCHSCREEN = 1;
    public static final int MODE_TOUCHPAD = 2;

    public String getModeName(int mode) {
        if(mode == MODE_TOUCHSCREEN)
            return getString(R.string.touchscreen);
        else
            return getString(R.string.touchpad);
    }

    public interface OnClientConnectListener {
        void onSuccess(CClientApplication app, Socket socket);
        void onFail(CClientApplication app);
    }

    private static final int PORT = 19754;
    private Socket socket;
    private String serverName;
    private OnClientConnectListener listener;
    private int mode;

    public CClientApplication() {
        socket = null;
        serverName = null;
    }

    public String getServerName() {
        return serverName;
    }

    public int getMode() {
        return mode;
    }

    public void setListener(OnClientConnectListener listener) {
        this.listener = listener;
    }

    public void initializeNetwork(final String addr, int mode) {
        final CClientApplication app = this;
        this.mode = mode;
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    socket = new Socket(addr, PORT);
                    // Get server name
                    serverName = ((StringPacket) nextPacket()).getContent();
                    // Send client name and mode
                    if(!sendPacket(new StringPacket(Build.MODEL)))
                        throw new Exception();
                    if(!sendPacket(new TouchPacket(app.mode, 0.0f, 0.0f, 0)))
                        throw new Exception();

                    if (listener != null)
                        listener.onSuccess(app, socket);
                } catch (Exception e) {
                    Log.d(LOG_TAG, e.getMessage());
                    if (listener != null)
                        listener.onFail(app);
                }
            }
        }).start();
    }

    public void destroyNetwork() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if(socket != null)
                    try {
                        socket.getOutputStream().write(new byte[0]);
                        socket.close();
                    } catch (Exception e) {
                        Log.d(LOG_TAG, e.getMessage());
                    }
                socket = null;
            }
        }).start();
    }

    public Packet nextPacket() {
        if(socket == null)
            return null;
        Packet rtn = null;
        Log.d(LOG_TAG, "nextPacket");
        try {
            InputStream istream = socket.getInputStream();
            byte[] magicNumberBuff = new byte[4];
            istream.read(magicNumberBuff);
            int magicNumber = ByteBuffer.wrap(magicNumberBuff)
                    .order(ByteOrder.LITTLE_ENDIAN).getInt();
            Log.d(LOG_TAG, Integer.toString(magicNumber));
            switch(magicNumber) {
                case StringPacket.MAGIC_NUMBER:
                    rtn = nextStringPacket();
                    break;
                case TouchPacket.MAGIC_NUMBER:
                    rtn = nextTouchPacket();
                    break;
            }
        } catch (Exception e) {
            Log.d(LOG_TAG, e.getMessage());
        }
        return rtn;
    }

    private TouchPacket nextTouchPacket() throws Exception {
        InputStream istream = socket.getInputStream();
        byte[] buff = new byte[4];

        if(istream.read(buff) != 4)
            throw new Exception("Failed to read idx.");
        int idx = ByteBuffer.wrap(buff).order(ByteOrder.LITTLE_ENDIAN).getInt();

        if(istream.read(buff) != 4)
            throw new Exception("Failed to read x.");
        float x = ByteBuffer.wrap(buff).order(ByteOrder.LITTLE_ENDIAN).getFloat();

        if(istream.read(buff) != 4)
            throw new Exception("Failed to read y.");
        float y = ByteBuffer.wrap(buff).order(ByteOrder.LITTLE_ENDIAN).getFloat();

        if(istream.read(buff) != 4)
            throw new Exception("Failed to read y.");
        int down = ByteBuffer.wrap(buff).order(ByteOrder.LITTLE_ENDIAN).getInt();

        return new TouchPacket(idx, x, y, down);
    }

    @NonNull
    private StringPacket nextStringPacket() throws Exception {
        InputStream istream = socket.getInputStream();
        byte[] buff = new byte[4];

        if(istream.read(buff) != 4)
            throw new Exception("Failed to read the length.");
        int len = ByteBuffer.wrap(buff).order(ByteOrder.LITTLE_ENDIAN).getInt();

        byte[] contentBuff = new byte[len];
        int tlen = 0, offset = 0;
        Log.d(LOG_TAG, Integer.toString(len));
        while((tlen = istream.read(contentBuff, offset, len)) > 0) {
            len -= tlen;
            offset += tlen;
        }
        String content = new String(contentBuff, "UTF-8");
        return new StringPacket(content);
    }

    public boolean sendPacket(Packet packet) {
        if(packet == null || socket == null)
            return false;

        try {
            socket.getOutputStream().write(
                    ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
                            .putInt(packet.getMagicNumber()).array()
            );
            byte[] buff;
            while((buff = packet.next()) != null) {
                    socket.getOutputStream().write(buff);
            }
        } catch(Exception e) {
            Log.d(LOG_TAG, e.getMessage());
            return false;
        }
        return true;
    }

}
