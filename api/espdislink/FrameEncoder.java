package espdislink;

import java.io.ByteArrayOutputStream;
import java.util.Objects;

/**
 * 协议/封包层：将渲染层生成的操作（ops）封装为固件可识别的数据帧。
 *
 * 线协议（需与固件保持一致）：
 * - 0..1：固定头 0xAA 0x55
 * - 2：   fullLen（uint8），表示整包长度（包含头+长度字节+帧间隔+payload）
 * - 3..4：frameIntervalMs（uint16，大端），帧间隔（ms）
 * - 5.. ：payload（由若干操作组成）
 */
public final class FrameEncoder {
    public static final byte HDR1 = (byte) 0xAA;
    public static final byte HDR2 = (byte) 0x55;

    private FrameEncoder() {
    }

    /** 心跳包：AA 55 04 02 */
    public static byte[] encodeHeartbeat() {
        return new byte[]{HDR1, HDR2, 0x04, 0x02};
    }

    /**
     * TONE 命令包：AA 55 0B 20 02 seq freqH freqL durH durL volume
     */
    public static byte[] encodeToneCommand(int seq, int frequencyHz, int durationMs, int volume) {
        if (seq < 0 || seq > 0xFF) {
            throw new IllegalArgumentException("seq 超出范围: " + seq);
        }
        if (frequencyHz <= 0 || frequencyHz > 0xFFFF) {
            throw new IllegalArgumentException("frequencyHz 超出范围: " + frequencyHz);
        }
        if (durationMs <= 0 || durationMs > 0xFFFF) {
            throw new IllegalArgumentException("durationMs 超出范围: " + durationMs);
        }
        if (volume < 0 || volume > 100) {
            throw new IllegalArgumentException("volume 超出范围(0-100): " + volume);
        }

        return new byte[]{
                HDR1,
                HDR2,
                0x0B,
                0x20,
                0x02,
                (byte) (seq & 0xFF),
                (byte) ((frequencyHz >> 8) & 0xFF),
                (byte) (frequencyHz & 0xFF),
                (byte) ((durationMs >> 8) & 0xFF),
                (byte) (durationMs & 0xFF),
                (byte) (volume & 0xFF)
        };
    }

    public static byte[] encodeFrame(int frameIntervalMs, CharPacket... ops) {
        if (frameIntervalMs < 0 || frameIntervalMs > 0xFFFF) {
            throw new IllegalArgumentException("frameIntervalMs 超出范围: " + frameIntervalMs);
        }

        Objects.requireNonNull(ops, "ops");

        ByteArrayOutputStream payload = new ByteArrayOutputStream();
        for (CharPacket op : ops) {
            if (op == null) {
                throw new IllegalArgumentException("op 不能为 null");
            }
            byte[] bytes = op.toBytes();
            if (bytes == null) {
                throw new IllegalArgumentException(op.getClass().getName() + ".toBytes() 返回 null");
            }
            payload.writeBytes(bytes);
        }

        int fullLen = 5 + payload.size();
        if (fullLen > 255) {
            throw new IllegalArgumentException("单帧过大（length 为 uint8），fullLen=" + fullLen);
        }

        byte[] packet = new byte[fullLen];
        packet[0] = HDR1;
        packet[1] = HDR2;
        packet[2] = (byte) (fullLen & 0xFF);
        packet[3] = (byte) ((frameIntervalMs >> 8) & 0xFF);
        packet[4] = (byte) (frameIntervalMs & 0xFF);

        byte[] payloadBytes = payload.toByteArray();
        System.arraycopy(payloadBytes, 0, packet, 5, payloadBytes.length);
        return packet;
    }
}
