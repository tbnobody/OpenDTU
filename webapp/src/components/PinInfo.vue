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
                    <template v-for="(category) in categories">
                        <tr v-for="(prop, prop_idx) in properties(category)">
                            <td v-if="prop_idx == 0" :rowspan="properties(category).length">
                                {{ capitalizeFirstLetter(category) }}</td>
                            <td :class="{ 'table-danger': !isEqual(category, prop) }">{{ prop }}</td>
                            <td>
                                <template v-if="((selectedPinAssignment as Device)[category as keyof Device])">
                                    {{ (selectedPinAssignment as any)[category][prop] }}</template>
                            </td>
                            <td>
                                <template v-if="((currentPinAssignment as Device)[category as keyof Device])">
                                    {{ (currentPinAssignment as any)[category][prop] }}</template>
                            </td>
                        </tr>
                    </template>
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
    computed: {
        categories(): string[] {
            let curArray: Array<string> = [];
            if (this.currentPinAssignment) {
                curArray = Object.keys(this.currentPinAssignment as Device);
            }

            let selArray: Array<string> = [];
            if (this.selectedPinAssignment) {
                selArray = Object.keys(this.selectedPinAssignment as Device);
            }

            let total: Array<string> = [];
            total = total.concat(curArray, selArray);
            return Array.from(new Set(total)).filter(cat => cat != 'name').sort();
        },
    },
    methods: {
        properties(category: string): string[] {
            let curArray: Array<string> = [];
            if ((this.currentPinAssignment as Device)[category as keyof Device]) {
                curArray = Object.keys((this.currentPinAssignment as Device)[category as keyof Device]);
            }

            let selArray: Array<string> = [];
            if ((this.selectedPinAssignment as Device)[category as keyof Device]) {
                selArray = Object.keys((this.selectedPinAssignment as Device)[category as keyof Device]);
            }

            let total: Array<string> = [];
            total = total.concat(curArray, selArray);

            return Array.from(new Set(total)).sort();
        },
        isEqual(category: string, prop: string): boolean {
            if (!((this.selectedPinAssignment as Device)[category as keyof Device])) {
                return false;
            }
            if (!((this.currentPinAssignment as Device)[category as keyof Device])) {
                return false;
            }

            return (this.selectedPinAssignment as any)[category][prop] == (this.currentPinAssignment as any)[category][prop];
        },
        capitalizeFirstLetter(value: string): string {
            return value.charAt(0).toUpperCase() + value.slice(1);
        },
    }
});
</script>
