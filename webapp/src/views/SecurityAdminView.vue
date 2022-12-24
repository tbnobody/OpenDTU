<template>
    <BasePage :title="$t('securityadmin.SecuritySettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="savePasswordConfig">
            <div class="card">
                <div class="card-header text-bg-primary">{{ $t('securityadmin.AdminPassword') }}</div>
                <div class="card-body">

                    <InputElement :label="$t('securityadmin.Password')"
                                  v-model="securityConfigList.password"
                                  type="password" maxlength="64"/>

                    <InputElement :label="$t('securityadmin.RepeatPassword')"
                                  v-model="passwordRepeat"
                                  type="password" maxlength="64"/>

                    <div class="alert alert-secondary" role="alert" v-html="$t('securityadmin.PasswordHint')"></div>
                </div>
            </div>

            <div class="card mt-5">
                <div class="card-header text-bg-primary">{{ $t('securityadmin.Permissions') }}</div>
                <div class="card-body">

                    <InputElement :label="$t('securityadmin.ReadOnly')"
                                  v-model="securityConfigList.allow_readonly"
                                  type="checkbox" wide/>
                </div>
            </div>

            <button type="submit" class="btn btn-primary mb-3">{{ $t('securityadmin.Save') }}</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import InputElement from '@/components/InputElement.vue';
import { handleResponse, authHeader } from '@/utils/authentication';
import type { SecurityConfig } from '@/types/SecurityConfig';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        InputElement,
    },
    data() {
        return {
            dataLoading: true,
            alertMessage: "",
            alertType: "info",
            showAlert: false,

            securityConfigList: {} as SecurityConfig,
            passwordRepeat: "",
        };
    },
    created() {
        this.getPasswordConfig();
    },
    methods: {
        getPasswordConfig() {
            this.dataLoading = true;
            fetch("/api/security/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (data) => {
                        this.securityConfigList = data;
                        this.passwordRepeat = this.securityConfigList.password;
                        this.dataLoading = false;
                    }
                );
        },
        savePasswordConfig(e: Event) {
            e.preventDefault();

            if (this.securityConfigList.password != this.passwordRepeat) {
                this.alertMessage = "Passwords are not equal";
                this.alertType = "warning";
                this.showAlert = true;
                return;
            }

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.securityConfigList));

            fetch("/api/security/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>