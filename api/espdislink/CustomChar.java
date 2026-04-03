package espdislink;

/**
 * 自定义字符（8 行点阵）。
 * payload 格式：0x01 + 8 字节点阵。
 */
public class CustomChar implements CharPacket {
    private final byte[] dotMatrix;

    public CustomChar(byte[] dotMatrix) {
        if (dotMatrix == null || dotMatrix.length != 8) {
            throw new IllegalArgumentException("自定义字符需要 8 字节，当前为 " + (dotMatrix == null ? 0 : dotMatrix.length));
        }

        this.dotMatrix = new byte[8];
        for (int i = 0; i < 8; i++) {
            // 固件侧按 5bit 点阵使用，保留低 5 位
            this.dotMatrix[i] = (byte) (dotMatrix[i] & 0x1F);
        }
    }

    @Override
    public byte[] toBytes() {
        byte[] result = new byte[9];
        result[0] = 0x01;
        System.arraycopy(dotMatrix, 0, result, 1, 8);
        return result;
    }

    public static CustomChar[] fromByteArray(byte[] raw) {
        if (raw == null || raw.length % 8 != 0) {
            throw new IllegalArgumentException("输入字节数必须是 8 的倍数，当前为 " + (raw == null ? 0 : raw.length));
        }

        int count = raw.length / 8;
        CustomChar[] chars = new CustomChar[count];

        for (int i = 0; i < count; i++) {
            byte[] slice = new byte[8];
            System.arraycopy(raw, i * 8, slice, 0, 8);
            chars[i] = new CustomChar(slice);
        }

        return chars;
    }
}
