export type Schema = {
    [key: string]: 'string' | 'number' | 'boolean' | 'object' | 'array' | Schema;
};

/* eslint-disable  @typescript-eslint/no-explicit-any */
export function hasStructure(obj: any, schema: Schema): boolean {
    if (typeof obj !== 'object' || obj === null) return false;

    for (const key in schema) {
        const expectedType = schema[key];

        if (['string', 'number', 'boolean'].includes(expectedType as string)) {
            if (typeof obj[key] !== expectedType) return false;
        } else if (expectedType === 'object') {
            if (typeof obj[key] !== 'object' || obj[key] === null) return false;
        } else if (expectedType === 'array') {
            if (!Array.isArray(obj[key])) return false;
        } else if (typeof expectedType === 'object') {
            // Recursively check nested objects
            if (!hasStructure(obj[key], expectedType as Schema)) return false;
        }
    }

    return true;
}
