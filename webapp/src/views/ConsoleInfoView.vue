<template>
    <BasePage :title="$t('console.Console')" :isLoading="dataLoading">
        <CardElement :text="$t('console.VirtualDebugConsole')" textVariant="text-bg-primary">
            <div class="row g-3 align-items-center">
                <div class="col">
                    <div class="form-check form-switch">
                        <input class="form-check-input" type="checkbox" role="switch" id="autoScroll"
                            v-model="isAutoScroll">
                        <label class="form-check-label" for="autoScroll">
                            {{ $t('console.EnableAutoScroll') }}
                        </label>
                    </div>
                </div>
                <div class="col text-end">
                    <div class="btn-group" role="group">
                        <button type="button" class="btn btn-primary" :onClick="clearConsole">
                            {{ $t('console.ClearConsole') }}</button>
                        <button type="button" class="btn btn-secondary" :onClick="copyConsole">
                            {{ $t('console.CopyToClipboard') }}</button>
                    </div>
                </div>
            </div>
            <textarea id="console" class="form-control" rows="24" v-model="consoleBuffer" readonly></textarea>
        </CardElement>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import CardElement from '@/components/CardElement.vue';
import { authUrl } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        CardElement,
    },
    data() {
        return {
            socket: {} as WebSocket,
            heartInterval: 0,
            dataLoading: true,
            consoleBuffer: "",
            isAutoScroll: true,
        };
    },
    created() {
        this.initSocket();
        this.dataLoading = false;
    },
    unmounted() {
        this.closeSocket();
    },
    watch: {
        consoleBuffer() {
            if (this.isAutoScroll) {
                let textarea = this.$el.querySelector("#console");
                setTimeout(() => {
                    textarea.scrollTop = textarea.scrollHeight;
                }, 0);
            }
        }
    },
    methods: {
        initSocket() {
            console.log("Starting connection to WebSocket Server");

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === "https:" ? "wss" : "ws"
                }://${authString}${host}/console`;

            this.closeSocket();
            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                console.log(event);
                this.consoleBuffer += event.data;
                this.heartCheck(); // Reset heartbeat detection
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log("Successfully connected to the echo websocket server...");
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        // Send heartbeat packets regularly * 59s Send a heartbeat
        heartCheck() {
            this.heartInterval && clearTimeout(this.heartInterval);
            this.heartInterval = setInterval(() => {
                if (this.socket.readyState === 1) {
                    // Connection status
                    this.socket.send("ping");
                } else {
                    this.initSocket(); // Breakpoint reconnection 5 Time
                }
            }, 5 * 1000);
        },
        /** To break off websocket Connect */
        closeSocket() {
            try {
                this.socket.close();
            } catch {
                // continue regardless of error
            }

            this.heartInterval && clearTimeout(this.heartInterval);
        },
        clearConsole() {
            this.consoleBuffer = "";
        },
        copyConsole() {
            navigator.clipboard.writeText(this.consoleBuffer).then(
                () => {
                    console.log('clipboard successfully set');
                },
                () => {
                    console.error('clipboard write failed');
                }
            );
        }
    }
});
</script>

<style>
textarea:focus.form-control,
textarea.form-control {
    background-color: #0C0C0C;
    color: #CCCCCC;
    padding: 8px;
    font-family: courier new;
}
</style>