<template>
    <BasePage :title="$t('pluginadmin.PluginSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="alert.show" dismissible :variant="alert.type">
            {{ alert.message }}
        </BootstrapAlert>

        <CardElement :text="$t('pluginadmin.PluginList')" textVariant="text-bg-primary" add-space>
            <div class="table-responsive">
                <table class="table">
                    <thead>
                        <tr>
                            <th scope="col">{{ $t('pluginadmin.Name') }}</th>
                            <th>{{ $t('pluginadmin.Status') }}</th>
                            <th>{{ $t('pluginadmin.Action') }}</th>
                        </tr>
                    </thead>
                    <tbody ref="pList">
                        <tr v-for="plugin in plugins" v-bind:key="plugin['name']" :data-id="plugin['name']">
                            <td>{{ plugin['name'] }}</td>
                            <td>
                                {{ plugin['enabled'] }}
                            </td>
                            <td>
                                <a href="#" class="icon" :title="$t('pluginadmin.EditPlugin')">
                                    <BIconPencil v-on:click="onOpenModal(modal, plugin)" />
                                </a>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </CardElement>
    </BasePage>

    <div class="modal" id="pluginEdit" tabindex="-1">
        <div class="modal-dialog modal-lg">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title">{{ $t('pluginadmin.EditPlugin') }}</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    <form>
                        <div class="mb-3">
                            <label for="plugin-id" class="col-form-label">
                                {{ $t('pluginadmin.Plugin') }}
                            </label>
                            <p v-for="(value, propertyName)  in selectedPluginData">
                            <!-- {{ propertyName }}:{{ value }}:{{ idx }} -->
                            <label class="col-sm-2 col-form-label">{{ propertyName }}: </label>
                            <span v-if="typeof(value)==='number'" >
                                <input v-model="selectedPluginData[propertyName]" type="number"/>
                            </span>
                            <span v-else-if="typeof(value)==='boolean'">
                                <input v-model="selectedPluginData[propertyName]" type="checkbox"/>
                            </span>
                            <span v-else>
                                <input v-model="selectedPluginData[propertyName]" type="text" maxlength="32"/>
                            </span>
                            
                            </p>
                        </div>
                    </form>

                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="onCloseModal(modal)"
                        data-bs-dismiss="modal">{{ $t('pluginadmin.Cancel') }}</button>
                    <button type="button" class="btn btn-primary" @click="onEditSubmit">
                        {{ $t('pluginadmin.Save') }}</button>
                </div>
            </div>
        </div>
    </div>

</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import InputElement from '@/components/InputElement.vue';
import Sortable from 'sortablejs';
import { authHeader, handleResponse } from '@/utils/authentication';
import * as bootstrap from 'bootstrap';

import {
    BIconInfoCircle,
    BIconPencil,
    BIconTrash,
    BIconArrowDown,
    BIconArrowUp,
    BIconGripHorizontal,
} from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

declare interface OpenDTUPlugin {
    name: string;
    id: number;
    enabled: boolean;
}

declare interface AlertResponse {
    message: string;
    type: string;
    code: number;
    show: boolean;
}

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        InputElement,
        BIconInfoCircle,
        BIconPencil,
        BIconTrash,
        BIconArrowDown,
        BIconArrowUp,
        BIconGripHorizontal,
    },
    data() {
        return {
            modal: {} as bootstrap.Modal,
            newPluginData: {},
            selectedPluginData: {} as object,
            plugins: [],
            dataLoading: true,
            alert: {} as AlertResponse,
            sortable: {} as Sortable,
        };
    },
    mounted() {
        this.modal = new bootstrap.Modal('#pluginEdit');
    },
    created() {
        this.getPlugins();
    },
    methods: {
        getPlugins() {
            this.dataLoading = true;
            fetch("/api/plugin/list", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.plugins = data.plugins;
                    this.dataLoading = false;

                    this.$nextTick(() => {
                        const table = this.$refs.pList as HTMLElement;

                        this.sortable = Sortable.create(table, {
                            sort: true,
                            handle: '.drag-handle',
                            animation: 150,
                            draggable: 'tr',
                        });
                    });
                });
        },
        callPluginApiEndpoint(endpoint: string, jsonData: string) {
            const formData = new FormData();
            formData.append("data", jsonData);

            fetch("/api/plugin/" + endpoint, {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.getPlugins();
                    this.alert = data;
                    this.alert.message = this.$t('apiresponse.' + data.code, data.param);
                    this.alert.show = true;
                });
        },
        onEditSubmit() {
            this.callPluginApiEndpoint("edit", JSON.stringify(this.selectedPluginData));
            this.onCloseModal(this.modal);
        },
        onOpenModal(modal: bootstrap.Modal, pplugin: {}) {
            // deep copy object for editing/deleting
            this.selectedPluginData = JSON.parse(JSON.stringify(pplugin));;
            modal.show();
        },
        onCloseModal(modal: bootstrap.Modal) {
            modal.hide();
        },
        onSaveOrder() {
            this.callPluginApiEndpoint("order", JSON.stringify({ order: this.sortable.toArray() }));
        },
    },
});
</script>

<style>
.drag-handle {
    cursor: grab;
}
</style>