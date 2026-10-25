<template>
    <CardElement :text="$t('networkstatus.Title')" textVariant="text-bg-primary">
        <div role="status" aria-live="polite">
            <p v-if="requestFailed" class="alert alert-warning mb-0">{{ $t('networkstatus.Unavailable') }}</p>
            <p v-else-if="!networkStatus" class="mb-0">{{ $t('base.Loading') }}</p>
            <template v-else>
                <p>
                    <span
                        :class="[
                            'badge text-wrap text-start lh-base',
                            networkStatus.network_connected ? 'text-bg-success' : 'text-bg-warning',
                        ]"
                    >
                        {{ connectionText }}
                    </span>
                </p>
                <dl class="row mb-0">
                    <template v-if="networkStatus.sta_connection_state === 'connected'">
                        <dt class="col-sm-4">{{ $t('networkstatus.ConnectedSsid') }}</dt>
                        <dd class="col-sm-8 text-break">{{ networkStatus.sta_ssid }}</dd>
                    </template>
                    <template v-if="networkStatus.sta_target_ssid">
                        <dt class="col-sm-4">{{ $t('networkstatus.TargetSsid') }}</dt>
                        <dd class="col-sm-8 text-break">{{ networkStatus.sta_target_ssid }}</dd>
                    </template>
                    <template
                        v-if="networkStatus.sta_connection_state === 'paused' && networkStatus.sta_retry_in !== null"
                    >
                        <dt class="col-sm-4">{{ $t('networkstatus.NextAttempt') }}</dt>
                        <dd class="col-sm-8">
                            {{ $t('networkstatus.RetryIn', { seconds: networkStatus.sta_retry_in }) }}
                        </dd>
                    </template>
                    <template v-if="networkStatus.network_connected">
                        <dt class="col-sm-4">{{ $t('interfacenetworkinfo.IpAddress') }}</dt>
                        <dd class="col-sm-8">{{ networkStatus.network_ip }}</dd>
                    </template>
                    <template v-if="networkStatus.sta_disconnect_reason !== null">
                        <dt class="col-sm-4">{{ $t('networkstatus.DisconnectReason') }}</dt>
                        <dd class="col-sm-8">{{ disconnectText }} ({{ networkStatus.sta_disconnect_reason }})</dd>
                    </template>
                    <dt class="col-sm-4">{{ $t('networkstatus.AccessPoint') }}</dt>
                    <dd class="col-sm-8 text-break">
                        <template v-if="networkStatus.ap_status">
                            {{ networkStatus.ap_ssid }} · {{ networkStatus.ap_ip }}
                        </template>
                        <template v-else>{{ $t('wifiapinfo.Disabled') }}</template>
                    </dd>
                </dl>
            </template>
        </div>
    </CardElement>
</template>

<script lang="ts">
import CardElement from '@/components/CardElement.vue';
import type { NetworkStatus } from '@/types/NetworkStatus';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: { CardElement },
    props: { refreshKey: { type: Number, default: 0 } },
    emits: ['status'],
    data() {
        return {
            networkStatus: null as NetworkStatus | null,
            requestFailed: false,
            active: false,
            refreshTimer: undefined as number | undefined,
            requestController: null as AbortController | null,
        };
    },
    computed: {
        connectionText(): string {
            if (!this.networkStatus) return '';
            if (this.networkStatus.network_mode === 'Ethernet') {
                return this.$t(
                    this.networkStatus.network_connected
                        ? 'networkstatus.EthernetConnected'
                        : 'networkstatus.EthernetDisconnected'
                );
            }
            return this.$t('networkstatus.States.' + this.networkStatus.sta_connection_state);
        },
        disconnectText(): string {
            const reasons: Record<number, string> = {
                8: 'StationLeft',
                15: 'HandshakeTimeout',
                200: 'SignalLost',
                201: 'NetworkNotFound',
                202: 'AuthenticationFailed',
                203: 'AssociationFailed',
                204: 'HandshakeTimeout',
            };
            const reason = this.networkStatus?.sta_disconnect_reason;
            return this.$t('networkstatus.Reasons.' + (reason == null ? 'Other' : (reasons[reason] ?? 'Other')));
        },
    },
    watch: {
        refreshKey() {
            this.refresh();
        },
    },
    mounted() {
        this.active = true;
        this.refresh();
    },
    beforeUnmount() {
        this.active = false;
        window.clearTimeout(this.refreshTimer);
        this.requestController?.abort();
    },
    methods: {
        async refresh() {
            if (!this.active || this.requestController) return;
            window.clearTimeout(this.refreshTimer);
            const controller = new AbortController();
            this.requestController = controller;
            const timeout = window.setTimeout(() => controller.abort(), 5000);
            try {
                const response = await fetch('/api/network/status', {
                    headers: authHeader(),
                    cache: 'no-store',
                    signal: controller.signal,
                });
                const status: NetworkStatus = await handleResponse(response, this.$emitter, this.$router, true);
                if (!this.active) return;
                this.networkStatus = status;
                this.requestFailed = false;
                this.$emit('status', status);
            } catch {
                if (!this.active) return;
                // A lost browser connection must not leave the last successful state looking current.
                this.networkStatus = null;
                this.requestFailed = true;
                this.$emit('status', null);
            } finally {
                window.clearTimeout(timeout);
                this.requestController = null;
                if (this.active) this.refreshTimer = window.setTimeout(this.refresh, 2000);
            }
        },
    },
});
</script>
