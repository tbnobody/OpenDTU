<template>
    <BasePage :title="$t('fileadmin.ConfigManagement')" :isLoading="loading" :show-reload="true" @reload="getFileList">
        <BootstrapAlert v-model="alert.show" dismissible :variant="alert.type">
            {{ alert.message }}
        </BootstrapAlert>

        <CardElement :text="$t('fileadmin.BackupHeader')" textVariant="text-bg-primary">
            <div class="table-responsive">
                <table class="table">
                    <thead>
                        <tr>
                            <th scope="col">{{ $t('fileadmin.Name') }}</th>
                            <th>{{ $t('fileadmin.Size') }}</th>
                            <th>{{ $t('fileadmin.Action') }}</th>
                        </tr>
                    </thead>
                    <tbody ref="fileList">
                        <tr v-for="(file, index) in fileList" :key="index" :value="index">
                            <td>{{ file.name }}</td>
                            <td>{{ $n(file.size, 'byte') }}</td>
                            <td>
                                <a href="#" class="icon text-danger" :title="$t('fileadmin.Delete')">
                                    <BIconTrash v-on:click="onOpenModal(modalDelete, file)" /> </a
                                >&nbsp;
                                <a href="#" class="icon" :title="$t('fileadmin.Download')">
                                    <BIconDownload v-on:click="downloadFile(file.name)" />
                                </a>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </CardElement>

        <CardElement :text="$t('fileadmin.RestoreHeader')" textVariant="text-bg-primary" add-space center-content>
            <div v-if="!uploading && UploadError != ''">
                <p class="h1 mb-2">
                    <BIconExclamationCircleFill />
                </p>
                <span style="vertical-align: middle" class="ml-2">
                    {{ UploadError }}
                </span>
                <br />
                <br />
                <button class="btn btn-light" @click="clearUpload">
                    <BIconArrowLeft /> {{ $t('fileadmin.Back') }}
                </button>
            </div>

            <div v-else-if="!uploading && UploadSuccess">
                <span class="h1 mb-2">
                    <BIconCheckCircle />
                </span>
                <span> {{ $t('fileadmin.UploadSuccess') }} </span>
            </div>

            <div v-else-if="!uploading">
                <div class="row g-3 align-items-center form-group pt-2">
                    <div class="col-sm">
                        <select class="form-select" v-model="restoreFileSelect" @change="onUploadFileChange">
                            <option v-for="file in restoreList" :key="file.name" :value="file.name">
                                {{ file.descr }}
                            </option>
                        </select>
                    </div>
                    <div class="col-sm">
                        <input
                            class="form-control"
                            type="file"
                            ref="file"
                            accept=".json"
                            @change="onUploadFileChange"
                        />
                    </div>
                    <div class="col-sm">
                        <button class="btn btn-primary" @click="onUpload" :disabled="!isValidJson">
                            {{ $t('fileadmin.Restore') }}
                        </button>
                    </div>
                </div>
            </div>

            <div v-else-if="uploading">
                <div class="progress">
                    <div
                        class="progress-bar"
                        role="progressbar"
                        :style="{ width: progress + '%' }"
                        v-bind:aria-valuenow="progress"
                        aria-valuemin="0"
                        aria-valuemax="100"
                    >
                        {{ progress }}%
                    </div>
                </div>
            </div>

            <div class="alert alert-danger mt-3" role="alert" v-html="$t('fileadmin.RestoreHint')"></div>
        </CardElement>

        <CardElement :text="$t('fileadmin.ResetHeader')" textVariant="text-bg-primary" center-content add-space>
            <button class="btn btn-danger" @click="onFactoryResetModal">
                {{ $t('fileadmin.FactoryResetButton') }}
            </button>

            <div class="alert alert-danger mt-3" role="alert" v-html="$t('fileadmin.ResetHint')"></div>
        </CardElement>
    </BasePage>

    <ModalDialog modalId="fileDelete" small :title="$t('fileadmin.Delete')" :closeText="$t('fileadmin.Cancel')">
        {{
            $t('fileadmin.DeleteMsg', {
                name: selectedFile.name,
            })
        }}
        <template #footer>
            <button type="button" class="btn btn-danger" @click="onDelete">
                {{ $t('fileadmin.Delete') }}
            </button>
        </template>
    </ModalDialog>

    <ModalDialog modalId="factoryReset" small :title="$t('fileadmin.FactoryReset')" :closeText="$t('fileadmin.Cancel')">
        {{ $t('fileadmin.ResetMsg') }}
        <template #footer>
            <button type="button" class="btn btn-danger" @click="onFactoryResetPerform">
                {{ $t('fileadmin.ResetConfirm') }}
            </button>
        </template>
    </ModalDialog>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import ModalDialog from '@/components/ModalDialog.vue';
