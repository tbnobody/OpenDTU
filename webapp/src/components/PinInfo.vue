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
                    <template v-for="(category) in categories" :key="category">
                        <tr v-for="(prop, prop_idx) in properties(category)" :key="prop">
                            <td v-if="prop_idx == 0" :rowspan="properties(category).length">
                                {{ capitalizeFirstLetter(category) }}</td>
                            <td :class="{ 'table-danger': !isEqual(category, prop) }">{{ prop }}</td>
                            <td>
                                <template v-if="selectedPinAssignment && category in selectedPinAssignment">
                                    {{ (selectedPinAssignment as any)[category][prop] }}</template>
                            </td>
                            <td>
                                <template v-if="currentPinAssignment && category in currentPinAssignment">
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
            if (this.currentPinAssignment && category in this.currentPinAssignment) {
                curArray = Object.keys((this.currentPinAssignment as Device)[category as keyof Device]);
            }

            let selArray: Array<string> = [];
            if (this.selectedPinAssignment && category in this.selectedPinAssignment) {
                selArray = Object.keys((this.selectedPinAssignment as Device)[category as keyof Device]);
            }

            let total: Array<string> = [];
            total = total.concat(curArray, selArray);

            return Array.from(new Set(total)).sort();
        },
        isEqual(category: string, prop: string): boolean {
            let comSel = 999999;
            let comCur = 999999;

            if (this.selectedPinAssignment && category in this.selectedPinAssignment) {
                comSel = (this.selectedPinAssignment as any)[category][prop];
            }
            if (this.currentPinAssignment && category in this.currentPinAssignment) {
                comCur = (this.currentPinAssignment as any)[category][prop];
            }

            if (comSel == -1 || comSel == 255 || comSel == undefined) {
                comSel = 999999;
            }

            if (comCur == -1 || comCur == 255 || comCur == undefined) {
                comCur = 999999;
            }

            return comSel == comCur;
        },
        capitalizeFirstLetter(value: string): string {
            return value.charAt(0).toUpperCase() + value.slice(1);
        },
    }
});
</script>
