<template>
    <div class="row mb-3">
        <label for="nearbyWifiNetwork" class="col-sm-2 col-form-label">{{ $t('networkscan.NearbyNetworks') }}</label>
        <div class="col-sm-10">
            <div class="d-flex flex-column flex-sm-row gap-2">
                <select
                    id="nearbyWifiNetwork"
                    class="form-select"
                    style="min-width: 0"
                    v-model="selectedNetwork"
                    :disabled="scanning || networks.length === 0"
                    aria-describedby="nearbyWifiHelp"
                >
                    <option value="" disabled>{{ $t('networkscan.SelectNetwork') }}</option>
                    <option v-for="network in networks" :key="network.bssid" :value="network.bssid">
                        {{ network.ssid }} · {{ network.rssi }} dBm ·
                        {{ $t(network.secure ? 'networkscan.Secured' : 'networkscan.Open') }} · {{ network.bssid }}
                    </option>
                </select>
                <button
                    type="button"
                    class="btn btn-outline-primary flex-shrink-0"
                    :disabled="scanning"
                    @click="scan()"
                >
                    {{ $t(scanning ? 'networkscan.Scanning' : requested ? 'networkscan.Refresh' : 'networkscan.Scan') }}
                </button>
            </div>
            <p id="nearbyWifiHelp" class="form-text mb-0">{{ $t('networkscan.ManualHint') }}</p>
            <div role="status" aria-live="polite">
                <p v-if="error" class="alert alert-warning mt-2 mb-0">{{ $t('networkscan.Errors.' + error) }}</p>
                <p v-else-if="scanning" class="form-text mb-0">{{ $t('networkscan.ScanningHint') }}</p>
                <p v-else-if="complete" class="form-text mb-0">
                    {{
                        networks.length
                            ? $t('networkscan.Found', { count: networks.length })
                            : $t('networkscan.NoNetworks')
                    }}
                </p>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

interface WifiNetwork {
    ssid: string;
    bssid: string;
    rssi: number;
    secure: boolean;
}

interface WifiScanResponse {
    state: 'idle' | 'running' | 'complete' | 'failed';
    networks: WifiNetwork[];
    error: 'busy' | 'wifi_disabled' | 'scan_failed' | 'timeout' | null;
}

export default defineComponent({
    props: {
        modelValue: { type: String, required: true },
    },
    emits: ['update:modelValue'],
    data() {
        return {
            networks: [] as WifiNetwork[],
            selectedBssid: '',
            requested: false,
            scanning: false,
            complete: false,
            error: '' as Exclude<WifiScanResponse['error'], null> | 'unavailable' | '',
            active: true,
            controller: null as AbortController | null,
        };
    },
    computed: {
        selectedNetwork: {
            get(): string {
                const selected = this.networks.find(
                    (network) => network.bssid === this.selectedBssid && network.ssid === this.modelValue
                );
                return (
                    selected?.bssid ?? this.networks.find((network) => network.ssid === this.modelValue)?.bssid ?? ''
                );
            },
            set(bssid: string) {
                const network = this.networks.find((network) => network.bssid === bssid);
                if (network) {
                    this.selectedBssid = bssid;
                    this.$emit('update:modelValue', network.ssid);
                }
            },
        },
    },
    mounted() {
        void this.scan('GET');
    },
    beforeUnmount() {
        this.active = false;
        this.controller?.abort();
    },
    methods: {
        async scan(initialMethod: 'GET' | 'POST' = 'POST') {
            if (this.scanning) return;
            this.requested = true;
            this.scanning = true;
            this.complete = false;
            this.error = '';
            this.networks = [];
            const controller = new AbortController();
            this.controller = controller;
            // Bound the whole scan, including polling delays and a request that never returns.
            const timeout = window.setTimeout(() => controller.abort(), 25000);
            try {
                let method = initialMethod;
                while (!controller.signal.aborted) {
                    const response = await fetch('/api/network/scan', {
                        method,
                        headers: authHeader(),
                        cache: 'no-store',
                        signal: controller.signal,
                    });
                    if (!this.active || controller.signal.aborted) return;
                    if (response.status === 202) {
                        await this.waitForPoll(controller.signal);
                        method = 'GET';
                        continue;
                    }
                    const result: WifiScanResponse = await handleResponse(response, this.$emitter, this.$router, true);
                    if (!this.active || controller.signal.aborted) return;
                    if (
                        response.status !== 200 ||
                        !result ||
                        !Array.isArray(result.networks) ||
                        (result.state !== 'complete' && result.state !== 'failed')
                    ) {
                        this.error = 'unavailable';
                        return;
                    }
                    if (result.state === 'complete') {
                        this.networks = result.networks;
                        this.complete = true;
                        return;
                    }
                    this.error = result.error ?? 'scan_failed';
                    return;
                }
            } catch {
                if (this.active) this.error = controller.signal.aborted ? 'timeout' : 'unavailable';
            } finally {
                window.clearTimeout(timeout);
                if (this.active && controller.signal.aborted) this.error = 'timeout';
                this.controller = null;
                this.scanning = false;
            }
        },
        waitForPoll(signal: AbortSignal) {
            return new Promise<void>((resolve, reject) => {
                const timer = window.setTimeout(() => {
                    signal.removeEventListener('abort', abort);
                    resolve();
                }, 1000);
                const abort = () => {
                    window.clearTimeout(timer);
                    reject();
                };
                signal.addEventListener('abort', abort, { once: true });
                if (signal.aborted) abort();
            });
        },
    },
});
</script>
