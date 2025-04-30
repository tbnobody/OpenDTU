<template>
    <BasePage :title="$t('loggingadmin.LoggingSettings')" :isLoading="dataLoading">
        <BootstrapAlert
            v-model="alert.show"
            dismissible
            :variant="alert.type"
            :auto-dismiss="alert.type != 'success' ? 0 : 5000"
        >
            {{ alert.message }}
        </BootstrapAlert>
        <form @submit="saveLogConfig">
            <CardElement :text="$t('loggingadmin.LogLevel')" textVariant="text-bg-primary">
                <div class="row mb-3">
                    <label for="inputDefaultLevel" class="col-sm-2 col-form-label">{{
                        $t('loggingadmin.DefaultLevel')
                    }}</label>
                    <div class="col-sm-10">
                        <select class="form-select" id="inputDefaultLevel" v-model="loggingList.loglevel.default">
                            <option
                                v-for="level in logLevelList.filter((property) => property.key >= 0)"
                                :value="level.key"
                                :key="level.key"
                            >
                                {{ $t('loggingadmin.' + level.value) }}
                            </option>
                        </select>
                    </div>
                </div>

                <div class="table-responsive">
                    <table class="table table-striped">
                        <thead>
                            <tr>
                                <th scope="col">{{ $t('loggingadmin.Module') }}</th>
                                <th>{{ $t('loggingadmin.LogLevel') }}</th>
                            </tr>
                        </thead>
                        <tbody>
                            <tr
                                v-for="module in loggingList.loglevel.modules"
                                v-bind:key="module.name"
                                :data-id="module.name"
                                class="align-middle"
                            >
                                <td>{{ module.name }}</td>
                                <td>
                                    <select class="form-select" v-model="module.level">
                                        <option v-for="level in logLevelList" :value="level.key" :key="level.key">
                                            {{ $t('loggingadmin.' + level.value) }}
                                        </option>
                                    </select>
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </CardElement>

            <div class="alert alert-danger mt-3" role="alert" v-html="$t('loggingadmin.CoreHint')"></div>

            <FormFooter @reload="getLogConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import type { AlertResponse } from '@/types/AlertResponse';
import type { LoggingConfig } from '@/types/LoggingConfig';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
    },
    data() {
        return {
            dataLoading: true,
            alert: {} as AlertResponse,
            loggingList: {} as LoggingConfig,
            logLevelList: [
                { key: -1, value: 'log_inherit' },
                { key: 0, value: 'log_none' },
                { key: 1, value: 'log_error' },
                { key: 2, value: 'log_warn' },
                { key: 3, value: 'log_info' },
                { key: 4, value: 'log_debug' },
                { key: 5, value: 'log_verbose' },
            ],
        };
    },
    created() {
        this.getLogConfig();
    },
    methods: {
        getLogConfig() {
            this.dataLoading = true;
            fetch('/api/logging/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.loggingList = data;
                    this.dataLoading = false;
                });
        },
        saveLogConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.loggingList));

            fetch('/api/logging/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.alert = data;
                    this.alert.message = this.$t('apiresponse.' + data.code, data.param);
                    this.alert.show = true;
                });
        },
    },
});
</script>
