<template>
    <BasePage :title="$t('vedirectinfo.VedirectInformation')" :isLoading="dataLoading">
        <CardElement :text="$t('vedirectinfo.ConfigurationSummary')" textVariant="text-bg-primary">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('vedirectinfo.Status') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !vedirectDataList.vedirect_enabled,
                                'text-bg-success': vedirectDataList.vedirect_enabled,
                            }">
                                <span v-if="vedirectDataList.vedirect_enabled">{{ $t('vedirectinfo.Enabled') }}</span>
                                <span v-else>{{ $t('vedirectinfo.Disabled') }}</span>
                            </td>
                        </tr>
                        <tr v-show="vedirectDataList.vedirect_enabled">
                                <th>{{ $t('vedirectinfo.UpdatesOnly') }}</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !vedirectDataList.vedirect_updatesonly,
                                    'text-bg-success': vedirectDataList.vedirect_updatesonly,
                                }">
                                    <span v-if="vedirectDataList.vedirect_updatesonly">{{ $t('vedirectinfo.UpdatesEnabled') }}</span>
                                    <span v-else>{{ $t('vedirectinfo.UpdatesDisabled') }}</span>
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
import type { VedirectStatus } from "@/types/VedirectStatus";
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        CardElement,
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
            fetch("/api/vedirect/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.vedirectDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>
