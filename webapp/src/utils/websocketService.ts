// WebSocketService.ts

export interface WebSocketCallbacks {
    onMessage?: (event: MessageEvent) => void;
    onOpen?: (event: Event) => void;
    onClose?: (event: CloseEvent) => void;
    onError?: (event: Event) => void;
}

export default class WebSocketService {
    private url: string;
    private socket: WebSocket | null = null;

    private reconnectDelay = 200;
    private maxDelay = 3000;

    private heartbeatInterval: number | null = null;
    private heartbeatTimeout = 25000; // 25s ping interval

    private onMessage: (event: MessageEvent) => void;
    private onOpen: (event: Event) => void;
    private onClose: (event: CloseEvent) => void;
    private onError: (event: Event) => void;

    constructor(url: string, callbacks: WebSocketCallbacks = {}) {
        this.url = url;
        this.onMessage = callbacks.onMessage ?? (() => {});
        this.onOpen = callbacks.onOpen ?? (() => {});
        this.onClose = callbacks.onClose ?? (() => {});
        this.onError = callbacks.onError ?? (() => {});
    }

    /** Public connect entry */
    public connect(): void {
        const isiOS = /iPad|iPhone|iPod/.test(navigator.userAgent);

        if (isiOS) {
            // Fix for WebKit timing bug on all iOS browsers (Safari/Chrome/Vivaldi/etc.)
            setTimeout(() => this.openSocket(), 50);
        } else {
            this.openSocket();
        }
    }

    /** Internal open function */
    private openSocket(): void {
        try {
            this.socket = new WebSocket(this.url);

            this.socket.onopen = (event: Event) => {
                this.reconnectDelay = 200; // reset backoff
                this.startHeartbeat();
                this.onOpen(event);
            };

            this.socket.onmessage = (event: MessageEvent) => {
                this.onMessage(event);
            };

            this.socket.onclose = (event: CloseEvent) => {
                this.stopHeartbeat();
                this.onClose(event);
                this.scheduleReconnect();
            };

            this.socket.onerror = (event: Event) => {
                this.onError(event);
                // iOS often doesn't fire real errors, but we close to trigger retry
                this.socket?.close();
            };
        } catch {
            this.scheduleReconnect();
        }
    }

    /** Reconnect with gradual backoff */
    private scheduleReconnect(): void {
        this.stopHeartbeat();

        setTimeout(() => {
            this.reconnectDelay = Math.min(this.reconnectDelay * 1.4, this.maxDelay);
            this.connect();
        }, this.reconnectDelay);
    }

    /** Heartbeat keeps NAT/websocket alive */
    private startHeartbeat(): void {
        this.stopHeartbeat();

        this.heartbeatInterval = window.setInterval(() => {
            if (this.socket?.readyState === WebSocket.OPEN) {
                this.socket.send(JSON.stringify({ type: 'ping' }));
            }
        }, this.heartbeatTimeout);
    }

    private stopHeartbeat(): void {
        if (this.heartbeatInterval !== null) {
            clearInterval(this.heartbeatInterval);
            this.heartbeatInterval = null;
        }
    }

    /** Explicit close function */
    public close(): void {
        this.stopHeartbeat();

        if (this.socket) {
            this.socket.onopen = null;
            this.socket.onmessage = null;
            this.socket.onclose = null;
            this.socket.onerror = null;

            this.socket.close();
            this.socket = null;
        }
    }
}
