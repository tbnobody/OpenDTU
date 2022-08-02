<template>
    <div class="container-xxl" role="main">
        <div class="page-header">
            <h1>Ve.direct Settings</h1>
        </div>
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <div class="text-center" v-if="dataLoading">
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </div>
        <template v-if="!dataLoading">
            <form @submit="saveVedirectConfig">
                <div class="card">
                    <div class="card-header text-white bg-primary">Ve.direct Configuration</div>
                    <div class="card-body">
                        <div class="row mb-3">
                            <label class="col-sm-4 form-check-label" for="inputVedirect">Enable Ve.direct</label>
                            <div class="col-sm-8">
                                <div class="form-check form-switch">
                                    <input class="form-check-input" type="checkbox" id="inputVedirect"
                                        v-model="vedirectConfigList.vedirect_enabled" />
                                </div>
                            </div>
                        </div>
                        <div class="row mb-3" v-show="vedirectConfigList.vedirect_enabled">
                            <label class="col-sm-2 form-check-label" for="inputTls">Send only updates</label>
                            <div class="col-sm-10">
                                <div class="form-check form-switch">
                                    <input class="form-check-input" type="checkbox" id="inputTls"
                                        v-model="vedirectConfigList.vedirect_updatesonly" />
                                </div>
                            </div>
                        </div>
                    </div>
                </div>

                <button type="submit" class="btn btn-primary mb-3">Save</button>
            </form>
        </template>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BootstrapAlert from "@/components/partials/BootstrapAlert.vue";

export default defineComponent({
    components: {
        BootstrapAlert,
    },
    data() {
        return {
            dataLoading: true,
            vedirectConfigList: {
                vedirect_enabled: false,
                vedirect_updatesonly: true,
            },
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
            fetch("/api/vedirect/config")
                .then((response) => response.json())
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
                body: formData,
            })
                .then(function (response) {
                    if (response.status != 200) {
                        throw response.status;
                    } else {
                        return response.json();
                    }
                })
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