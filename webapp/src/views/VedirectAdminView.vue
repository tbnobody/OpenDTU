<template>
    <BasePage :title="$t('vedirectadmin.VedirectSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveVedirectConfig">
            <CardElement :text="$t('vedirectadmin.VedirectConfiguration')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('vedirectadmin.EnableVedirect')"
                    v-model="vedirectConfigList.vedirect_enabled"
                    type="checkbox"
                    wide
                />
            </CardElement>

            <CardElement
                :text="$t('vedirectadmin.VedirectParameter')"
                textVariant="text-bg-primary"
                add-space
                v-show="vedirectConfigList.vedirect_enabled"
            >
                <InputElement
                    :label="$t('vedirectadmin.VerboseLogging')"
                    v-model="vedirectConfigList.verbose_logging"
                    type="checkbox"
                    wide
                />

                <InputElement
                    :label="$t('vedirectadmin.UpdatesOnly')"
                    v-model="vedirectConfigList.vedirect_updatesonly"
                    type="checkbox"
                    wide
                />
            </CardElement>

            <FormFooter @reload="getVedirectConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { VedirectConfig } from '@/types/VedirectConfig';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
        InputElement,
    },
    data() {
        return {
            dataLoading: true,
            vedirectConfigList: {} as VedirectConfig,
            alertMessage: '',
            alertType: 'info',
            showAlert: false,
        };
    },
    created() {
        this.getVedirectConfig();
    },
    methods: {
        getVedirectConfig() {
            this.dataLoading = true;
            fetch('/api/vedirect/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.vedirectConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveVedirectConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.vedirectConfigList));

            fetch('/api/vedirect/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                    this.alertType = response.type;
                    this.showAlert = true;
                });
        },
    },
});
</script>
