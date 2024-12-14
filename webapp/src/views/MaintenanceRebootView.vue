<template>
    <BasePage :title="$t('maintenancereboot.DeviceReboot')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <CardElement :text="$t('maintenancereboot.PerformReboot')" textVariant="text-bg-primary" center-content>
            <button class="btn btn-danger" @click="onOpenModal(performReboot)">
                {{ $t('maintenancereboot.Reboot') }}
            </button>

            <div class="alert alert-danger mt-3" role="alert" v-html="$t('maintenancereboot.RebootHint')"></div>
        </CardElement>
    </BasePage>

    <ModalDialog
        modalId="performReboot"
        small
        :title="$t('maintenancereboot.RebootOpenDTU')"
        :closeText="$t('maintenancereboot.Cancel')"
    >
        {{ $t('maintenancereboot.RebootQuestion') }}
        <template #footer>
            <button type="button" class="btn btn-danger" @click="onReboot">
                {{ $t('maintenancereboot.Reboot') }}
            </button>
        </template>
    </ModalDialog>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import ModalDialog from '@/components/ModalDialog.vue';
import { authHeader, handleResponse, isLoggedIn } from '@/utils/authentication';
import * as bootstrap from 'bootstrap';
import { defineComponent } from 'vue';
import { waitRestart } from '@/utils/waitRestart';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        ModalDialog,
    },
    data() {
        return {
            performReboot: {} as bootstrap.Modal,

            dataLoading: false,

            alertMessage: '',
            alertType: 'info',
            showAlert: false,
        };
    },
    mounted() {
        if (!isLoggedIn()) {
            this.$router.push({
                path: '/login',
                query: { returnUrl: this.$router.currentRoute.value.fullPath },
            });
        }

        this.performReboot = new bootstrap.Modal('#performReboot');
    },
    methods: {
        onReboot() {
            const formData = new FormData();
            formData.append('data', JSON.stringify({ reboot: true }));

            fetch('/api/maintenance/reboot', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.alertMessage = this.$t('apiresponse.' + data.code, data.param);
                    this.alertType = data.type;
                    this.showAlert = true;
                    waitRestart(this.$router);
                });
            this.onCloseModal(this.performReboot);
        },
        onOpenModal(modal: bootstrap.Modal) {
            modal.show();
        },
        onCloseModal(modal: bootstrap.Modal) {
            modal.hide();
        },
    },
});
</script>
