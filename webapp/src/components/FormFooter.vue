<template>
    <hr class="border border-3 opacity-75" />
    <div class="d-grid gap-2 d-md-flex justify-content-md-end">
        <button type="button" class="btn btn-secondary" @click="$emit('reload')">
            {{ $t('base.Cancel') }}
        </button>
        <button type="submit" class="btn btn-primary">{{ $t('base.Save') }}</button>
    </div>
</template>

<script lang="ts">
export default {
    emits: ['reload'],
};
</script>
