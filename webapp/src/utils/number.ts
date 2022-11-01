export const formatNumber = (num: number, digits: number): string => {
    return new Intl.NumberFormat(
        undefined, { minimumFractionDigits: digits, maximumFractionDigits: digits }
    ).format(num);
}