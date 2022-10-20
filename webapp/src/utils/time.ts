export const timestampToString = (value: number, includeDays = false): string => {
    const days = Math.floor(value / (24 * 60 * 60));
    const secAfterDays = value - days * (24 * 60 * 60);
    const hours = Math.floor(secAfterDays / (60 * 60));
    const secAfterHours = secAfterDays - hours * (60 * 60);
    const minutes = Math.floor(secAfterHours / 60);
    const seconds = secAfterHours - minutes * 60;

    const dHours = hours > 9 ? hours : "0" + hours;
    const dMins = minutes > 9 ? minutes : "0" + minutes;
    const dSecs = seconds > 9 ? seconds : "0" + seconds;

    if (includeDays) {
        return days + " days " + dHours + ":" + dMins + ":" + dSecs;
    }
    return dHours + ":" + dMins + ":" + dSecs;
}