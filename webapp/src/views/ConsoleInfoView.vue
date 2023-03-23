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

                let outstr = new String(event.data);
                if (outstr.endsWith('\n')) {
                    outstr = outstr.substring(0, outstr.length - 1);
                }
                this.consoleBuffer += this.getOutDate() + outstr.replaceAll("\n", "\n" + this.getOutDate());
                this.heartCheck(); // Reset heartbeat detection
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log("Successfuly connected to the echo websocket server...");
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
        getOutDate(): String {
            const u = new Date();
            return ('0' + u.getHours()).slice(-2) + ':' +
                ('0' + u.getMinutes()).slice(-2) + ':' +
                ('0' + u.getSeconds()).slice(-2) + '.' +
                (u.getMilliseconds() / 1000).toFixed(3).slice(2, 5) + ' > ';
        },
        clearConsole() {
            this.consoleBuffer = "";
        },
        copyConsole() {
            var input = document.createElement('textarea');
            input.innerHTML = this.consoleBuffer;
            document.body.appendChild(input);
            input.select();
            document.execCommand('copy');
            document.body.removeChild(input);
        }
    }
});
</script>

<style>
#console {
    background-color: #0C0C0C;
    color: #CCCCCC;
    padding: 8px;
    font-family: courier new;
    font-size: .875em;
}
</style>