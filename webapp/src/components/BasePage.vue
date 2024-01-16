<template>
    <div :class="{'container-xxl': !isWideScreen,
    'container-fluid': isWideScreen}" role="main">
        <div class="page-header">
            <div class="row">
                <div class="col-sm-11">
                    <h1>{{ title }}
                        <span v-if="showWebSocket" :class="{
                            'onlineMarker': isWebsocketConnected,
                            'offlineMarker': !isWebsocketConnected,
                        }"></span>
                    </h1>
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
import PullToRefresh from 'pulltorefreshjs';

export default defineComponent({
    components: {
        BIconArrowClockwise,
    },
    props: {
        title: { type: String, required: true },
        isLoading: { type: Boolean, required: false, default: false },
        isWideScreen: { type: Boolean, required: false, default: false },
        isWebsocketConnected: { type: Boolean, required: false, default: false },
        showWebSocket: { type: Boolean, required: false, default: false },
        showReload: { type: Boolean, required: false, default: false },
    },
    mounted() {
        var self = this;
        console.log("init");
        PullToRefresh.init({
            mainElement: 'main', // above which element?
            instructionsPullToRefresh: this.$t('base.Pull'),
            instructionsReleaseToRefresh: this.$t('base.Release'),
            instructionsRefreshing: this.$t('base.Refreshing'),
            onRefresh: function() {
                self.$emit('reload');
            }
        });
    },
    unmounted() {
        console.log("destroy");
        PullToRefresh.destroyAll();
    },
});
</script>

<style>
.ptr--text {
    color: var(--bs-primary-text-emphasis) !important;
}
.ptr--icon {
    color: var(--bs-primary-text-emphasis) !important;
}

.offlineMarker:before {
    content: '';
    position: absolute;
    width: 8px;
    height: 8px;
    background: #ff0000;
    border-color: #ff0000;
    border-radius: 50%;
}
.onlineMarker:before {
    content: '';
    position: absolute;
    width: 8px;
    height: 8px;
    background: #00bb00;
    border-color: #00bb00;
    border-radius: 50%;
}
.onlineMarker:after {
    content: '';
    position: absolute;
    width: 32px;
    height: 32px;
    margin: -12px 0 0 -12px;
    border: 1px solid #00bb00;
    border-radius: 50%;
    box-shadow: 0 0 4px #00bb00, inset 0 0 4px rgb(56, 111, 169);
    transform: scale(0);
    animation: online 2.5s ease-in-out infinite;
}
@keyframes online {
    0% {
        transform: scale(.1);
        opacity: 1;
    }

    70% {
        transform: scale(2.5);
        opacity: 0;
    }

    100% {
        opacity: 0;
    }
}
</style>