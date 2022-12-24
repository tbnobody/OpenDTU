<template>
    <BasePage :title="$t('about.AboutOpendtu')">
        <div class="accordion" id="accordionExample">
            <div class="accordion-item">
                <h2 class="accordion-header" id="headingOne">
                    <button class="accordion-button" type="button" data-bs-toggle="collapse"
                        data-bs-target="#collapseOne" aria-expanded="true" aria-controls="collapseOne">
                        <span class="badge text-bg-secondary">
                            <BIconInfoCircle class="fs-4" />
                        </span>&nbsp;{{ $t('about.ProjectOrigin') }}
                    </button>
                </h2>
                <div id="collapseOne" class="accordion-collapse collapse show" aria-labelledby="headingOne"
                    data-bs-parent="#accordionExample">
                    <div class="accordion-body">
                        <p class="fw-normal" v-html="$t('about.ProjectOriginBody1')"></p>
                        <p class="fw-normal" v-html="$t('about.ProjectOriginBody2')"></p>
                        <p class="fw-normal" v-html="$t('about.ProjectOriginBody3')"></p>
                        <p class="fw-normal" v-html="$t('about.ProjectOriginBody4')"></p>
                    </div>
                </div>
            </div>
            <div class="accordion-item">
                <h2 class="accordion-header" id="headingTwo">
                    <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse"
                        data-bs-target="#collapseTwo" aria-expanded="false" aria-controls="collapseTwo">
                        <span class="badge text-bg-secondary">
                            <BIconActivity class="fs-4" />
                        </span>&nbsp;{{ $t('about.NewsUpdates') }}
                    </button>
                </h2>
                <div id="collapseTwo" class="accordion-collapse collapse" aria-labelledby="headingTwo"
                    data-bs-parent="#accordionExample">
                    <div class="accordion-body" v-html="$t('about.NewsUpdatesBody')"></div>
                </div>
            </div>
            <div class="accordion-item">
                <h2 class="accordion-header" id="headingThree">
                    <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse"
                        data-bs-target="#collapseThree" aria-expanded="false" aria-controls="collapseThree">
                        <span class="badge text-bg-secondary">
                            <BIconBug class="fs-4" />
                        </span>&nbsp;{{ $t('about.ErrorReporting') }}
                    </button>
                </h2>
                <div id="collapseThree" class="accordion-collapse collapse" aria-labelledby="headingThree"
                    data-bs-parent="#accordionExample">
                    <div class="accordion-body" v-html="$t('about.ErrorReportingBody')"></div>
                </div>
            </div>
            <div class="accordion-item">
                <h2 class="accordion-header" id="headingFour">
                    <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse"
                        data-bs-target="#collapseFour" aria-expanded="false" aria-controls="collapseFour">
                        <span class="badge text-bg-secondary">
                            <BIconChat class="fs-4" />
                        </span>&nbsp;{{ $t('about.Discussion') }}
                    </button>
                </h2>
                <div id="collapseFour" class="accordion-collapse collapse" aria-labelledby="headingFour"
                    data-bs-parent="#accordionExample">
                    <div class="accordion-body" v-html="$t('about.DiscussionBody')"></div>
                </div>
            </div>

        </div>

    </BasePage>

</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import {
    BIconActivity,
    BIconBug,
    BIconChat,
    BIconInfoCircle
} from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BIconActivity,
        BIconBug,
        BIconChat,
        BIconInfoCircle,
    },
});

</script>