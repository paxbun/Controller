package com.freiyer.cclient;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class TouchPacket extends Packet {
    private int idx;
    private float x;
    private float y;
    private int down;
    private int nextInvoked;

    public static final int MAGIC_NUMBER = 0x544F4348;

    public TouchPacket() {
        super(MAGIC_NUMBER);
        this.idx = -1;
        this.x = 0.0f;
        this.y = 0.0f;
        this.down = 0;
        this.nextInvoked = 0;
    }

    public TouchPacket(int idx, float x, float y, int down) {
        super(MAGIC_NUMBER);
        this.idx = idx;
        this.x = x;
        this.y = y;
        this.down = down;
        this.nextInvoked = 0;
    }


    @Override
    public byte[] next() {
        switch(nextInvoked++) {
            case 0: return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
                    .putInt(idx).array();
            case 1: return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
                    .putFloat(x).array();
            case 2: return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
                    .putFloat(y).array();
            case 3: return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
                    .putInt(down).array();
        }
        return null;
    }

    public int getIdx() {
        return idx;
    }

    public float getX() {
        return x;
    }

    public float getY() {
        return y;
    }

    public int getDown() { return down; }
}
