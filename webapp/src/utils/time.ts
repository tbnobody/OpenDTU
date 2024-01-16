export const timestampToString = (timestampSeconds: number, includeDays = false): string => {
    const timeString = new Date(timestampSeconds * 1000).toLocaleTimeString([], { timeZone: "UTC" });
    if (!includeDays) return timeString;

    const secondsPerDay = 60 * 60 * 24;
    const days = Math.floor(timestampSeconds / secondsPerDay);
    return new Intl.RelativeTimeFormat().format(-days, "day") + " " + timeString;
}