<template>
    <div>
        <InputElement
                :label="$t('httprequestsettings.url')"
                v-model="cfg.url"
                type="text"
                maxlength="1024"
                placeholder="http://admin:supersecret@mypowermeter.home/status"
                prefix="GET "
                :tooltip="$t('httprequestsettings.urlDescription')"
                wide />

        <div class="row mb-3">
            <label for="auth_type" class="col-sm-4 col-form-label">{{ $t('httprequestsettings.authorization') }}</label>
            <div class="col-sm-8">
                <select id="auth_type" class="form-select" v-model="cfg.auth_type">
                    <option v-for="a in authTypeList" :key="a.key" :value="a.key">
                        {{ $t('httprequestsettings.authType' + a.value) }}
                    </option>
                </select>
            </div>
        </div>

        <InputElement
                v-if="cfg.auth_type != 0"
                :label="$t('httprequestsettings.username')"
                v-model="cfg.username"
                type="text"
                maxlength="64"
                wide />

        <InputElement
                v-if="cfg.auth_type != 0"
                :label="$t('httprequestsettings.password')"
                v-model="cfg.password"
                type="password"
                maxlength="64"
                wide />

        <InputElement
                :label="$t('httprequestsettings.headerKey')"
                v-model="cfg.header_key"
                type="text"
                maxlength="64"
                :tooltip="$t('httprequestsettings.headerKeyDescription')"
                wide />

        <InputElement
                :label="$t('httprequestsettings.headerValue')"
                v-model="cfg.header_value"
                type="text"
                maxlength="256"
                wide />

        <InputElement
                :label="$t('httprequestsettings.timeout')"
                v-model="cfg.timeout"
                type="number"
                :postfix="$t('httprequestsettings.milliSeconds')"
                wide />
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import InputElement from '@/components/InputElement.vue';

export default defineComponent({
    props: {
        'cfg': { type: Object, required: true }
    },
    components: {
        InputElement
    },
    data() {
        return {
            authTypeList: [
                { key: 0, value: "None" },
                { key: 1, value: "Basic" },
                { key: 2, value: "Digest" },
            ]
        };
    }
});
</script>
