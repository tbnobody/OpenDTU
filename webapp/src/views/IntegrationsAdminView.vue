<template>
    <BasePage :title="$t('integrationsadmin.IntegrationSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveIntegrationsConfig">
            <CardElement :text="$t('integrationsadmin.goecontroller')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('integrationsadmin.goecontrollerEnabled')"
                    v-model="integrationsConfig.goe_ctrl_enabled"
                    type="checkbox"
                    wide
                    :tooltip="$t('integrationsadmin.goecontrollerEnabledHint')"
                />

                <InputElement
                    v-show="integrationsConfig.goe_ctrl_enabled"
                    :label="$t('integrationsadmin.goecontrollerEnableHomeCategory')"
                    v-model="integrationsConfig.goe_ctrl_publish_home_category"
                    type="checkbox"
                    wide
                    :tooltip="$t('integrationsadmin.goecontrollerEnableHomeCategoryHint')"
                />

                <InputElement
                    v-show="integrationsConfig.goe_ctrl_enabled"
                    :label="$t('integrationsadmin.goecontrollerHostname')"
                    v-model="integrationsConfig.goe_ctrl_hostname"
                    type="text"
                    placeholder="go-econtroller_XXXXXX"
                />

                <InputElement
                    v-show="integrationsConfig.goe_ctrl_enabled"
                    :label="$t('integrationsadmin.goecontrollerUpdateInterval')"
                    v-model="integrationsConfig.goe_ctrl_update_interval"
                    type="number"
                    min="3"
                    max="65535"
                    :postfix="$t('mqttadmin.Seconds')"
                />
            </CardElement>
            <FormFooter @reload="getIntegrationsConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';
import type { IntegrationsConfig } from '@/types/IntegrationsConfig';

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
            integrationsConfig: {} as IntegrationsConfig,
            alertMessage: '',
            alertType: 'info',
            showAlert: false,
        };
    },
    created() {
        this.getIntegrationsConfig();
    },
    methods: {
        getIntegrationsConfig() {
            this.dataLoading = true;
            fetch('/api/integrations/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.integrationsConfig = data;
                    this.dataLoading = false;
                });
        },
        saveIntegrationsConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.integrationsConfig));

            fetch('/api/integrations/config', {
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
