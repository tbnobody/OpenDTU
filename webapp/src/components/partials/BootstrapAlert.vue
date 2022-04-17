<template>
  <div
    v-if="isAlertVisible"
    ref="element"
    class="alert"
    role="alert"
    :class="classes"
  >
    <slot />
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

<script>
import { computed, defineComponent, onBeforeUnmount, ref, watch } from "vue";
import Alert from "bootstrap/js/dist/alert";

export const toInteger = (value, defaultValue = NaN) => {
  return Number.isInteger(value) ? value : defaultValue;
};

export default defineComponent({
  name: "BAlert",
  props: {
    dismissLabel: { type: String, default: "Close" },
    dismissible: { type: Boolean, default: false },
    fade: { type: Boolean, default: false },
    modelValue: { type: [Boolean, Number], default: false },
    show: { type: Boolean, default: false },
    variant: { type: String, default: "info" },
  },
  emits: ["dismissed", "dismiss-count-down", "update:modelValue"],
  setup(props, { emit }) {
    const element = undefined;
    let instance = undefined;
    const classes = computed(() => ({
      [`alert-${props.variant}`]: props.variant,
      show: props.modelValue,
      "alert-dismissible": props.dismissible,
      fade: props.modelValue,
    }));

    let _countDownTimeout = 0;

    const parseCountDown = (value) => {
      if (typeof value === "boolean") {
        return 0;
      }

      const numberValue = toInteger(value, 0);
      return numberValue > 0 ? numberValue : 0;
    };

    const clearCountDownInterval = () => {
      if (_countDownTimeout === undefined) return;
      clearTimeout(_countDownTimeout);
      _countDownTimeout = undefined;
    };

    const countDown = ref(parseCountDown(props.modelValue));
    const isAlertVisible = computed(() => props.modelValue || props.show);

    onBeforeUnmount(() => {
      clearCountDownInterval();
      instance?.dispose();
      instance = undefined;
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
      countDown.value = parseCountDown(props.modelValue);
      if ((parsedModelValue.value || props.show) && !instance)
        instance = new Alert(element);
    };

    const dismissClicked = () => {
      if (typeof props.modelValue === "boolean") {
        emit("update:modelValue", false);
      } else {
        emit("update:modelValue", 0);
      }
      emit("dismissed");
    };

    watch(() => props.modelValue, handleShowAndModelChanged);
    watch(() => props.show, handleShowAndModelChanged);

    watch(countDown, (newValue) => {
      clearCountDownInterval();
      if (typeof props.modelValue === "boolean") return;
      emit("dismiss-count-down", newValue);
      if (newValue === 0 && props.modelValue > 0) emit("dismissed");
      if (props.modelValue !== newValue) emit("update:modelValue", newValue);
      if (newValue > 0) {
        _countDownTimeout = setTimeout(() => {
          countDown.value--;
        }, 1000);
      }
    });
    return {
      dismissClicked,
      isAlertVisible,
      element,
      classes,
    };
  },
});
</script>