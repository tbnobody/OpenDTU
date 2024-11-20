<template>
    <BasePage :title="$t('vedirectinfo.VedirectInformation')" :isLoading="dataLoading">
        <CardElement :text="$t('vedirectinfo.ConfigurationSummary')" textVariant="text-bg-primary" table>
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('vedirectinfo.Status') }}</th>
                            <td>
                                <StatusBadge
                                    :status="vedirectDataList.vedirect_enabled"
                                    true_text="vedirectinfo.Enabled"
                                    false_text="vedirectinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('vedirectinfo.VerboseLogging') }}</th>
                            <td>
                                <StatusBadge
                                    :status="vedirectDataList.verbose_logging"
                                    true_text="vedirectinfo.Enabled"
                                    false_text="vedirectinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('vedirectinfo.UpdatesOnly') }}</th>
                            <td>
                                <StatusBadge
                                    :status="vedirectDataList.vedirect_updatesonly"
                                    true_text="vedirectinfo.UpdatesEnabled"
                                    false_text="vedirectinfo.UpdatesDisabled"
                                />
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </CardElement>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import CardElement from '@/components/CardElement.vue';
import StatusBadge from '@/components/StatusBadge.vue';
import type { VedirectStatus } from '@/types/VedirectStatus';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        CardElement,
        StatusBadge,
    },
    data() {
        return {
            dataLoading: true,
            vedirectDataList: {} as VedirectStatus,
        };
    },
    created() {
        this.getVedirectInfo();
    },
    methods: {
        getVedirectInfo() {
            this.dataLoading = true;
            fetch('/api/vedirect/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.vedirectDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>
