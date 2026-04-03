package espdislink;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * 对外主入口：ESP32-1602 上位机连接与发送。
 *
 * 分层：
 * - 传输层：{@link TcpTransport}
 * - 协议层：{@link FrameEncoder}
 * - 渲染层：{@link LcdRenderer}
 */
public final class EspDisLink implements AutoCloseable {
    private final String host;
    private final int port;
    private final TcpTransport transport;
    private final AtomicInteger toneSeq = new AtomicInteger(0);
    private final Object sendLock = new Object();
    private byte[] lastSentPacket;

    private final AtomicBoolean keyListenerRunning = new AtomicBoolean(false);
    private Thread keyListenerThread;

    /** 按钮事件回调（设备侧发送 "KEY:<name>"） */
    public interface ButtonListener {
        void onButtonPressed(String buttonName);
    }

    private volatile ButtonListener buttonListener;

    public EspDisLink(String host, int port) {
        this.host = Objects.requireNonNull(host, "host");
        this.port = port;
        this.transport = new TcpTransport(host, port);
    }

    /**
     * 建立连接。
     * @param soTimeoutMs Socket 读超时；0 表示不超时（阻塞等待）
     */
    public void connect(int soTimeoutMs) throws IOException {
        transport.connect(soTimeoutMs);
    }

    /** 建立连接（不超时，阻塞等待）。 */
    public void connect() throws IOException {
        connect(0);
    }

    public boolean isConnected() {
        return transport.isConnected();
    }

    /** 启动心跳（AA 55 04 02），建议 1000~3000ms。 */
    public void startHeartbeat(long intervalMs) {
        transport.startHeartbeat(intervalMs);
    }

    public void stopHeartbeat() {
        transport.stopHeartbeat();
    }

    /** 设置按键监听回调（不会自动启动线程）。 */
    public void setButtonListener(ButtonListener listener) {
        this.buttonListener = listener;
    }

    /**
     * 启动按键监听线程（需要先 connect）。
     * 设备侧会发送形如 "KEY:UP" 的消息。
     */
    public void startButtonListener() throws IOException {
        if (keyListenerRunning.getAndSet(true)) {
            return;
        }

        final InputStream in = transport.getInputStream();

        keyListenerThread = new Thread(() -> {
            byte[] buffer = new byte[64];
            try {
                System.out.println("[ESP32-1602] 正在监听按键...");
                while (keyListenerRunning.get()) {
                    int len = in.read(buffer);
                    if (len <= 0) {
                        break;
                    }

                    String msg = new String(buffer, 0, len).trim();
                    if (msg.startsWith("KEY:")) {
                        String key = msg.substring(4);
                        ButtonListener listener = buttonListener;
                        if (listener != null) {
                            listener.onButtonPressed(key);
                        }
                    }
                }
            } catch (IOException ignored) {
                // socket 关闭时会触发 IOException，用于退出线程
            } finally {
                keyListenerRunning.set(false);
            }
        }, "esp32-1602-key-listener");

        keyListenerThread.setDaemon(true);
        keyListenerThread.start();
    }

    public void stopButtonListener() {
        keyListenerRunning.set(false);
        if (keyListenerThread != null) {
            keyListenerThread.interrupt();
        }
    }

    private void sendIfChanged(byte[] data) throws IOException {
        Objects.requireNonNull(data, "data");
        synchronized (sendLock) {
            if (lastSentPacket != null && Arrays.equals(lastSentPacket, data)) {
                return;
            }
            transport.send(data);
            lastSentPacket = Arrays.copyOf(data, data.length);
        }
    }

    /** 发送原始字节（高级用法）。 */
    public void sendRaw(byte[] data) throws IOException {
        sendIfChanged(data);
    }

    /**
     * 发送一帧。
     * @param frameIntervalMs 帧间隔（ms），0 表示立即显示；非 0 时设备端会缓存并按间隔播放
     */
    public void sendFrame(int frameIntervalMs, CharPacket... ops) throws IOException {
        byte[] frame = FrameEncoder.encodeFrame(frameIntervalMs, ops);
        sendIfChanged(frame);
    }

    /** 立即显示（等价于 frameIntervalMs=0）。 */
    public void sendNow(CharPacket... ops) throws IOException {
        sendFrame(0, ops);
    }

    /** 发送两行 16 字符文本（自动补空格/截断）。 */
    public void sendText2x16Now(String line1, String line2) throws IOException {
        sendNow(LcdRenderer.text2x16(line1, line2));
    }

    /**
     * 发送蜂鸣器 TONE 命令（默认音量 50）。
     */
    public void sendTone(int frequencyHz, int durationMs) throws IOException {
        sendTone(frequencyHz, durationMs, 50);
    }

    /**
     * 发送蜂鸣器 TONE 命令。
     * 协议：AA 55 LEN 20 02 seq freqH freqL durH durL volume
     */
    public void sendTone(int frequencyHz, int durationMs, int volume) throws IOException {
        if (frequencyHz <= 0 || frequencyHz > 0xFFFF) {
            throw new IllegalArgumentException("frequencyHz 超出范围: " + frequencyHz);
        }
        if (durationMs <= 0 || durationMs > 0xFFFF) {
            throw new IllegalArgumentException("durationMs 超出范围: " + durationMs);
        }
        if (volume < 0 || volume > 100) {
            throw new IllegalArgumentException("volume 超出范围(0-100): " + volume);
        }

        byte[] packet = FrameEncoder.encodeToneCommand(
                toneSeq.getAndIncrement() & 0xFF,
                frequencyHz,
                durationMs,
                volume
        );
        sendIfChanged(packet);
    }

    @Override
    public void close() {
        stopButtonListener();
        stopHeartbeat();
        transport.close();

        if (keyListenerThread != null) {
            try {
                keyListenerThread.join(300);
            } catch (InterruptedException ignored) {
            }
        }

        System.out.println("[ESP32-1602] 已断开连接");
    }
}
