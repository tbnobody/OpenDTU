<template>
    <div class="row mb-3">
        <label
            :for="inputId"
            :class="[ wide ? 'col-sm-4' : 'col-sm-2', isCheckbox ? 'form-check-label' : 'col-form-label' ]"
        >
            {{ label }}
            <BIconInfoCircle v-if="tooltip !== undefined" v-tooltip :title="tooltip" />
        </label>
        <div :class="[ wide ? 'col-sm-8' : 'col-sm-10' ]">
            <div v-if="!isTextarea"
                 :class="{'form-check form-switch': isCheckbox,
                          'input-group': postfix || prefix }"
            >
                 <span v-if="prefix"
                       class="input-group-text"
                       :id="descriptionId"
                 >
                    {{ prefix }}
                </span>
                <input
                    v-model="model"
                    :class="[ isCheckbox ? 'form-check-input' : 'form-control' ]"
                    :id="inputId"
                    :placeholder="placeholder"
                    :type="type"
                    :maxlength="maxlength"
                    :min="min"
                    :max="max"
                    :disabled="disabled"
                    :aria-describedby="descriptionId"
                />
                <span v-if="postfix"
                      class="input-group-text"
                      :id="descriptionId"
                >
                    {{ postfix }}
                </span>
                <slot/>
            </div>
            <div v-else>
                <textarea
                    v-model="model"
                    class="form-control"
                    :id="inputId"
                    :maxlength="maxlength"
                    :rows="rows"
                    :disabled="disabled"
                    :placeholder="placeholder"
                />
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { BIconInfoCircle } from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BIconInfoCircle,
    },
    props: {
        'modelValue': [String, Number, Boolean, Date],
        'label': String,
        'placeholder': String,
        'type': String,
        'maxlength': String,
        'min': String,
        'max': String,
        'rows': String,
        'disabled': Boolean,
        'postfix': String,
        'prefix': String,
        'wide': Boolean,
        'tooltip': String,
    },
    data() {
        return {};
    },
    computed: {
        model: {
            get(): any {
                if (this.type === 'checkbox') return !!this.modelValue;
                return this.modelValue;
            },
            set(value: any) {
                this.$emit('update:modelValue', value);
            },
        },
        uniqueLabel() {
            // normally, the label is sufficient to build a unique id
            // if two inputs with the same label text on one page is required,
            // use a unique placeholder even if it is a checkbox
            return this.label?.replace(/[^A-Za-z0-9]/g, '') +
                (this.placeholder ? this.placeholder.replace(/[^A-Za-z0-9]/g, '') : '');
        },
        inputId() {
            return 'input' + this.uniqueLabel
        },
        descriptionId() {
            return 'desc' + this.uniqueLabel;
        },
        isTextarea() {
            return this.type === 'textarea';
        },
        isCheckbox() {
            return this.type === 'checkbox';
        }
    },
});
</script>