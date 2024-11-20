<template>
    <div class="row flex-row flex-wrap g-3">
        <div class="col" v-for="category in categories" :key="category">
            <CardElement :text="capitalizeFirstLetter(category)" textVariant="text-bg-primary" table>
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>{{ $t('pininfo.Name') }}</th>
                                <th class="text-center">{{ $t('pininfo.ValueSelected') }}</th>
                                <th class="text-center">{{ $t('pininfo.ValueActive') }}</th>
                            </tr>
                            <tr v-for="(prop, prop_idx) in properties(category)" :key="prop_idx">
                                <td :class="{ 'table-danger': !isEqual(category, prop) }">
                                    {{ prop }}
                                </td>
                                <td class="text-center">
                                    <template v-if="selectedPinAssignment && category in selectedPinAssignment">
                                        {{ (selectedPinAssignment as any)[category][prop] }}</template
                                    >
                                </td>
                                <td class="text-center">
                                    <template v-if="currentPinAssignment && category in currentPinAssignment">
                                        {{ (currentPinAssignment as any)[category][prop] }}</template
                                    >
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </CardElement>
        </div>
    </div>
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
                selArray = selArray.filter((item) => curArray.includes(item));
            }

            let total: Array<string> = [];
            total = total.concat(curArray, selArray);
            return Array.from(new Set(total))
                .filter((cat) => cat != 'name' && cat != 'links')
                .sort();
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
                // eslint-disable-next-line @typescript-eslint/no-explicit-any
                comSel = (this.selectedPinAssignment as any)[category][prop];
            }
            if (this.currentPinAssignment && category in this.currentPinAssignment) {
                // eslint-disable-next-line @typescript-eslint/no-explicit-any
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
    },
});
</script>
