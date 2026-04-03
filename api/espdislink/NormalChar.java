package espdislink;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

/**
 * 普通字符/文本。
 * payload 格式：0x00 + 字节序列。
 */
public class NormalChar implements CharPacket {
    private final byte[] utf8Bytes;

    private NormalChar(byte[] utf8Bytes) {
        this.utf8Bytes = utf8Bytes;
    }

    @Override
    public byte[] toBytes() {
        byte[] result = new byte[1 + utf8Bytes.length];
        result[0] = 0x00;
        System.arraycopy(utf8Bytes, 0, result, 1, utf8Bytes.length);
        return result;
    }

    /**
     * 将字符串拆分为若干 NormalChar op。
     *
     * 规则（与当前固件解析一致）：
     * - ASCII（<0x80）：逐字节输出
     * - 以 0xE3 开头的 3 字节 UTF-8 序列：作为一个单元输出（固件侧用于假名映射）
     * - 其他多字节 UTF-8：固件未定义，统一降级为空格
     */
    public static NormalChar[] fromString(String str) {
        List<NormalChar> result = new ArrayList<>();
        try {
            if (str == null) {
                str = "";
            }

            byte[] utf8 = str.getBytes("UTF-8");
            for (int i = 0; i < utf8.length; i++) {
                int b0 = utf8[i] & 0xFF;

                // 固件侧目前仅对以 0xE3 开头的 3 字节 UTF-8 序列做“假名映射”处理
                if (b0 == 0xE3) {
                    if (i + 2 >= utf8.length) {
                        // UTF-8 不完整，按空格处理
                        result.add(new NormalChar(new byte[]{0x20}));
                        break;
                    }
                    byte b1 = utf8[i + 1];
                    byte b2 = utf8[i + 2];
                    result.add(new NormalChar(new byte[]{(byte) 0xE3, b1, b2}));
                    i += 2;
                    continue;
                }

                // ASCII 直接透传
                if (b0 < 0x80) {
                    result.add(new NormalChar(new byte[]{(byte) b0}));
                    continue;
                }

                // 其他 UTF-8 多字节字符固件未定义：按“字符”统一降级为空格
                // 同时跳过对应的后续字节，避免一个字符变成多个空格
                int skip;
                if ((b0 & 0xE0) == 0xC0) {
                    skip = 1; // 2 字节序列
                } else if ((b0 & 0xF0) == 0xE0) {
                    skip = 2; // 3 字节序列
                } else if ((b0 & 0xF8) == 0xF0) {
                    skip = 3; // 4 字节序列
                } else {
                    skip = 0; // 非法/续字节等情况
                }

                result.add(new NormalChar(new byte[]{0x20}));
                i = Math.min(i + skip, utf8.length - 1);
            }
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

        return result.toArray(new NormalChar[0]);
    }
}
