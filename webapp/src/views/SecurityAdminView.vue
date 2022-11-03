<template>
    <BasePage :title="'Security Settings'" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="savePasswordConfig">
            <div class="card">
                <div class="card-header text-bg-primary">Admin password</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputPassword" class="col-sm-2 col-form-label">Password:</label>
                        <div class="col-sm-10">
                            <input type="password" class="form-control" id="inputPassword" maxlength="64"
                                placeholder="Password" v-model="securityConfigList.password" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPasswordRepeat" class="col-sm-2 col-form-label">Repeat Password:</label>
                        <div class="col-sm-10">
                            <input type="password" class="form-control" id="inputPasswordRepeat" maxlength="64"
                                placeholder="Password" v-model="passwordRepeat" />
                        </div>
                    </div>

                    <div class="alert alert-secondary" role="alert">
                        <b>Hint:</b>
                        The administrator password is used to access this web interface (user 'admin'), but also to
                        connect to the device when in AP mode. It must be 8..64 characters.
                    </div>

                </div>
            </div>
            <button type="submit" class="btn btn-primary mb-3">Save</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';
import type { SecurityConfig } from '@/types/SecurityConfig';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
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
            fetch("/api/security/password", { headers: authHeader() })
                .then(handleResponse)
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

            fetch("/api/security/password", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then(handleResponse)
                .then(
                    (response) => {
                        this.alertMessage = response.message;
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>