<template>
    <CardElement :text="$t('pininfo.PinOverview')" textVariant="text-bg-primary">
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <thead>
                    <tr>
                        <th>{{ $t('pininfo.Category') }}</th>
                        <th>{{ $t('pininfo.Name') }}</th>
                        <th>{{ $t('pininfo.ValueSelected') }}</th>
                        <th>{{ $t('pininfo.ValueActive') }}</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                        <td rowspan="6">NRF24</td>
                        <td>MISO</td>
                        <td>{{ selectedPinAssignment?.nrf24?.miso }}</td>
                        <td>{{ currentPinAssignment?.nrf24?.miso }}</td>
                    </tr>
                    <tr>
                        <td>MOSI</td>
                        <td>{{ selectedPinAssignment?.nrf24?.mosi }}</td>
                        <td>{{ currentPinAssignment?.nrf24?.mosi }}</td>
                    </tr>
                    <tr>
                        <td>CLK</td>
                        <td>{{ selectedPinAssignment?.nrf24?.clk }}</td>
                        <td>{{ currentPinAssignment?.nrf24?.clk }}</td>
                    </tr>
                    <tr>
                        <td>IRQ</td>
                        <td>{{ selectedPinAssignment?.nrf24?.irq }}</td>
                        <td>{{ currentPinAssignment?.nrf24?.irq }}</td>
                    </tr>
                    <tr>
                        <td>EN</td>
                        <td>{{ selectedPinAssignment?.nrf24?.en }}</td>
                        <td>{{ currentPinAssignment?.nrf24?.en }}</td>
                    </tr>
                    <tr>
                        <td>CS</td>
                        <td>{{ selectedPinAssignment?.nrf24?.cs }}</td>
                        <td>{{ currentPinAssignment?.nrf24?.cs }}</td>
                    </tr>

                    <tr>
                        <td rowspan="7">Ethernet</td>
                        <td>enabled</td>
                        <td>{{ selectedPinAssignment?.eth?.enabled }}</td>
                        <td>{{ currentPinAssignment?.eth?.enabled }}</td>
                    </tr>
                    <tr>
                        <td>phy_addr</td>
                        <td>{{ selectedPinAssignment?.eth?.phy_addr }}</td>
                        <td>{{ currentPinAssignment?.eth?.phy_addr }}</td>
                    </tr>
                    <tr>
                        <td>power</td>
                        <td>{{ selectedPinAssignment?.eth?.power }}</td>
                        <td>{{ currentPinAssignment?.eth?.power }}</td>
                    </tr>
                    <tr>
                        <td>mdc</td>
                        <td>{{ selectedPinAssignment?.eth?.mdc }}</td>
                        <td>{{ currentPinAssignment?.eth?.mdc }}</td>
                    </tr>
                    <tr>
                        <td>mdio</td>
                        <td>{{ selectedPinAssignment?.eth?.mdio }}</td>
                        <td>{{ currentPinAssignment?.eth?.mdio }}</td>
                    </tr>
                    <tr>
                        <td>type</td>
                        <td>{{ selectedPinAssignment?.eth?.type }}</td>
                        <td>{{ currentPinAssignment?.eth?.type }}</td>
                    </tr>
                    <tr>
                        <td>clk_mode</td>
                        <td>{{ selectedPinAssignment?.eth?.clk_mode }}</td>
                        <td>{{ currentPinAssignment?.eth?.clk_mode }}</td>
                    </tr>

                    <tr>
                        <td rowspan="6">Display</td>
                        <td>type</td>
                        <td>{{ selectedPinAssignment?.display?.type }}</td>
                        <td>{{ currentPinAssignment?.display?.type }}</td>
                    </tr>
                    <tr>
                        <td>data</td>
                        <td>{{ selectedPinAssignment?.display?.data }}</td>
                        <td>{{ currentPinAssignment?.display?.data }}</td>
                    </tr>
                    <tr>
                        <td>clk</td>
                        <td>{{ selectedPinAssignment?.display?.clk }}</td>
                        <td>{{ currentPinAssignment?.display?.clk }}</td>
                    </tr>
                    <tr>
                        <td>cs</td>
                        <td>{{ selectedPinAssignment?.display?.cs }}</td>
                        <td>{{ currentPinAssignment?.display?.cs }}</td>
                    </tr>
                    <tr>
                        <td>reset</td>
                        <td>{{ selectedPinAssignment?.display?.reset }}</td>
                        <td>{{ currentPinAssignment?.display?.reset }}</td>
                    </tr>

                </tbody>
            </table>
        </div>
    </CardElement>
</template>

<script lang="ts">
import CardElement from '@/components/CardElement.vue';
import type { Device } from '@/types/PinMapping';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
    props: {
        selectedPinAssignment: { type: Object as PropType<Device | undefined>, required: true },
        currentPinAssignment: { type: Object as PropType<Device | undefined>, required: true },
    },
});
</script>
