<template>
    <BasePage :title="'Inverter Settings'" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <div class="card">
            <div class="card-header text-bg-primary">Add a new Inverter</div>
            <div class="card-body">
                <form class="form-inline" v-on:submit.prevent="onSubmit">
                    <div class="form-group">
                        <label>Serial</label>
                        <input v-model="inverterData.serial" type="number" class="form-control ml-sm-2 mr-sm-4 my-2"
                            required />
                    </div>
                    <div class="form-group">
                        <label>Name</label>
                        <input v-model="inverterData.name" type="text" class="form-control ml-sm-2 mr-sm-4 my-2"
                            maxlength="31" required />
                    </div>
                    <div class="ml-auto text-right">
                        <button type="submit" class="btn btn-primary my-2">Add</button>
                    </div>
                    <div class="alert alert-secondary" role="alert">
                        <b>Hint:</b> You can set additional parameters after you have created the inverter. Use the pen
                        icon in the inverter list.
                    </div>
                </form>
            </div>
        </div>

        <div class="card mt-5">
            <div class="card-header text-bg-primary">Inverter List</div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table">
                        <thead>
                            <tr>
                                <th scope="col">Serial</th>
                                <th>Name</th>
                                <th>Type</th>
                                <th>Action</th>
                            </tr>
                        </thead>
                        <tbody>
                            <tr v-for="inverter in sortedInverters" v-bind:key="inverter.id">

                                <td>
                                    {{ inverter.serial }}
                                </td>
                                <td>
                                    {{ inverter.name }}
                                </td>
                                <td>
                                    {{ inverter.type }}
                                </td>
                                <td>
                                    <a href="#" class="icon text-danger" title="Delete inverter">
                                        <BIconTrash v-on:click="onDeleteModal(inverter)" />
                                    </a>&nbsp;
                                    <a href="#" class="icon" title="Edit inverter">
                                        <BIconPencil v-on:click="onEdit(inverter)" />
                                    </a>
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
    </BasePage>

    <div class="modal" id="inverterEdit" tabindex="-1">
        <div class="modal-dialog">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title">Edit Inverter</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">

                    <form>
                        <div class="mb-3">
                            <label for="inverter-serial" class="col-form-label">Serial:</label>
                            <input v-model="editInverterData.serial" type="number" id="inverter-serial"
                                class="form-control" />
                        </div>
                        <div class="mb-3">
                            <label for="inverter-name" class="col-form-label">Name:</label>
                            <input v-model="editInverterData.name" type="text" id="inverter-name" class="form-control"
                                maxlength="31" />
                        </div>

                        <div class="mb-3" v-for="(max, index) in editInverterData.max_power" :key="`${index}`">
                            <label :for="`inverter-max_${index}`" class="col-form-label">Max power string {{ index +
                            1
                            }}:</label>
                            <div class="input-group">
                                <input type="number" class="form-control" :id="`inverter-max_${index}`" min="0"
                                    v-model="editInverterData.max_power[index]"
                                    :aria-describedby="`inverter-maxDescription_${index} inverter-maxHelpText_${index}`" />
                                <span class="input-group-text" :id="`inverter-maxDescription_${index}`">W</span>
                            </div>
                            <div :id="`inverter-maxHelpText_${index}`" class="form-text">This value is used to
                                calculate the Irradiation.</div>
                        </div>
                    </form>

                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="onCancel"
                        data-bs-dismiss="modal">Cancel</button>
                    <button type="button" class="btn btn-primary" @click="onEditSubmit(editId)">Save
                        changes</button>
                </div>
            </div>
        </div>
    </div>

    <div class="modal" id="inverterDelete" tabindex="-1">
        <div class="modal-dialog">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title">Delete Inverter</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    Are you sure you want to delete the inverter "{{ deleteInverterData.name }}" with serial number
                    {{ deleteInverterData.serial }}?
                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="onDeleteCancel"
                        data-bs-dismiss="modal">Cancel</button>
                    <button type="button" class="btn btn-danger" @click="onDelete(deleteId.toString())">Delete</button>
                </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import {
    BIconTrash,
    BIconPencil
} from 'bootstrap-icons-vue';
import * as bootstrap from 'bootstrap';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';

declare interface Inverter {
    id: string,
    serial: number,
    name: string,
    type: string
    max_power: number[]
}

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        BIconTrash,
        BIconPencil,
    },
    data() {
        return {
            modal: {} as bootstrap.Modal,
            modalDelete: {} as bootstrap.Modal,
            deleteId: -1,
            editId: "-1",
            inverterData: {} as Inverter,
            editInverterData: {} as Inverter,
            deleteInverterData: {} as Inverter,
            inverters: [] as Inverter[],
            dataLoading: true,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    mounted() {
        this.modal = new bootstrap.Modal('#inverterEdit');
        this.modalDelete = new bootstrap.Modal('#inverterDelete');
    },
    created() {
        this.getInverters();
    },
    computed: {
        sortedInverters(): Inverter[] {
            return this.inverters.slice().sort((a, b) => {
                return a.serial - b.serial;
            });
        },
    },
    methods: {
        getInverters() {
            this.dataLoading = true;
            fetch("/api/inverter/list", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter))
                .then((data) => {
                    this.inverters = data.inverter;
                    this.dataLoading = false;
                });
        },
        onSubmit() {
            const formData = new FormData();
            formData.append("data", JSON.stringify(this.inverterData));

            fetch("/api/inverter/add", {
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
                )
                .then(() => { this.getInverters() });

            this.inverterData.serial = 0;
            this.inverterData.name = "";
        },
        onDeleteModal(inverter: Inverter) {
            this.modalDelete.show();
            this.deleteInverterData.serial = inverter.serial;
            this.deleteInverterData.name = inverter.name;
            this.deleteInverterData.type = inverter.type;
            this.deleteId = +inverter.id;
        },
        onDeleteCancel() {
            this.deleteId = -1;
            this.deleteInverterData.serial = 0;
            this.deleteInverterData.name = "";
            this.deleteInverterData.max_power = [];
            this.modalDelete.hide();
        },
        onDelete(id: string) {
            const formData = new FormData();
            formData.append("data", JSON.stringify({ id: id }));

            fetch("/api/inverter/del", {
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
                )
                .then(() => { this.getInverters() });
            this.deleteId = -1;
            this.deleteInverterData.serial = 0;
            this.deleteInverterData.name = "";
            this.deleteInverterData.max_power = [];
            this.modalDelete.hide();
        },
        onEdit(inverter: Inverter) {
            this.modal.show();
            this.editId = inverter.id;
            this.editInverterData.serial = inverter.serial;
            this.editInverterData.name = inverter.name;
            this.editInverterData.type = inverter.type;
            this.editInverterData.max_power = inverter.max_power;
        },
        onCancel() {
            this.editId = "-1";
            this.editInverterData.serial = 0;
            this.editInverterData.name = "";
            this.editInverterData.max_power = [];
            this.modal.hide();
        },
        onEditSubmit(id: string) {
            const formData = new FormData();
            this.editInverterData.id = id;
            formData.append("data", JSON.stringify(this.editInverterData));

            fetch("/api/inverter/edit", {
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
                )
                .then(() => { this.getInverters() });

            this.editId = "-1";
            this.editInverterData.serial = 0;
            this.editInverterData.name = "";
            this.editInverterData.type = "";
            this.editInverterData.max_power = [];
            this.modal.hide();
        },
    },
});
</script>