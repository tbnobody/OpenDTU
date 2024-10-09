/* eslint-env node */
import js from "@eslint/js";
import pluginVue from 'eslint-plugin-vue'
import vueTsEslintConfig from '@vue/eslint-config-typescript'

export default [
    js.configs.recommended,
    ...pluginVue.configs['flat/essential'],
    ...vueTsEslintConfig(),
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
        ecmaVersion: 2022
      },
    }
  ]
