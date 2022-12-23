<template>
    <BasePage :title="$t('dtuadmin.DtuSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveDtuConfig">
            <div class="card">
                <div class="card-header text-bg-primary">{{ $t('dtuadmin.DtuConfiguration') }}</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputDtuSerial" class="col-sm-2 col-form-label">{{ $t('dtuadmin.Serial') }}
                            <BIconInfoCircle v-tooltip :title="$t('dtuadmin.SerialHint')" />
                        </label>
                        <div class="col-sm-10">
                            <input type="number" class="form-control" id="inputDtuSerial" min="1" max="199999999999"
                                v-model="dtuConfigList.dtu_serial" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPollInterval" class="col-sm-2 col-form-label">
                            {{ $t('dtuadmin.PollInterval') }}
                        </label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="inputPollInterval" min="1" max="86400"
                                    v-model="dtuConfigList.dtu_pollinterval"
                                    aria-describedby="pollIntervalDescription" />
                                <span class="input-group-text" id="pollIntervalDescription">
                                    {{ $t('dtuadmin.Seconds') }}
                                </span>
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputTimezone" class="col-sm-2 col-form-label">
                            {{ $t('dtuadmin.PaLevel') }}
                            <BIconInfoCircle v-tooltip :title="$t('dtuadmin.PaLevelHint')" />
                        </label>
                        <div class="col-sm-10">
                            <select class="form-select" v-model="dtuConfigList.dtu_palevel">
                                <option v-for="palevel in palevelList" :key="palevel.key" :value="palevel.key">
                                    {{ palevel.value }}
                                </option>
                            </select>
                        </div>
                    </div>
                </div>
            </div>
            <button type="submit" class="btn btn-primary mb-3">{{ $t('dtuadmin.Save') }}</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';
import type { DtuConfig } from "@/types/DtuConfig";
import {
    BIconInfoCircle,
} from 'bootstrap-icons-vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        BIconInfoCircle,
    },
    data() {
        return {
            dataLoading: true,
            dtuConfigList: {} as DtuConfig,
            palevelList: [
                { key: 0, value: this.$t('dtuadmin.Min') },
                { key: 1, value: this.$t('dtuadmin.Low') },
                { key: 2, value: this.$t('dtuadmin.High') },
                { key: 3, value: this.$t('dtuadmin.Max') },
            ],
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    created() {
        this.getDtuConfig();
    },
    methods: {
        getDtuConfig() {
            this.dataLoading = true;
            fetch("/api/dtu/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (data) => {
                        this.dtuConfigList = data;
                        this.dataLoading = false;
                    }
                );
        },
        saveDtuConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.dtuConfigList));

            fetch("/api/dtu/config", {
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