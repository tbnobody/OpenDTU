export const timestampToString = (locale: string, timestampSeconds: number, includeDays = false): string[] => {
    const timeString = new Date(timestampSeconds * 1000).toLocaleTimeString(locale, { timeZone: "UTC", hour12: false });
    if (!includeDays) return [timeString];

    const secondsPerDay = 60 * 60 * 24;
    const days = Math.floor(timestampSeconds / secondsPerDay).toFixed(0);
    return [days, timeString];
}