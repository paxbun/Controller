package com.freiyer.cclient;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class StringPacket extends Packet {
    private String content;
    private int nextInvoked;

    public static final int MAGIC_NUMBER = 0x53545247;

    public StringPacket() {
        super(MAGIC_NUMBER);
        this.content = null;
        this.nextInvoked = 0;
    }

    public StringPacket(String content) {
        super(MAGIC_NUMBER);
        this.content = content;
        this.nextInvoked = 0;
    }

    @Override
    public byte[] next() {
        switch(nextInvoked++)
        {
            case 0: return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
                    .putInt(content.length()).array();
            case 1: return content.getBytes();
        }
        return null;
    }

    public String getContent() {
        return content;
    }
}
