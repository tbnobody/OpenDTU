import { Tooltip } from 'bootstrap'

export const tooltip = {
    mounted(el: HTMLElement) {
        const tooltip = new Tooltip(el)
    }
}