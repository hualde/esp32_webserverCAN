const translations = {
    es: {
        title: "Lizarte On! Configurator",
        status: "Dispositivo conectado y listo.",
        next: "Ejecutar Paso",
        finish: "Finalizar",
        completed: "¡Acción completada!"
    },
    en: {
        title: "Lizarte On! Configurator",
        status: "Device connected and ready.",
        next: "Execute Step",
        finish: "Finish",
        completed: "Action completed!"
    },
    fr: {
        title: "Lizarte On! Configurator",
        status: "Appareil connecté y listo.",
        next: "Exécuter l'étape",
        finish: "Terminer",
        completed: "Action terminée!"
    },
    de: {
        title: "Lizarte On! Configurator",
        status: "Gerät verbunden und bereit.",
        next: "Schritt ausführen",
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

    // Sync button titles with can_frames.json if available
    if (canData) {
        if (canData.action1) {
            document.querySelector('#btn-1 .btn-text').innerText = canData.action1.title[lang];
        }
        if (canData.action2) {
            document.querySelector('#btn-2 .btn-text').innerText = canData.action2.title[lang];
        }
    }

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

    const debugCard = document.getElementById('key-debug');
    if (debugCard) {
        if ((currentActionKey === 'action1' || currentActionKey === 'action2') && currentStepIdx === 0) {
            refreshKeyDebug();
        } else {
            debugCard.classList.add('hidden');
        }
    }

    const selectorCard = document.getElementById('step2-selector');
    if (selectorCard) {
        if (currentActionKey === 'action1' && currentStepIdx === 1) {
            selectorCard.classList.remove('hidden');
            updateCodingByte();
        } else {
            selectorCard.classList.add('hidden');
        }
    }

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

async function refreshKeyDebug() {
    const debugCard = document.getElementById('key-debug');
    const debugValue = document.getElementById('key-debug-value');
    if (!debugCard || !debugValue) return;

    try {
        const response = await fetch('/api/last_rx');
        if (!response.ok) throw new Error('Bad response');
        const data = await response.json();
        if (data.valid) {
            debugValue.innerText = data.key;
            debugCard.classList.remove('hidden');
        } else {
            debugValue.innerText = '---';
            debugCard.classList.add('hidden');
        }
    } catch (err) {
        debugCard.classList.add('hidden');
    }
}

function getSelectedTscValue() {
    const radios = document.querySelectorAll('input[name="tsc"]');
    for (const r of radios) {
        if (r.checked) return parseInt(r.value, 16);
    }
    return 0x00;
}

function updateCodingByte() {
    const bit0 = document.getElementById('bit0-dsr')?.checked ? 0x01 : 0x00;
    const bit1 = document.getElementById('bit1-parking')?.checked ? 0x02 : 0x00;
    const tsc = getSelectedTscValue(); // 0x00, 0x04, 0x08
    const bit4 = document.getElementById('bit4-lane')?.checked ? 0x10 : 0x00;
    const bit5 = document.getElementById('bit5-angle')?.checked ? 0x20 : 0x00;
    const bit7 = document.getElementById('bit7-profile')?.checked ? 0x80 : 0x00;

    const value = bit0 | bit1 | tsc | bit4 | bit5 | bit7;
    const hex = `0x${value.toString(16).toUpperCase().padStart(2, '0')}`;
    const label = document.getElementById('xx-value');
    if (label) label.innerText = hex;
    return hex;
}

async function executeStep() {
    if (!currentActionKey) return;

    const btn = document.getElementById('next-step-btn');
    btn.disabled = true;

    const action = canData[currentActionKey];
    const lang = document.getElementById('language-select').value;

    let url = `/api/execute_step?action=${currentActionKey}&step=${currentStepIdx}`;
    if (currentActionKey === 'action1' && currentStepIdx === 1) {
        const selectedValue = updateCodingByte();
        if (!selectedValue) {
            alert("Selecciona una opción");
            btn.disabled = false;
            return;
        }
        url += `&xx=${encodeURIComponent(selectedValue)}`;
    }

    try {
        const response = await fetch(url, {
            method: 'POST'
        });

        if (response.ok) {
            let data = null;
            try {
                data = await response.json();
            } catch (e) {
                data = null;
            }

            refreshKeyDebug();

            if ((currentActionKey === 'action1' || currentActionKey === 'action2') && currentStepIdx === 0) {
                if (data && data.access_ok) {
                    currentStepIdx = 1;
                    setTimeout(() => updateStepUI(), 300);
                } else {
                    btn.disabled = false;
                }
            } else if ((currentActionKey === 'action1' && (currentStepIdx >= 1 && currentStepIdx <= 4)) ||
                       (currentActionKey === 'action2' && (currentStepIdx === 1 || currentStepIdx === 2 || currentStepIdx === 3))) {
                if (data && data.step_ok) {
                    if (currentStepIdx < action.steps.length - 1) {
                        currentStepIdx++;
                        setTimeout(() => updateStepUI(), 300);
                    } else {
                        document.getElementById('progress-inner').style.width = `100%`;
                        document.getElementById('step-description').innerText = translations[lang].completed;
                        btn.innerText = translations[lang].finish;
                        btn.disabled = false;
                        btn.onclick = showMainScreen;
                    }
                } else {
                    btn.disabled = false;
                }
            } else if (currentStepIdx < action.steps.length - 1) {
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

    const inputs = [
        'bit0-dsr', 'bit1-parking', 'bit4-lane', 'bit5-angle', 'bit7-profile'
    ];
    inputs.forEach(id => {
        const el = document.getElementById(id);
        if (el) el.addEventListener('change', updateCodingByte);
    });
    document.querySelectorAll('input[name="tsc"]').forEach(el => {
        el.addEventListener('change', updateCodingByte);
    });

    // Default 0x85: DSR + TSC Lernwerten + perfil conductor
    const bit0 = document.getElementById('bit0-dsr');
    const bit7 = document.getElementById('bit7-profile');
    const tscLern = document.querySelector('input[name="tsc"][value="0x04"]');
    if (bit0) bit0.checked = true;
    if (bit7) bit7.checked = true;
    if (tscLern) tscLern.checked = true;
    updateCodingByte();
});
