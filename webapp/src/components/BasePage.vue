<template>
    <div :class="{'container-xxl': !isWideScreen,
    'container-fluid': isWideScreen}" role="main">
        <div class="page-header">
            <div class="row">
                <div class="col-sm-11">
                    <h1>{{ title }}</h1>
                </div>
                <div class="col-sm-1" v-if="showReload">
                    <button type="button" class="float-end btn btn-outline-primary"
                        @click="$emit('reload')" v-tooltip :title="$t('base.Reload')" ><BIconArrowClockwise /></button>
                </div>
            </div>
        </div>

        <div class="text-center" v-if="isLoading">
            <div class="spinner-border" role="status">
                <span class="visually-hidden">{{ $t('base.Loading') }}</span>
            </div>
        </div>

        <template v-if="!isLoading">
            <slot />
        </template>
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
        title: { type: String, required: true },
        isLoading: { type: Boolean, required: false, default: false },
        isWideScreen: { type: Boolean, required: false, default: false },
        showReload: { type: Boolean, required: false, default: false },
    },
});
</script>