import type { AlertResponse } from '@/types/AlertResponse';
import type { FileInfo } from '@/types/File';
import { authHeader, handleResponse } from '@/utils/authentication';
import type { Schema } from '@/utils/structure';
import { hasStructure } from '@/utils/structure';
import { waitRestart } from '@/utils/waitRestart';
import * as bootstrap from 'bootstrap';
import {
    BIconArrowLeft,
    BIconCheckCircle,
    BIconDownload,
    BIconExclamationCircleFill,
    BIconTrash,
} from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        ModalDialog,
        BIconArrowLeft,
        BIconCheckCircle,
        BIconDownload,
        BIconExclamationCircleFill,
        BIconTrash,
    },
    data() {
        return {
            loading: true,
            fileList: [] as FileInfo[],
            selectedFile: {} as FileInfo,
            file: {} as Blob,
            modalDelete: {} as bootstrap.Modal,
            modalFactoryReset: {} as bootstrap.Modal,
            alert: {} as AlertResponse,
            uploading: false,
            progress: 0,
            UploadError: '',
            UploadSuccess: false,
            restoreFileSelect: 'config.json',
            restoreList: [
                {
                    name: 'config.json',
                    descr: 'Main Config (config.json)',
                    template: { cfg: 'object' } as Schema,
                },
                {
                    name: 'pin_mapping.json',
                    descr: 'Pin Mapping (pin_mapping.json)',
                    template: { name: 'string' } as Schema,
                },
                {
                    name: 'pack.lang.json',
                    descr: 'Language Pack (pack.lang.json)',
                    template: { meta: 'object' } as Schema,
                },
            ],
            isValidJson: false,
        };
    },
    mounted() {
        this.modalDelete = new bootstrap.Modal('#fileDelete');
        this.modalFactoryReset = new bootstrap.Modal('#factoryReset');
    },
    created() {
        this.getFileList();
    },
    methods: {
        getFileList() {
            this.loading = true;
            fetch('/api/file/list', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.fileList = data;
                    this.loading = false;
                });
        },
        downloadFile(filename: string) {
            fetch('/api/file/get?file=' + filename, { headers: authHeader() })
                .then((res) => res.blob())
                .then((blob) => {
                    const file = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.href = file;
                    a.download = filename;
                    document.body.appendChild(a);
                    a.click();
                    a.remove();
                });
        },
        callFileApiEndpoint(endpoint: string, jsonData: string) {
            const formData = new FormData();
            formData.append('data', jsonData);

            fetch('/api/file/' + endpoint, {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.getFileList();
                    this.alert = data;
                    this.alert.message = this.$t('apiresponse.' + data.code, data.param);
                    this.alert.show = true;
                });
        },
        onOpenModal(modal: bootstrap.Modal, file: FileInfo) {
            // deep copy File object for editing/deleting
            this.selectedFile = JSON.parse(JSON.stringify(file)) as FileInfo;
            modal.show();
        },
        onCloseModal(modal: bootstrap.Modal) {
            modal.hide();
        },
        onDelete() {
            this.callFileApiEndpoint('delete', JSON.stringify({ file: this.selectedFile.name }));
            this.onCloseModal(this.modalDelete);
        },
        onUploadFileChange() {
            const target = this.$refs.file as HTMLInputElement;
            if (target.files !== null) {
                this.file = target.files[0];
            }
            if (!this.file) return;

            // Read the file content
            const reader = new FileReader();
            reader.onload = (e) => {
                try {
                    const checkTemplate = this.restoreList.find((i) => i.name == this.restoreFileSelect)?.template;
                    // Parse the file content as JSON
                    let checkValue = JSON.parse(e.target?.result as string);
                    if (Array.isArray(checkValue)) {
                        checkValue = checkValue[0];
                    }

                    if (checkValue && checkTemplate && hasStructure(checkValue, checkTemplate)) {
                        this.isValidJson = true;
                        this.alert.show = false;
                    } else {
                        this.isValidJson = false;
                        this.alert.message = this.$t('fileadmin.InvalidJsonContent');
                    }
                } catch {
                    this.isValidJson = false;
                    this.alert.message = this.$t('fileadmin.InvalidJson');
                }

                if (!this.isValidJson) {
                    this.alert.type = 'warning';
                    this.alert.show = true;
                }
            };
            reader.readAsText(this.file);
        },
        onUpload() {
            this.uploading = true;
            const formData = new FormData();
            const target = this.$refs.file as HTMLInputElement; //  event.target as HTMLInputElement;
            if (target.files !== null && target.files?.length > 0) {
                this.file = target.files[0];
            } else {
                this.UploadError = this.$t('fileadmin.NoFileSelected');
                this.uploading = false;
                this.progress = 0;
                return;
            }
            const request = new XMLHttpRequest();
            request.addEventListener('load', () => {
                // request.response will hold the response from the server
                if (request.status === 200) {
                    this.UploadSuccess = true;
                    waitRestart(this.$router);
                } else if (request.status !== 500) {
                    this.UploadError = `[HTTP ERROR] ${request.statusText}`;
                } else {
                    this.UploadError = request.responseText;
                }
                this.uploading = false;
                this.progress = 0;
            });
            // Upload progress
            request.upload.addEventListener('progress', (e) => {
                this.progress = Math.trunc((e.loaded / e.total) * 100);
            });
            request.withCredentials = true;

            formData.append('config', this.file, 'config');
            request.open('post', '/api/file/upload?file=' + this.restoreFileSelect);
            authHeader().forEach((value, key) => {
                request.setRequestHeader(key, value);
            });
            request.send(formData);
        },
        clearUpload() {
            this.UploadError = '';
            this.UploadSuccess = false;
            this.getFileList();
        },
        onFactoryResetModal() {
            this.modalFactoryReset.show();
        },
        onFactoryResetCancel() {
            this.modalFactoryReset.hide();
        },
        onFactoryResetPerform() {
            const formData = new FormData();
            formData.append('data', JSON.stringify({ delete: true }));

            fetch('/api/file/delete_all', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    this.alert.message = this.$t('apiresponse.' + response.code, response.param);
                    this.alert.type = response.type;
                    this.alert.show = true;
                });
            this.modalFactoryReset.hide();
        },
    },
});
</script>
