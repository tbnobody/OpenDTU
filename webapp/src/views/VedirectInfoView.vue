<template>
    <div class="container-xxl" role="main">
        <div class="page-header">
            <h1>Ve.direct Info</h1>
        </div>

        <div class="text-center" v-if="dataLoading">
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </div>

        <template v-if="!dataLoading">
            <div class="card">
                <div class="card-header bg-primary">Configuration Summary</div>
                <div class="card-body">
                    <div class="table-responsive">
                        <table class="table table-hover table-condensed">
                            <tbody>
                                <tr>
                                    <th>Status</th>
                                    <td class="badge" :class="{
                                        'bg-danger': !vedirectDataList.vedirect_enabled,
                                        'bg-success': vedirectDataList.vedirect_enabled,
                                    }">
                                        <span v-if="vedirectDataList.vedirect_enabled">enabled</span>
                                        <span v-else>disabled</span>
                                    </td>
                                </tr>
                                <tr v-show="vedirectDataList.vedirect_enabled">
                                    <th>Updates Only</th>
                                    <td class="badge" :class="{
                                        'bg-danger': !vedirectDataList.vedirect_updatesonly,
                                        'bg-success': vedirectDataList.vedirect_updatesonly,
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
        </template>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    data() {
        return {
            dataLoading: true,
            vedirectDataList: {
                vedirect_enabled: false,
                vedirect_updatesonly: true,
            },
        };
    },
    created() {
        this.getVedirectInfo();
    },
    methods: {
        getVedirectInfo() {
            this.dataLoading = true;
            fetch("/api/vedirect/status")
                .then((response) => response.json())
                .then((data) => {
                    this.vedirectDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>
