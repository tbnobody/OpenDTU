<template>
    <BasePage :title="'Ve.direct Info'" :isLoading="dataLoading">
        <div class="card">
            <div class="card-header text-bg-primary">Configuration Summary</div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>Status</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !vedirectDataList.vedirect_enabled,
                                    'text-bg-success': vedirectDataList.vedirect_enabled,
                                }">
                                    <span v-if="vedirectDataList.vedirect_enabled">enabled</span>
                                    <span v-else>disabled</span>
                                </td>
                            </tr>
                            <tr v-show="vedirectDataList.vedirect_enabled">
                                    <th>Updates Only</th>
                                    <td class="badge" :class="{
                                        'text-bg-danger': !vedirectDataList.vedirect_updatesonly,
                                        'text-bg-success': vedirectDataList.vedirect_updatesonly,
                                    }">
                                        <span v-if="vedirectDataList.vedirect_updatesonly">enabled</span>
                                        <span v-else>disabled</span>
                                    </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { handleResponse, authHeader } from '@/utils/authentication';
import BasePage from '@/components/BasePage.vue';
import type { VedirectStatus } from "@/types/VedirectStatus";

export default defineComponent({
    components: {
        BasePage,
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
