/* eslint-env node */
import path from "node:path";
import { fileURLToPath } from "node:url";

import { FlatCompat } from "@eslint/eslintrc";
import js from "@eslint/js";
import pluginVue from 'eslint-plugin-vue'

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const compat = new FlatCompat({
  baseDirectory: __dirname,
  recommendedConfig: js.configs.recommended,
});

export default [
    js.configs.recommended,
    ...pluginVue.configs['flat/essential'],
    ...compat.extends("@vue/eslint-config-typescript/recommended"),
    {
      files: [
        "**/*.vue",
        "**/*.js",
        "**/*.jsx",
        "**/*.cjs",
        "**/*.mjs",
        "**/*.ts",
        "**/*.tsx",
        "**/*.cts",
        "**/*.mts",
      ],
      languageOptions: {
        ecmaVersion: 'latest'
      },
    }
  ]
