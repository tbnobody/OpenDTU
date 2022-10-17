<template>
    <BootstrapAlert :show="!devInfoList.valid_data">
        <h4 class="alert-heading">
            <BIconInfoSquare class="fs-2" />&nbsp;No Information available
        </h4>Did not receive any valid data from the inverter till now. Still trying...
    </BootstrapAlert>
    <table v-if="devInfoList.valid_data" class="table table-hover">
        <tbody>
            <tr>
                <td>Model</td>
                <td v-if="devInfoList.hw_model_name != ''">{{ devInfoList.hw_model_name }}</td>
                <td v-else>Unknown model! Please report the "Hardware Part Number" and model (e.g. HM-350) as an issue
                    <a href="https://github.com/tbnobody/OpenDTU/issues" target="_blank">here</a>.
                </td>
            </tr>
            <tr>
                <td>Bootloader Version</td>
                <td>{{ formatVersion(devInfoList.fw_bootloader_version) }}</td>
            </tr>
            <tr>
                <td>Firmware Version</td>
                <td>{{ formatVersion(devInfoList.fw_build_version) }}</td>
            </tr>
            <tr>
                <td>Firmware Build Date</td>
                <td>{{ devInfoList.fw_build_datetime }}</td>
            </tr>
            <tr>
                <td>Hardware Part Number</td>
                <td>{{ devInfoList.hw_part_number }}</td>
            </tr>
            <tr>
                <td>Hardware Version</td>
                <td>{{ devInfoList.hw_version }}</td>
            </tr>
        </tbody>
    </table>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { BIconInfoSquare } from 'bootstrap-icons-vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';

declare interface DevInfoData {
    valid_data: boolean,
    fw_bootloader_version: number,
    fw_build_version: number,
    fw_build_datetime: Date,
    hw_part_number: number,
    hw_version: number,
    hw_model_name: string,
}

export default defineComponent({
    components: {
        BIconInfoSquare,
        BootstrapAlert,
    },
    props: {
        devInfoList: { type: Object as () => DevInfoData, required: true },
    },
    computed: {
        formatVersion() {
            return (value: number) => {
                const version_major = Math.floor(value / 10000);
                const version_minor = Math.floor((value - version_major * 10000) / 100);
                const version_patch = Math.floor((value - version_major * 10000 - version_minor * 100));
                return version_major + "." + version_minor + "." + version_patch;
            };
        }
    }
});
</script>