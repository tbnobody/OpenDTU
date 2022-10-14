<template>
    <div class="container-xxl" role="main">
        <div class="page-header">
            <h1>About OpenDTU</h1>
        </div>


        <div class="accordion" id="accordionExample">
            <div class="accordion-item">
                <h2 class="accordion-header" id="headingOne">
                    <button class="accordion-button" type="button" data-bs-toggle="collapse"
                        data-bs-target="#collapseOne" aria-expanded="true" aria-controls="collapseOne">
                        <span class="badge bg-secondary">
                            <BIconInfoCircle class="fs-4" />
                        </span>&nbsp;Project Origin
                    </button>
                </h2>
                <div id="collapseOne" class="accordion-collapse collapse show" aria-labelledby="headingOne"
                    data-bs-parent="#accordionExample">
                    <div class="accordion-body">
                        <p class="fw-normal">
                            This project was started from
                            <a href="https://www.mikrocontroller.net/topic/525778" target="_blank">this discussion.
                                (Mikrocontroller.net)</a>
                        </p>
                        <p class="fw-normal">
                            The Hoymiles protocol was decrypted through the voluntary efforts of many participants.
                            OpenDTU,
                            among others, was developed based on this work. The project is licensed under an Open Source
                            License (<a href="https://www.gnu.de/documents/gpl-2.0.de.html" target="_blank">GNU General
                                Public License version 2</a>).
                        </p>
                        <p class="fw-normal">
                            The software was developed to the best of our knowledge and belief. Nevertheless, no
                            liability
                            can be accepted for a malfunction or guarantee loss of the inverter.
                        </p>
                        <p class="fw-normal">
                            OpenDTU is freely available. If you paid money for the software, you probably got ripped
                            off.
                        </p>
                    </div>
                </div>
            </div>
            <div class="accordion-item">
                <h2 class="accordion-header" id="headingTwo">
                    <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse"
                        data-bs-target="#collapseTwo" aria-expanded="false" aria-controls="collapseTwo">
                        <span class="badge bg-secondary">
                            <BIconActivity class="fs-4" />
                        </span>&nbsp;News &amp; Updates
                    </button>
                </h2>
                <div id="collapseTwo" class="accordion-collapse collapse" aria-labelledby="headingTwo"
                    data-bs-parent="#accordionExample">
                    <div class="accordion-body">
                        New updates can be found on Github: <a href="https://github.com/tbnobody/OpenDTU"
                            target="_blank">https://github.com/tbnobody/OpenDTU</a>
                    </div>
                </div>
            </div>
            <div class="accordion-item">
                <h2 class="accordion-header" id="headingThree">
                    <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse"
                        data-bs-target="#collapseThree" aria-expanded="false" aria-controls="collapseThree">
                        <span class="badge bg-secondary">
                            <BIconBug class="fs-4" />
                        </span>&nbsp;Error Reporting
                    </button>
                </h2>
                <div id="collapseThree" class="accordion-collapse collapse" aria-labelledby="headingThree"
                    data-bs-parent="#accordionExample">
                    <div class="accordion-body">
                        Please report issues using the feature provided by <a
                            href="https://github.com/tbnobody/OpenDTU/issues" target="_blank">Github</a>
                    </div>
                </div>
            </div>
            <div class="accordion-item">
                <h2 class="accordion-header" id="headingFour">
                    <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse"
                        data-bs-target="#collapseFour" aria-expanded="false" aria-controls="collapseFour">
                        <span class="badge bg-secondary">
                            <BIconChat class="fs-4" />
                        </span>&nbsp;Discussion
                    </button>
                </h2>
                <div id="collapseFour" class="accordion-collapse collapse" aria-labelledby="headingFour"
                    data-bs-parent="#accordionExample">
                    <div class="accordion-body">
                        Discuss with us on <a href="https://discord.gg/WzhxEY62mB" target="_blank">Discord</a> or <a
                            href="https://github.com/tbnobody/OpenDTU/discussions" target="_blank">Github</a>
                    </div>
                </div>
            </div>

        </div>

    </div>

</template>

<script lang="ts">
import { defineComponent } from 'vue';
import {
    BIconInfoCircle,
    BIconActivity,
    BIconBug,
    BIconChat
} from 'bootstrap-icons-vue';

export default defineComponent({
    components: {
        BIconInfoCircle,
        BIconActivity,
        BIconBug,
        BIconChat,
    },
});

</script>