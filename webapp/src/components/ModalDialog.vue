<template>
    <div class="modal" :id="modalId" tabindex="-1">
        <div class="modal-dialog" :class="[small ? '' : 'modal-lg']">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title">{{ title }}</h5>
                    <div class="ms-auto d-flex align-items-center gap-2">
                        <button
                            v-if="showReload"
                            type="button"
                            class="btn d-flex align-items-center"
                            @click="$emit('reload')"
                            v-tooltip
                            :title="$t('base.Reload')"
                        >
                            <BIconArrowClockwise />
                        </button>
                        <button
                            type="button"
                            class="btn-close"
                            data-bs-dismiss="modal"
                            :aria-label="getCloseText"
                            @click="close"
                        ></button>
                    </div>
                </div>
                <div class="modal-body">
                    <div class="text-center" v-if="loading">
                        <div class="spinner-border" role="status">
                            <span class="visually-hidden">{{ $t('home.Loading') }}</span>
                        </div>
                    </div>
                    <slot v-else> </slot>
                </div>
                <div class="modal-footer">
                    <slot name="footer"> </slot>
                    <button type="button" class="btn btn-secondary" @click="close" data-bs-dismiss="modal">
                        {{ getCloseText }}
                    </button>
                </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { BIconArrowClockwise } from 'bootstrap-icons-vue';

export default defineComponent({
    components: {
        BIconArrowClockwise,
    },
    props: {
        modalId: { type: String, required: true },
        title: { type: String, required: true },
        closeText: { type: String, required: false, default: '' },
        showReload: { type: Boolean, required: false, default: false },
        small: Boolean,
        loading: Boolean,
    },
    computed: {
        getCloseText() {
            return this.closeText == '' ? this.$t('base.Close') : this.closeText;
        },
    },
    methods: {
        close() {
            this.$emit('close');
        },
    },
});
</script>
