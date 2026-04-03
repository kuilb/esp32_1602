package espdislink;

/**
 * 最小示例：验证 API 是否可用。
 *
 * 用法：传入 <host> <port>
 */
public final class ExampleMain {
    public static void main(String[] args) throws Exception {
        if (args.length < 2) {
            System.out.println("Usage: ExampleMain <host> <port>");
            return;
        }

        String host = args[0];
        int port = Integer.parseInt(args[1]);

        try (EspDisLink link = new EspDisLink(host, port)) {
            link.connect();
            link.startHeartbeat(2000);

            link.sendText2x16Now("Hello", "ESP32-1602");

            byte[] glyph = new byte[]{
                    0b00000,
                    0b01010,
                    0b01010,
                    0b00000,
                    0b10001,
                    0b01110,
                    0b00000,
                    0b00000
            };
            link.sendNow(LcdRenderer.glyph(glyph));
            link.sendTone(880, 120, 60);

            Thread.sleep(1000);
        }
    }
}
