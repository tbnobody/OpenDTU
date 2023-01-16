<template>
    <CardElement :text="$t('pininfo.PinOverview')" textVariant="text-bg-primary">
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <thead>
                    <tr>
                        <th>{{ $t('pininfo.Category') }}</th>
                        <th>{{ $t('pininfo.Name') }}</th>
                        <th>{{ $t('pininfo.Number') }}</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                        <td rowspan="6">NRF24</td>
                        <td>MISO</td>
                        <td>{{ pinAssignment?.nrf24?.miso }}</td>
                    </tr>
                    <tr>
                        <td>MOSI</td>
                        <td>{{ pinAssignment?.nrf24?.mosi }}</td>
                    </tr>
                    <tr>
                        <td>CLK</td>
                        <td>{{ pinAssignment?.nrf24?.clk }}</td>
                    </tr>
                    <tr>
                        <td>IRQ</td>
                        <td>{{ pinAssignment?.nrf24?.irq }}</td>
                    </tr>
                    <tr>
                        <td>EN</td>
                        <td>{{ pinAssignment?.nrf24?.en }}</td>
                    </tr>
                    <tr>
                        <td>CS</td>
                        <td>{{ pinAssignment?.nrf24?.cs }}</td>
                    </tr>

                    <tr>
                        <td rowspan="7">Ethernet</td>
                        <td>enabled</td>
                        <td>{{ pinAssignment?.eth?.enabled }}</td>
                    </tr>
                    <tr>
                        <td>phy_addr</td>
                        <td>{{ pinAssignment?.eth?.phy_addr }}</td>
                    </tr>
                    <tr>
                        <td>power</td>
                        <td>{{ pinAssignment?.eth?.power }}</td>
                    </tr>
                    <tr>
                        <td>mdc</td>
                        <td>{{ pinAssignment?.eth?.mdc }}</td>
                    </tr>
                    <tr>
                        <td>mdio</td>
                        <td>{{ pinAssignment?.eth?.mdio }}</td>
                    </tr>
                    <tr>
                        <td>type</td>
                        <td>{{ pinAssignment?.eth?.type }}</td>
                    </tr>
                    <tr>
                        <td>clk_mode</td>
                        <td>{{ pinAssignment?.eth?.clk_mode }}</td>
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
        pinAssignment: { type: Object as PropType<Device | undefined>, required: true },
    },
});
</script>
