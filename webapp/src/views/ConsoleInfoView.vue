<template>
    <BasePage :title="$t('console.Console')" :isLoading="dataLoading">
        <CardElement :text="$t('console.VirtualDebugConsole')" textVariant="text-bg-primary">
            <div class="btn-toolbar mb-3" role="toolbar">
                <div class="input-group me-2">
                    <div class="input-group-text" id="btnSearch">
                        <BIconSearch />
                    </div>
                    <input
                        v-model="searchText"
                        type="text"
                        class="form-control"
                        :placeholder="$t('console.RegularExpression')"
                        aria-label="Search"
                        aria-describedby="btnSearch"
                    />
                </div>

                <div class="btn-group me-2" role="group" aria-label="Filter by level">
                    <div class="dropdown">
                        <button
                            class="btn btn-outline-secondary dropdown-toggle"
                            type="button"
                            id="logLevelDropdown"
                            data-bs-toggle="dropdown"
                            aria-expanded="false"
                        >
                            <BIconFilter /> {{ $t('console.LogLevel', { cnt: activeLevels.length }) }}
                        </button>
                        <ul
                            class="dropdown-menu dropdown-menu-dark p-2"
                            aria-labelledby="logLevelDropdown"
                            style="min-width: 200px"
                        >
                            <li v-for="(label, tag) in levelMap" :key="tag">
                                <div class="form-check">
                                    <input
                                        class="form-check-input"
                                        type="checkbox"
                                        :id="`dropdown-filter-${label}`"
                                        v-model="activeLevels"
                                        :value="levelMap[tag]"
                                    />
                                    <label class="form-check-label" :for="`dropdown-filter-${label}`">
                                        {{ label.toUpperCase() }}
                                    </label>
                                </div>
                            </li>
                        </ul>
                    </div>
                </div>

                <div class="btn-group me-2" role="group">
                    <button
                        type="button"
                        class="btn btn-primary"
                        :onClick="clearConsole"
                        :title="$t('console.ClearConsole')"
                    >
                        <BIconTrash />
                    </button>
                </div>
                <div class="btn-group me-2" role="group">
                    <button
                        type="button"
                        class="btn btn-secondary"
                        :onClick="copyConsole"
                        :title="$t('console.CopyToClipboard')"
                    >
                        <BIconClipboard />
                    </button>
                    <button
                        type="button"
                        class="btn btn-secondary"
                        :onClick="exportConsole"
                        :title="$t('console.Download')"
                    >
                        <BIconDownload />
                    </button>
                </div>

                <div class="col-auto ms-auto mt-2">
                    <div class="form-check form-switch">
                        <input
                            class="form-check-input"
                            type="checkbox"
                            role="switch"
                            id="autoScroll"
                            v-model="autoScroll"
                        />
                        <label class="form-check-label" for="autoScroll">
                            {{ $t('console.EnableAutoScroll') }}
                        </label>
                    </div>
                </div>
            </div>

            <div id="log" ref="logRef" class="log-output" @scroll="handleScroll">
                <div v-for="(line, index) in filteredLines" :key="index" class="log-line" :class="line.level">
                    <span class="timestamp">[{{ formatTimestamp(line.timestamp) }}]</span>
                    {{ line.text }}
                </div>
            </div>
            {{ $t('console.Lines', { lines: $n(filteredLines.length) }) }}
        </CardElement>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import CardElement from '@/components/CardElement.vue';
import { authUrl } from '@/utils/authentication';
import { BIconClipboard, BIconDownload, BIconFilter, BIconSearch, BIconTrash } from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

interface LogLine {
    text: string;
    timestamp: Date;
    level: string;
}

