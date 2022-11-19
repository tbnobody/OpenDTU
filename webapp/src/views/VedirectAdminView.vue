<template>
    <BasePage :title="'Ve.direct  Settings'" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveVedirectConfig">
            <div class="card">
                <div class="card-header text-bg-primary">Ve.direct  Configuration</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputVedirect">Enable Ve.direct</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputVedirect"
                                    v-model="vedirectConfigList.vedirect_enabled" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3" v-show="vedirectConfigList.vedirect_enabled">
                        <label for="inputPollInterval" class="col-sm-2 col-form-label">Poll Interval:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="inputPollInterval" min="1" max="86400"
                                    placeholder="Poll Interval in Seconds" v-model="vedirectConfigList.vedirect_pollinterval"
                                    aria-describedby="pollIntervalDescription" />
                                <span class="input-group-text" id="pollIntervalDescription">seconds</span>
                            </div>
                        </div>
                    </div>
                
                    <div class="row mb-3" v-show="vedirectConfigList.vedirect_enabled">
                        <label class="col-sm-2 form-check-label" for="inputUpdates">Send only updates</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputUpdates"
                                    v-model="vedirectConfigList.vedirect_updatesonly" />
                            </div>
                        </div>
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
import type { VedirectConfig } from "@/types/VedirectConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
    },
    data() {
        return {
            dataLoading: true,
            vedirectConfigList: {} as VedirectConfig,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    created() {
        this.getVedirectConfig();
    },
    methods: {
        getVedirectConfig() {
            this.dataLoading = true;
            fetch("/api/vedirect/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter))
                .then((data) => {
                    this.vedirectConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveVedirectConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.vedirectConfigList));

            fetch("/api/vedirect/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter))
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