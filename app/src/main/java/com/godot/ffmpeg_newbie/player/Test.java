package com.godot.ffmpeg_newbie.player;

import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * @Desc 说明
 * @Author Godot
 * @Date 2019-12-13
 * @Version 1.0
 * @Mail 809373383@qq.com
 */
public class Test {
    public static void main(String[] args) throws NoSuchAlgorithmException {
        System.out.println(
                signature((byte) 1, 492, getMd5Bytes("931ace17856d640fae3f21339e1fcad5"), Long.parseLong("15084841576"),
                getMd5Bytes("hello world")));

    }
    public static String signature(byte platform, int appVersion, byte[] deviceNoMd5, long phone, byte[] key) throws NoSuchAlgorithmException {
        ByteBuffer byteBuffer = ByteBuffer.allocate(45);
        byteBuffer.put(platform);
        byteBuffer.putInt(appVersion);
        byteBuffer.put(deviceNoMd5);
        byteBuffer.putLong(phone);
        byteBuffer.put(key);

        System.out.println(toHex(byteBuffer.array()));

        return toHex(getMd5(byteBuffer.array()));
    }

    public static byte[] getMd5Bytes(String value) throws NoSuchAlgorithmException {
        MessageDigest md5 = MessageDigest.getInstance("MD5");
        return md5.digest(value.getBytes());
    }
    public static byte[] getMd5(byte[] bytes) throws NoSuchAlgorithmException {
        MessageDigest md5 = MessageDigest.getInstance("MD5");
        return md5.digest(bytes);
    }
    private static final char m_hexCodes[] = { '0', '1', '2', '3', '4', '5',
            '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    private static final int m_shifts[] = { 60, 56, 52, 48, 44, 40, 36, 32, 28,
            24, 20, 16, 12, 8, 4, 0 };



    private static String toHex(final long value, final int digitNum) {
        StringBuilder result = new StringBuilder(digitNum);

        for (int j = 0; j < digitNum; j++) {
            int index = (int) ((value >> m_shifts[j + (16 - digitNum)]) & 15);
            result.append(m_hexCodes[index]);
        }

        return result.toString();
    }

    public static String toHex(final byte value) {
        return toHex(value, 2);
    }

    public static String toHex(final short value) {
        return toHex(value, 4);
    }

    public static String toHex(final int value) {
        return toHex(value, 8);
    }

    public static String toHex(final long value) {
        return toHex(value, 16);
    }

    public static String toHex(final byte[] value) {
        return toHex(value, 0, value.length);
    }


    public static String toHex(final byte[] value, final int offset,
                               final int length) {
        StringBuilder retVal = new StringBuilder();

        int end = offset + length;
        for (int x = offset; x < end; x++) {
            retVal.append(toHex(value[x]));
        }

        return retVal.toString();
    }
}
