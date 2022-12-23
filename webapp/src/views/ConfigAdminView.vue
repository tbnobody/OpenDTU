<template>
    <BasePage :title="$t('configadmin.ConfigManagement')" :isLoading="loading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <div class="card">
            <div class="card-header text-bg-primary">{{ $t('configadmin.BackupHeader') }}</div>
            <div class="card-body text-center">
                {{ $t('configadmin.BackupConfig') }}
                <button class="btn btn-primary" @click="downloadConfig">{{ $t('configadmin.Backup') }}
                </button>
            </div>
        </div>

        <div class="card mt-5">
            <div class="card-header text-bg-primary">{{ $t('configadmin.RestoreHeader') }}</div>
            <div class="card-body text-center">

                <div v-if="!uploading && UploadError != ''">
                    <p class="h1 mb-2">
                        <BIconExclamationCircleFill />
                    </p>
                    <span style="vertical-align: middle" class="ml-2">
                        {{ UploadError }}
                    </span>
                    <br />
                    <br />
                    <button class="btn btn-light" @click="clear">
                        <BIconArrowLeft /> {{ $t('configadmin.Back') }}
                    </button>
                </div>

                <div v-else-if="!uploading && UploadSuccess">
                    <span class="h1 mb-2">
                        <BIconCheckCircle />
                    </span>
                    <span> {{ $t('configadmin.UploadSuccess') }} </span>
                    <br />
                    <br />
                    <button class="btn btn-primary" @click="clear">
                        <BIconArrowLeft /> {{ $t('configadmin.Back') }}
                    </button>
                </div>

                <div v-else-if="!uploading">
                    <div class="form-group pt-2 mt-3">
                        <input class="form-control" type="file" ref="file" accept=".json" @change="uploadConfig" />
                    </div>
                </div>

                <div v-else-if="uploading">
                    <div class="progress">
                        <div class="progress-bar" role="progressbar" :style="{ width: progress + '%' }"
                            v-bind:aria-valuenow="progress" aria-valuemin="0" aria-valuemax="100">
                            {{ progress }}%
                        </div>
                    </div>
                </div>

                <div class="alert alert-danger mt-3" role="alert" v-html="$t('configadmin.RestoreHint')"></div>
            </div>
        </div>

        <div class="card mt-5">
            <div class="card-header text-bg-primary">{{ $t('configadmin.ResetHeader') }}</div>
            <div class="card-body text-center">

                <button class="btn btn-danger" @click="onFactoryResetModal">{{ $t('configadmin.FactoryResetButton') }}
                </button>

                <div class="alert alert-danger mt-3" role="alert" v-html="$t('configadmin.ResetHint')"></div>
            </div>
        </div>
    </BasePage>

    <div class="modal" id="factoryReset" tabindex="-1">
        <div class="modal-dialog">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title">{{ $t('configadmin.FactoryReset') }}</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    {{ $t('configadmin.ResetMsg') }}
                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="onFactoryResetCancel"
                        data-bs-dismiss="modal">{{ $t('configadmin.Cancel') }}</button>
                    <button type="button" class="btn btn-danger" @click="onFactoryResetPerform">
                        {{ $t('configadmin.ResetConfirm') }}
                    </button>
                </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import {
    BIconExclamationCircleFill,
    BIconArrowLeft,
    BIconCheckCircle
} from 'bootstrap-icons-vue';
import * as bootstrap from 'bootstrap';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader, isLoggedIn } from '@/utils/authentication';

export default defineComponent({
    components: {
        BasePage,
        BIconExclamationCircleFill,
        BIconArrowLeft,
        BIconCheckCircle,
        BootstrapAlert,
    },
    data() {
        return {
            modalFactoryReset: {} as bootstrap.Modal,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
            loading: true,
            uploading: false,
            progress: 0,
            UploadError: "",
            UploadSuccess: false,
            file: {} as Blob,
        };
    },
    mounted() {
        if (!isLoggedIn()) {
            this.$router.push({ path: "/login", query: { returnUrl: this.$router.currentRoute.value.fullPath } });
        }
        this.modalFactoryReset = new bootstrap.Modal('#factoryReset');
        this.loading = false;
    },
    methods: {
        onFactoryResetModal() {
            this.modalFactoryReset.show();
        },
        onFactoryResetCancel() {
            this.modalFactoryReset.hide();
        },
        onFactoryResetPerform() {
            const formData = new FormData();
            formData.append("data", JSON.stringify({ delete: true }));

            fetch("/api/config/delete", {
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
                )
            this.modalFactoryReset.hide();
        },
        downloadConfig() {
            fetch("/api/config/get", { headers: authHeader() })
                .then(res => res.blob())
                .then(blob => {
                    var file = window.URL.createObjectURL(blob);
                    var a = document.createElement('a');
                    a.href = file;
                    a.download = "config.json";
                    document.body.appendChild(a);
                    a.click();
                    a.remove();
                });
        },
        uploadConfig(event: Event | null) {
            this.uploading = true;
            const formData = new FormData();
            if (event !== null) {
                const target = event.target as HTMLInputElement;
                if (target.files !== null) {
                    this.file = target.files[0];
                }
            }
            const request = new XMLHttpRequest();
            request.addEventListener("load", () => {
                // request.response will hold the response from the server
                if (request.status === 200) {
                    this.UploadSuccess = true;
                } else if (request.status !== 500) {
                    this.UploadError = `[HTTP ERROR] ${request.statusText}`;
                } else {
                    this.UploadError = request.responseText;
                }
                this.uploading = false;
                this.progress = 0;
            });
            // Upload progress
            request.upload.addEventListener("progress", (e) => {
                this.progress = Math.trunc((e.loaded / e.total) * 100);
            });
            request.withCredentials = true;

            formData.append("config", this.file, "config");
            request.open("post", "/api/config/upload");
            authHeader().forEach((value, key) => {
                request.setRequestHeader(key, value);
            });
            request.send(formData);
        },
        clear() {
            this.UploadError = "";
            this.UploadSuccess = false;
        },
    },
});
</script>