export default defineComponent({
    components: {
        BasePage,
        CardElement,
        BIconDownload,
        BIconClipboard,
        BIconTrash,
        BIconFilter,
        BIconSearch,
    },
    data() {
        return {
            socket: {} as WebSocket,
            heartInterval: 0,
            dataLoading: true,
            autoScroll: true,
            activeLevels: ['error', 'warning', 'info', 'debug', 'verbose'],
            searchText: '',
            lines: [] as LogLine[],
            buffer: '',
            levelMap: {
                'E (': 'error',
                'W (': 'warning',
                'I (': 'info',
                'D (': 'debug',
                'V (': 'verbose',
            } as Record<string, string>,
        };
    },
    created() {
        this.initSocket();
        this.dataLoading = false;
    },
    unmounted() {
        this.closeSocket();
    },
    computed: {
        filteredLines(): LogLine[] {
            return this.lines.filter((line) => {
                const regexp = new RegExp(this.searchText, 'i');
                const found = regexp.test(line.text);
                return this.activeLevels.includes(line.level) && found;
            });
        },
    },
    methods: {
        initSocket() {
            console.log('Starting connection to WebSocket Server');

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === 'https:' ? 'wss' : 'ws'}://${authString}${host}/console`;

            this.closeSocket();
            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                console.log(event);

                this.buffer += event.data;
                const splitLines = this.buffer.split('\n');

                this.buffer = splitLines.pop() || ''; // Save the incomplete line

                splitLines.forEach((line) => {
                    this.lines.push({
                        text: line,
                        timestamp: new Date(), // assign time of message arrival
                        level: this.getLineClass(line),
                    });
                    this.$nextTick(() => {
                        if (this.autoScroll) {
                            const el = this.$refs.logRef as HTMLDivElement;
                            if (el) {
                                el.scrollTop = el.scrollHeight;
                            }
                        }
                    });
                });
                this.heartCheck(); // Reset heartbeat detection
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log('Successfully connected to the echo websocket server...');
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        getLineClass(line: string): string {
            for (const tag in this.levelMap) {
                if (line.includes(tag)) {
                    return this.levelMap[tag];
                }
            }
            return 'default';
        },
        handleScroll() {
            const el = this.$refs.logRef as HTMLDivElement;
            if (!el) {
                return;
            }

            const threshold = 20; // px from bottom to consider "at bottom"
            const atBottom = el.scrollTop + el.clientHeight >= el.scrollHeight - threshold;

            this.autoScroll = atBottom;
        },
        // Send heartbeat packets regularly * 5s Send a heartbeat
        heartCheck() {
            if (this.heartInterval) {
                clearInterval(this.heartInterval);
            }
            this.heartInterval = setInterval(() => {
                if (this.socket.readyState === WebSocket.OPEN) {
                    // Connection status
                    this.socket.send('ping');
                } else {
                    clearInterval(this.heartInterval);
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

            if (this.heartInterval) {
                clearInterval(this.heartInterval);
            }
        },
        formatTimestamp(date: Date): string {
            return (
                ('0' + date.getHours()).slice(-2) +
                ':' +
                ('0' + date.getMinutes()).slice(-2) +
                ':' +
                ('0' + date.getSeconds()).slice(-2) +
                '.' +
                (date.getMilliseconds() / 1000).toFixed(3).slice(2, 5)
            );
        },
        clearConsole() {
            this.lines = [];
            this.buffer = '';
        },
        copyConsole() {
            const content = this.filteredLines
                .map((line) => `[${this.formatTimestamp(line.timestamp)}] ${line.text}`)
                .join('\n');
            navigator.clipboard
                .writeText(content)
                .then(() => {
                    console.log('Copied to clipboard!');
                })
                .catch((err) => {
                    console.error('Failed to copy:', err);
                });
        },
        exportConsole() {
            const content = this.filteredLines
                .map((line) => `[${this.formatTimestamp(line.timestamp)}] ${line.text}`)
                .join('\n');
            const timestamp = this.getFileTimestamp();
            const filename = `opendtu-log-${timestamp}.txt`;
            const blob = new Blob([content], { type: 'text/plain' });
            const url = URL.createObjectURL(blob);

            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            a.click();

            URL.revokeObjectURL(url);
        },
        getFileTimestamp(): string {
            const now = new Date();
            const pad = (n: number) => n.toString().padStart(2, '0');

            const year = now.getFullYear();
            const month = pad(now.getMonth() + 1);
            const day = pad(now.getDate());
            const hour = pad(now.getHours());
            const min = pad(now.getMinutes());
            const sec = pad(now.getSeconds());

            return `${year}-${month}-${day}_${hour}-${min}-${sec}`;
        },
    },
});
</script>

<style scoped>
.log-output {
    height: 500px;
    max-height: 500px;
    overflow-y: auto;
    overflow-x: auto;
    padding: 8px;
    background: #111;
    border: 1px solid #333;
    border-radius: 6px;
    font-family: courier new;
    font-size: 0.875em;
}

.log-line {
    margin: 3px 0;
    white-space: pre;
}

.timestamp {
    color: #888;
    margin-right: 8px;
}

.error {
    color: #ff5555;
}

.warning {
    color: #ffcc00;
}

.info {
    color: #a8ff60;
}

.debug {
    color: #57c7ff;
}

.verbose {
    color: #bbbbbb;
}

.default {
    color: #ddd;
}
</style>
