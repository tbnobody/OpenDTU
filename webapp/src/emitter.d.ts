declare module 'vue' {
    interface ComponentCustomProperties {
        $emitter: Emitter;
    }
}

export {}; // Important! See note.
