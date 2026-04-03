package espdislink;

/**
 * 协议操作（op）的最小接口：可序列化为 payload 字节。
 */
public interface CharPacket {
    byte[] toBytes();
}
