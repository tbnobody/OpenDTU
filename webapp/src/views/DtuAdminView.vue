<template>
    <BasePage :title="'DTU Settings'" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveDtuConfig">
            <div class="card">
                <div class="card-header text-bg-primary">DTU Configuration</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputDtuSerial" class="col-sm-2 col-form-label">Serial:</label>
                        <div class="col-sm-10">
                            <input type="number" class="form-control" id="inputDtuSerial" min="1" max="199999999999"
                                placeholder="DTU Serial" v-model="dtuConfigList.dtu_serial" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPollInterval" class="col-sm-2 col-form-label">Poll Interval:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="inputPollInterval" min="1" max="86400"
                                    placeholder="Poll Interval in Seconds" v-model="dtuConfigList.dtu_pollinterval"
                                    aria-describedby="pollIntervalDescription" />
                                <span class="input-group-text" id="pollIntervalDescription">seconds</span>
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputTimezone" class="col-sm-2 col-form-label">PA Level:</label>
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
            <button type="submit" class="btn btn-primary mb-3">Save</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';
import type { DtuConfig } from "@/types/DtuConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
    },
    data() {
        return {
            dataLoading: true,
            dtuConfigList: {} as DtuConfig,
            palevelList: [
                { key: 0, value: "Minimum (-18 dBm)" },
                { key: 1, value: "Low (-12 dBm)" },
                { key: 2, value: "High (-6 dBm)" },
                { key: 3, value: "Maximum (0 dBm)" },
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
                .then((response) => handleResponse(response, this.$emitter))
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