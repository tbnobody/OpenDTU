<template>
    <BasePage :title="$t('configadmin.ConfigManagement')" :isLoading="loading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <CardElement :text="$t('configadmin.BackupHeader')" textVariant="text-bg-primary" center-content>
            <div class="row g-3 align-items-center">
                <div class="col-sm">
                    {{ $t('configadmin.BackupConfig') }}
                </div>
                <div class="col-sm">
                    <select class="form-select" v-model="backupFileSelect">
                        <option v-for="(file) in fileList.configs" :key="file.name" :value="file.name">
                            {{ file.name }}
                        </option>
                    </select>
                </div>
                <div class="col-sm">
                    <button class="btn btn-primary" @click="downloadConfig">{{ $t('configadmin.Backup') }}
                    </button>
                </div>
            </div>
        </CardElement>

        <CardElement :text="$t('configadmin.RestoreHeader')" textVariant="text-bg-primary" center-content add-space>
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
                <div class="row g-3 align-items-center form-group pt-2">
                    <div class="col-sm">
                        <select class="form-select" v-model="restoreFileSelect">
                            <option selected value="config.json">Main Config (config.json)</option>
                            <option selected value="pin_mapping.json">Pin Mapping (pin_mapping.json)</option>
                        </select>
                    </div>
                    <div class="col-sm">
                        <input class="form-control" type="file" ref="file" accept=".json" />
                    </div>
                    <div class="col-sm">
                        <button class="btn btn-primary" @click="uploadConfig">{{ $t('configadmin.Restore') }}
                        </button>
                    </div>
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
        </CardElement>

        <CardElement :text="$t('configadmin.ResetHeader')" textVariant="text-bg-primary" center-content add-space>
            <button class="btn btn-danger" @click="onFactoryResetModal">{{ $t('configadmin.FactoryResetButton') }}
            </button>

            <div class="alert alert-danger mt-3" role="alert" v-html="$t('configadmin.ResetHint')"></div>
        </CardElement>
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
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import type { ConfigFileList } from '@/types/Config';
import { authHeader, handleResponse } from '@/utils/authentication';
import * as bootstrap from 'bootstrap';
import {
    BIconArrowLeft,
    BIconCheckCircle,
    BIconExclamationCircleFill
} from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        BIconArrowLeft,
        BIconCheckCircle,
        BIconExclamationCircleFill,
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
            fileList: {} as ConfigFileList,
            backupFileSelect: "",
            restoreFileSelect: "config.json",
        };
    },
    mounted() {
        this.modalFactoryReset = new bootstrap.Modal('#factoryReset');
    },
    created() {
        this.getFileList();
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
        getFileList() {
            this.loading = true;
            fetch("/api/config/list", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.fileList = data;
                    if (this.fileList.configs) {
                        this.backupFileSelect = this.fileList.configs[0].name;
                    }
                    this.loading = false;
                });
        },
        downloadConfig() {
            fetch("/api/config/get?file=" + this.backupFileSelect, { headers: authHeader() })
                .then(res => res.blob())
                .then(blob => {
                    var file = window.URL.createObjectURL(blob);
                    var a = document.createElement('a');
                    a.href = file;
                    a.download = this.backupFileSelect;
                    document.body.appendChild(a);
                    a.click();
                    a.remove();
                });
        },
        uploadConfig() {
            this.uploading = true;
            const formData = new FormData();
            const target = this.$refs.file as HTMLInputElement; //  event.target as HTMLInputElement;
            if (target.files !== null && target.files?.length > 0) {
                this.file = target.files[0];
            } else {
                this.UploadError = this.$t("configadmin.NoFileSelected");
                this.uploading = false;
                this.progress = 0;
                return;
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
            request.open("post", "/api/config/upload?file=" + this.restoreFileSelect);
            authHeader().forEach((value, key) => {
                request.setRequestHeader(key, value);
            });
            request.send(formData);
        },
        clear() {
            this.UploadError = "";
            this.UploadSuccess = false;
            this.getFileList();
        },
    },
});
</script>