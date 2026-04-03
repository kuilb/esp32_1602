package espdislink;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * 传输层：负责 TCP 连接生命周期与原始字节流收发（不包含任何业务/协议逻辑）。
 */
public final class TcpTransport implements AutoCloseable {
    private final String host;
    private final int port;

    private Socket socket;
    private OutputStream out;
    private InputStream in;

    private Thread heartbeatThread;
    private final AtomicBoolean heartbeatRunning = new AtomicBoolean(false);

    public TcpTransport(String host, int port) {
        this.host = Objects.requireNonNull(host, "host");
        this.port = port;
    }

    public synchronized void connect(int soTimeoutMs) throws IOException {
        if (isConnected()) {
            return;
        }

        // 使用 connect timeout，避免设备离线时长时间阻塞。
        Socket s = new Socket();
        if (soTimeoutMs > 0) {
            s.connect(new InetSocketAddress(host, port), soTimeoutMs);
        } else {
            s.connect(new InetSocketAddress(host, port));
        }

        socket = s;
        socket.setSoTimeout(soTimeoutMs);
        socket.setTcpNoDelay(true);
        out = socket.getOutputStream();
        in = socket.getInputStream();
    }

    public void connect() throws IOException {
        connect(0);
    }

    public synchronized boolean isConnected() {
        return socket != null && socket.isConnected() && !socket.isClosed();
    }

    public void send(byte[] data) throws IOException {
        Objects.requireNonNull(data, "data");
        synchronized (this) {
            if (!isConnected()) {
                throw new IOException("未连接");
            }
        }
        synchronized (out) {
            out.write(data);
            out.flush();
        }
    }

    public synchronized InputStream getInputStream() throws IOException {
        if (!isConnected()) {
            throw new IOException("未连接");
        }
        return in;
    }

    /** 启动心跳线程（AA 55 04 02）。 */
    public void startHeartbeat(long intervalMs) {
        if (intervalMs <= 0) {
            throw new IllegalArgumentException("intervalMs 必须 > 0");
        }

        if (heartbeatRunning.getAndSet(true)) {
            return;
        }

        heartbeatThread = new Thread(() -> {
            final byte[] heartbeat = FrameEncoder.encodeHeartbeat();
            while (heartbeatRunning.get()) {
                try {
                    send(heartbeat);
                    Thread.sleep(intervalMs);
                } catch (InterruptedException e) {
                    break;
                } catch (IOException e) {
                    break;
                }
            }
            heartbeatRunning.set(false);
        }, "esp32-1602-heartbeat");

        heartbeatThread.setDaemon(true);
        heartbeatThread.start();
    }

    public void stopHeartbeat() {
        heartbeatRunning.set(false);
        if (heartbeatThread != null) {
            heartbeatThread.interrupt();
        }
    }

    @Override
    public synchronized void close() {
        stopHeartbeat();

        try {
            if (out != null) {
                out.close();
            }
        } catch (IOException ignored) {
        }

        try {
            if (in != null) {
                in.close();
            }
        } catch (IOException ignored) {
        }

        try {
            if (socket != null) {
                socket.close();
            }
        } catch (IOException ignored) {
        }

        out = null;
        in = null;
        socket = null;
    }
}
