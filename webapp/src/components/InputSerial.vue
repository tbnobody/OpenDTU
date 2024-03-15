<template>
    <input v-model="inputSerial" type="text" :id="id" :required="required" class="form-control" :class="inputClass" />
    <BootstrapAlert show :variant="formatShow" v-if="formatHint">{{ formatHint }}</BootstrapAlert>
</template>

<script lang="ts">
import BootstrapAlert from './BootstrapAlert.vue';
import { defineComponent } from 'vue';

const chars32 = '0123456789ABCDEFGHJKLMNPRSTUVWXY';

export default defineComponent({
    components: {
        BootstrapAlert,
    },
    props: {
        'modelValue': { type: [String, Number], required: true },
        'id': String,
        'inputClass': String,
        'required': Boolean,
    },
    data() {
        return {
            inputSerial: "",
            formatHint: "",
            formatShow: "info",
        };
    },
    computed: {
        model: {
            get(): any {
                return this.modelValue;
            },
            set(value: any) {
                this.$emit('update:modelValue', value);
            },
        },
    },
    watch: {
        modelValue: function (val) {
            this.inputSerial = val;
        },
        inputSerial: function (val) {
            const serial = val.toString().toUpperCase(); // Convert to lowercase for case-insensitivity

            if (serial == "") {
                this.formatHint = "";
                this.model = "";
                return;
            }

            this.formatShow = "info";

            // Contains only numbers
            if (/^1{1}[\dA-F]{11}$/.test(serial)) {
                this.model = serial;
                this.formatHint = this.$t('inputserial.format_hoymiles');
            }

            // Contains numbers and hex characters but at least one number
            else if (/^(?=.*\d)[\dA-F]{12}$/.test(serial)) {
                this.model = serial;
                this.formatHint = this.$t('inputserial.format_converted');
            }

            // Has format: xxxxxxxxx-xxx
            else if (/^((A01)|(A11)|(A21))[\dA-HJ-NR-YP]{6}-[\dA-HJ-NP-Z]{3}$/.test(serial)) {
                if (this.checkHerfChecksum(serial)) {
                    this.model = this.convertHerfToHoy(serial);
                    this.$nextTick(() => {
                        this.formatHint = this.$t('inputserial.format_herf_valid', { serial: this.model });
                    });

                } else {
                    this.formatHint = this.$t('inputserial.format_herf_invalid');
                    this.formatShow = "danger";
                }

            // Any other format
            } else {
                this.formatHint = this.$t('inputserial.format_unknown');
                this.formatShow = "danger";
            }
        }
    },
    methods: {
        checkHerfChecksum(sn: string) {
            const chars64 = 'HMFLGW5XC301234567899Z67YRT2S8ABCDEFGHJKDVEJ4KQPUALMNPRSTUVWXYNB';

            const checksum = sn.substring(sn.indexOf("-") + 1);
            const serial = sn.substring(0, sn.indexOf("-"));

            const first_char = '1';
            const i = chars32.indexOf(first_char)
            const sum1: number = Array.from(serial).reduce((sum, c) => sum + c.charCodeAt(0), 0) & 31;
            const sum2: number = Array.from(serial).reduce((sum, c) => sum + chars32.indexOf(c), 0) & 31;
            const ext = first_char + chars64[sum1 + i] + chars64[sum2 + i];

            return checksum == ext;
        },
        convertHerfToHoy(sn: string) {
            let sn_int: bigint = 0n;

            for (let i = 0; i < 9; i++) {
                const pos: bigint = BigInt(chars32.indexOf(sn[i].toUpperCase()));
                const shift: bigint = BigInt(42 - 5 * i - (i <= 2 ? 0 : 2));
                sn_int |= (pos << shift);
            }

            return sn_int.toString(16);
        }
    },
});
</script>
