<template>
    <div
        v-if="isAlertVisible"
        ref="element"
        class="alert"
        role="alert"
        :class="[classes, { 'alert-with-progress': autoDismiss }]"
    >
        <slot />
        <div v-if="autoDismiss" class="alert-progress-bar" :style="progressBarStyle"></div>
        <button
            v-if="dismissible"
            type="button"
            class="btn-close"
            data-bs-dismiss="alert"
            :aria-label="dismissLabel"
            @click="dismissClicked"
        />
    </div>
</template>

<script lang="ts">
import Alert from 'bootstrap/js/dist/alert';
import { computed, defineComponent, onBeforeUnmount, ref, watch } from 'vue';

export const toInteger = (value: number, defaultValue = NaN) => {
    return Number.isInteger(value) ? value : defaultValue;
};

export default defineComponent({
    name: 'BootstrapAlert',
    props: {
        autoDismiss: { type: Number, default: 0 },
        dismissLabel: { type: String, default: 'Close' },
        dismissible: { type: Boolean, default: false },
        fade: { type: Boolean, default: false },
        jumpToTop: { type: Boolean, default: true },
        modelValue: { type: [Boolean, Number], default: false },
        show: { type: Boolean, default: false },
        variant: { type: String, default: 'info' },
    },
    emits: ['dismissed', 'update:modelValue'],
    setup(props, { emit }) {
        const element = ref<HTMLElement>();
        const instance = ref<Alert>();
        const classes = computed(() => ({
            [`alert-${props.variant}`]: props.variant,
            show: props.modelValue,
            'alert-dismissible': props.dismissible,
            fade: props.modelValue,
        }));

        const isAlertVisible = computed(() => props.modelValue || props.show);

        onBeforeUnmount(() => {
            instance.value?.dispose();
            instance.value = undefined;
        });

        const parsedModelValue = computed(() => {
            if (props.modelValue === true) {
                return true;
            }
            if (props.modelValue === false) return false;

            if (toInteger(props.modelValue, 0) < 1) {
                // Boolean will always return false for the above comparison
                return false;
            }
            return !!props.modelValue;
        });

        const handleShowAndModelChanged = () => {
            if ((parsedModelValue.value || props.show) && !instance.value)
                instance.value = new Alert(element.value as HTMLElement);
        };

        const dismissClicked = () => {
            if (typeof props.modelValue === 'boolean') {
                emit('update:modelValue', false);
            } else {
                emit('update:modelValue', 0);
            }
            emit('dismissed');
        };

        watch(() => props.modelValue, handleShowAndModelChanged);
        watch(() => props.show, handleShowAndModelChanged);
        watch(isAlertVisible, (visible) => {
            if (visible && props.jumpToTop) window.scrollTo({ top: 0, behavior: 'smooth' });
        });

        const progressBarStyle = computed(() => {
            if (!props.autoDismiss) return {};
            return {
                animation: `slide-progress ${props.autoDismiss}ms linear`,
            };
        });

        // Auto-Dismiss Logic
        watch(
            () => props.modelValue,
            (newValue) => {
                if (newValue && props.autoDismiss > 0) {
                    setTimeout(() => {
                        dismissClicked();
                    }, props.autoDismiss);
                }
            }
        );
        return {
            dismissClicked,
            isAlertVisible,
            element,
            classes,
            progressBarStyle,
        };
    },
});
</script>

<style scoped>
.alert-with-progress {
    position: relative;
    overflow: hidden;
}

.alert-progress-bar {
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    height: 2px;
    background: rgba(255, 255, 255, 0.5);
    transform-origin: left;
}
</style>

<style>
@keyframes slide-progress {
    from {
        transform: scaleX(1);
    }
    to {
        transform: scaleX(0);
    }
}
</style>
