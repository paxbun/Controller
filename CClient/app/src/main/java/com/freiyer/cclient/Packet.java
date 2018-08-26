package com.freiyer.cclient;

public abstract class Packet {
    protected int magicNumber;
    public abstract byte[] next();

    public Packet(int magicNumber) {
        this.magicNumber = magicNumber;
    }

    public int getMagicNumber() {
        return magicNumber;
    }
}

