const translations = {
    es: {
        title: "Lizarte On! Configurator",
        status: "Dispositivo conectado y listo.",
        btn1: "Acción 1",
        btn2: "Acción 2",
        next: "Siguiente Paso",
        finish: "Finalizar",
        completed: "¡Acción completada!"
    },
    en: {
        title: "Lizarte On! Configurator",
        status: "Device connected and ready.",
        btn1: "Action 1",
        btn2: "Action 2",
        next: "Next Step",
        finish: "Finish",
        completed: "Action completed!"
    },
    fr: {
        title: "Lizarte On! Configurator",
        status: "Appareil conectado y listo.",
        btn1: "Action 1",
        btn2: "Action 2",
        next: "Étape Suivante",
        finish: "Terminer",
        completed: "Action terminée!"
    },
    de: {
        title: "Lizarte On! Configurator",
        status: "Gerät verbunden und bereit.",
        btn1: "Aktion 1",
        btn2: "Aktion 2",
        next: "Nächster Schritt",
        finish: "Abschließen",
        completed: "Aktion abgeschlossen!"
    }
};

let canData = null;
let currentActionKey = null;
let currentStepIdx = 0;

// Fetch CAN frames/steps on start
async function loadCanData() {
    try {
        const response = await fetch('can_frames.json');
        canData = await response.json();
        console.log("CAN Data loaded:", canData);
    } catch (err) {
        console.error("Failed to load CAN data", err);
    }
}

function changeLanguage() {
    const lang = document.getElementById('language-select').value;
    const t = translations[lang];

    // Main screen texts
    document.getElementById('title').innerText = t.title;
    document.getElementById('status-text').innerText = t.status;
    document.querySelector('#btn-1 .btn-text').innerText = t.btn1;
    document.querySelector('#btn-2 .btn-text').innerText = t.btn2;

    // If we are in step screen, update it too
    if (currentActionKey) {
        updateStepUI();
    }
}

function showMainScreen() {
    document.getElementById('step-screen').classList.add('hidden');
    document.getElementById('main-screen').classList.remove('hidden');
    currentActionKey = null;
}

function startAction(actionKey) {
    if (!canData) return;
    currentActionKey = actionKey;
    currentStepIdx = 0;

    document.getElementById('main-screen').classList.add('hidden');
    document.getElementById('step-screen').classList.remove('hidden');

    updateStepUI();
}

function updateStepUI() {
    const lang = document.getElementById('language-select').value;
    const action = canData[currentActionKey];
    const steps = action.steps;
    const t = translations[lang];

    document.getElementById('step-action-title').innerText = action.title[lang];
    document.getElementById('current-step-num').innerText = currentStepIdx + 1;
    document.getElementById('total-steps-num').innerText = steps.length;

    const step = steps[currentStepIdx];
    document.getElementById('step-description').innerText = step.description[lang];

    // Progress
    const progress = (currentStepIdx / steps.length) * 100;
    document.getElementById('progress-inner').style.width = `${progress}%`;

    // Reset button to default behavior
    const btn = document.getElementById('next-step-btn');
    btn.onclick = executeStep;
    btn.disabled = false;

    if (currentStepIdx === steps.length - 1) {
        btn.innerText = t.finish;
    } else {
        btn.innerText = t.next;
    }
}

async function executeStep() {
    if (!currentActionKey) return;

    const btn = document.getElementById('next-step-btn');
    btn.disabled = true;

    const action = canData[currentActionKey];
    const lang = document.getElementById('language-select').value;

    try {
        const response = await fetch(`/api/execute_step?action=${currentActionKey}&step=${currentStepIdx}`, {
            method: 'POST'
        });

        if (response.ok) {
            if (currentStepIdx < action.steps.length - 1) {
                currentStepIdx++;
                setTimeout(() => updateStepUI(), 300);
            } else {
                // Final step completed
                document.getElementById('progress-inner').style.width = `100%`;
                document.getElementById('step-description').innerText = translations[lang].completed;
                btn.innerText = translations[lang].finish;
                btn.disabled = false;
                btn.onclick = showMainScreen; // Temporary override for final click
            }
        } else {
            alert("Error");
            btn.disabled = false;
        }
    } catch (err) {
        console.error(err);
        btn.disabled = false;
    }
}

document.addEventListener('DOMContentLoaded', () => {
    loadCanData().then(() => {
        changeLanguage();
    });
});
