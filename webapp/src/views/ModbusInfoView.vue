<template>
    <BasePage :title="$t('modbusinfo.ModbusInformation')" :isLoading="dataLoading" :show-reload="true" @reload="getModbusInfo">
        <CardElement :text="$t('modbusinfo.TCPSummary')" textVariant="text-bg-primary">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('modbusinfo.Status') }}</th>
                            <td>
                                <StatusBadge :status="modbusDataList.modbus_tcp_enabled" true_text="modbusinfo.Enabled" false_text="modbusinfo.Disabled" />
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('modbusinfo.Server') }}</th>
                            <td>{{ modbusDataList.modbus_hostname }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('modbusinfo.IpAddress') }}</th>
                            <td>{{ modbusDataList.modbus_ip }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('modbusinfo.Port') }}</th>
                            <td>{{ modbusDataList.modbus_port }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('modbusinfo.Clients') }}</th>
                            <td>{{ modbusDataList.modbus_clients }}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </CardElement>
        <CardElement :text="$t('modbusinfo.ServerSummary')" textVariant="text-bg-primary">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('modbusinfo.IDDTUPro') }}</th>
                            <td>{{ modbusDataList.modbus_id_dtupro }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('modbusinfo.IDTotal') }}</th>
                            <td>{{ modbusDataList.modbus_id_total }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('modbusinfo.IDMeter') }}</th>
                            <td>{{ modbusDataList.modbus_id_meter }}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </CardElement>

    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import CardElement from '@/components/CardElement.vue';
import StatusBadge from '@/components/StatusBadge.vue';
import type { ModbusStatus } from '@/types/ModbusStatus';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        CardElement,
        StatusBadge
    },
    data() {
        return {
            dataLoading: true,
            modbusDataList: {} as ModbusStatus,
        };
    },
    created() {
        this.getModbusInfo();
    },
    methods: {
        getModbusInfo() {
            this.dataLoading = true;
            fetch("/api/modbus/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.modbusDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>
