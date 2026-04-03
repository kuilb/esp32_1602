package espdislink;

/**
 * 渲染层：将高层内容（两行文本、自定义字符等）转换为协议操作（ops）。
 */
public final class LcdRenderer {
    private LcdRenderer() {
    }

    public static CharPacket[] text(String text) {
        return NormalChar.fromString(text);
    }

    public static CharPacket glyph(byte[] dotMatrix8Rows) {
        return new CustomChar(dotMatrix8Rows);
    }

    public static CharPacket[] text2x16(String line1, String line2) {
        String l1 = padOrTrim(line1, 16);
        String l2 = padOrTrim(line2, 16);
        return text(l1 + l2);
    }

    private static String padOrTrim(String s, int len) {
        if (s == null) {
            s = "";
        }
        if (s.length() > len) {
            return s.substring(0, len);
        }
        if (s.length() == len) {
            return s;
        }
        StringBuilder sb = new StringBuilder(len);
        sb.append(s);
        while (sb.length() < len) {
            sb.append(' ');
        }
        return sb.toString();
    }
}
