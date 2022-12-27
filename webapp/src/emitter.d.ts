declare module '@vue/runtime-core' {
    interface ComponentCustomProperties {
        $emitter: Emitter;
    }
}

export { }  // Important! See note.