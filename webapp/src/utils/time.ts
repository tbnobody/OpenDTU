export function timestampToString(locale: string, timestampSeconds: number, includeDays: true): [number, string];
export function timestampToString(locale: string, timestampSeconds: number, includeDays?: false): [string];
export function timestampToString(
    locale: string,
    timestampSeconds: number,
    includeDays = false
): [number, string] | [string] {
    const timeString = new Date(timestampSeconds * 1000).toLocaleTimeString(locale, {
        timeZone: 'UTC',
        hour12: false,
    });
    if (!includeDays) return [timeString];

    const secondsPerDay = 60 * 60 * 24;
    const days = Math.floor(timestampSeconds / secondsPerDay);
    return [days, timeString];
}
