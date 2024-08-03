<template>
    <BasePage :title="$t('securityadmin.SecuritySettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="savePasswordConfig">
            <CardElement :text="$t('securityadmin.AdminPassword')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('securityadmin.Password')"
                    v-model="securityConfigList.password"
                    type="password"
                    maxlength="64"
                />

                <InputElement
                    :label="$t('securityadmin.RepeatPassword')"
                    v-model="passwordRepeat"
                    type="password"
                    maxlength="64"
                />

                <div class="alert alert-secondary" role="alert" v-html="$t('securityadmin.PasswordHint')"></div>
            </CardElement>

            <CardElement :text="$t('securityadmin.Permissions')" textVariant="text-bg-primary" add-space>
                <InputElement
                    :label="$t('securityadmin.ReadOnly')"
                    v-model="securityConfigList.allow_readonly"
                    type="checkbox"
                    wide
                />
            </CardElement>

            <FormFooter @reload="getPasswordConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { SecurityConfig } from '@/types/SecurityConfig';
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
            alertMessage: '',
            alertType: 'info',
            showAlert: false,

            securityConfigList: {} as SecurityConfig,
            passwordRepeat: '',
        };
    },
    created() {
        this.getPasswordConfig();
    },
    methods: {
        getPasswordConfig() {
            this.dataLoading = true;
            fetch('/api/security/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.securityConfigList = data;
                    this.passwordRepeat = this.securityConfigList.password;
                    this.dataLoading = false;
                });
        },
        savePasswordConfig(e: Event) {
            e.preventDefault();

            if (this.securityConfigList.password != this.passwordRepeat) {
                this.alertMessage = 'Passwords are not equal';
                this.alertType = 'warning';
                this.showAlert = true;
                return;
            }

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.securityConfigList));

            fetch('/api/security/config', {
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
