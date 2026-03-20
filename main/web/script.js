const translations = {
    es: {
        title: "Lizarte On! Configurator",
        status: "Dispositivo conectado y listo.",
        next: "Ejecutar Paso",
        finish: "Finalizar",
        completed: "¡Acción completada!",
        alertNeedByte: "Configura el byte XX y luego ejecuta el Paso 1.",
        errorGeneric: "Error",
        coding: {
            codingTitle: "Configuración del byte XX",
            help: "Ayuda",
            debugRx: "Respuesta RX:",
            dsrSub: "DSR — Recomendación de dirección",
            dsrOn: "Si Activado — Asistencia a la estabilidad mediante impulso en el volante",
            dsrOff: "Si Desactivado — Sin asistencia en situaciones de inestabilidad",
            parkSub: "Asistente de aparcamiento",
            parkOn: "Si Activo — Vehículo con Park Assist instalado",
            parkOff: "Si Desactivado — Vehículo sin aparcamiento automático",
            tscSub: "TSC — Compensación de tirado lateral",
            tsc0: "Desactivado — Sin compensación (motores pequeños o tracción total)",
            tsc4: "Con valores aprendidos — Compensación adaptativa (tracción delantera, motor potente)",
            tsc8: "Sin valores de aprendizaje — Compensación fija",
            laneSub: "Asistente de mantenimiento de carril",
            laneOn: "Activado — Vehículo equipado con cámara frontal",
            laneOff: "Desactivado — Vehículo sin cámara frontal",
            angSub: "Sensor de ángulo de dirección externo",
            angOn: "Activado — Si vehículo con sensor independiente",
            angOff: "Desactivado — Sensor interno integrado en cremallera (habitual)",
            profSub: "Selección del perfil de conductor",
            profOn: "Si Activado — Dureza de dirección variable según modo de conducción",
            profOff: "Si Desactivado — Dureza de dirección constante"
        },
        endstopHelp: {
            barTitle: "Guía de ajuste de topes"
        }
    },
    en: {
        title: "Lizarte On! Configurator",
        status: "Device connected and ready.",
        next: "Execute step",
        finish: "Finish",
        completed: "Action completed!",
        alertNeedByte: "Configure byte XX and then run step 1.",
        errorGeneric: "Error",
        coding: {
            codingTitle: "Byte XX configuration",
            help: "Help",
            debugRx: "RX response:",
            dsrSub: "DSR — Steering recommendation",
            dsrOn: "If enabled — Stability assist via steering impulse",
            dsrOff: "If disabled — No assist in instability situations",
            parkSub: "Park Assist",
            parkOn: "If active — Vehicle with Park Assist installed",
            parkOff: "If disabled — Vehicle without automatic parking",
            tscSub: "TSC — Side-pull compensation",
            tsc0: "Disabled — No compensation (small engines or all-wheel drive)",
            tsc4: "With learned values — Adaptive compensation (front-wheel drive, powerful engine)",
            tsc8: "Without learned values — Fixed compensation",
            laneSub: "Lane keeping assist",
            laneOn: "Enabled — Vehicle with front camera",
            laneOff: "Disabled — Vehicle without front camera",
            angSub: "External steering angle sensor",
            angOn: "Enabled — Vehicle with standalone sensor",
            angOff: "Disabled — Sensor integrated in rack (typical)",
            profSub: "Driver profile selection",
            profOn: "If enabled — Steering effort varies with driving mode",
            profOff: "If disabled — Constant steering effort"
        },
        endstopHelp: {
            barTitle: "Endstop adjustment guide"
        }
    },
    fr: {
        title: "Lizarte On! Configurator",
        status: "Appareil connecté et prêt.",
        next: "Exécuter l'étape",
        finish: "Terminer",
        completed: "Action terminée !",
        alertNeedByte: "Configurez l'octet XX puis exécutez l'étape 1.",
        errorGeneric: "Erreur",
        coding: {
            codingTitle: "Configuration de l'octet XX",
            help: "Aide",
            debugRx: "Réponse RX :",
            dsrSub: "DSR — Recommandation de direction",
            dsrOn: "Si activé — Assistance à la stabilité par impulsion sur le volant",
            dsrOff: "Si désactivé — Pas d'assistance en situation d'instabilité",
            parkSub: "Assistant de stationnement",
            parkOn: "Si actif — Véhicule avec Park Assist installé",
            parkOff: "Si désactivé — Véhicule sans stationnement automatique",
            tscSub: "TSC — Compensation du tirage latéral",
            tsc0: "Désactivé — Pas de compensation (petits moteurs ou transmission intégrale)",
            tsc4: "Avec valeurs apprises — Compensation adaptative (traction avant, moteur puissant)",
            tsc8: "Sans valeurs d'apprentissage — Compensation fixe",
            laneSub: "Assistant de maintien de voie",
            laneOn: "Activé — Véhicule avec caméra avant",
            laneOff: "Désactivé — Véhicule sans caméra avant",
            angSub: "Capteur d'angle de direction externe",
            angOn: "Activé — Véhicule avec capteur indépendant",
            angOff: "Désactivé — Capteur interne dans la crémaillère (courant)",
            profSub: "Sélection du profil conducteur",
            profOn: "Si activé — Effort de direction variable selon le mode de conduite",
            profOff: "Si désactivé — Effort de direction constant"
        },
        endstopHelp: {
            barTitle: "Guide de réglage des butées"
        }
    },
    de: {
        title: "Lizarte On! Configurator",
        status: "Gerät verbunden und bereit.",
        next: "Schritt ausführen",
        finish: "Abschließen",
        completed: "Aktion abgeschlossen!",
        alertNeedByte: "Byte XX konfigurieren und dann Schritt 1 ausführen.",
        errorGeneric: "Fehler",
        coding: {
            codingTitle: "Byte-XX-Konfiguration",
            help: "Hilfe",
            debugRx: "RX-Antwort:",
            dsrSub: "DSR — Lenkempfehlung",
            dsrOn: "Wenn aktiv — Stabilitätshilfe durch Lenkimpuls",
            dsrOff: "Wenn inaktiv — Keine Hilfe bei Instabilität",
            parkSub: "Parkassistent",
            parkOn: "Wenn aktiv — Fahrzeug mit Park Assist",
            parkOff: "Wenn inaktiv — Fahrzeug ohne automatisches Einparken",
            tscSub: "TSC — Seitlichen Zug ausgleichen",
            tsc0: "Deaktiviert — Kein Ausgleich (kleine Motoren oder Allrad)",
            tsc4: "Mit Lerndaten — Adaptiver Ausgleich (Frontantrieb, starker Motor)",
            tsc8: "Ohne Lerndaten — Fester Ausgleich",
            laneSub: "Spurhalteassistent",
            laneOn: "Aktiv — Fahrzeug mit Frontkamera",
            laneOff: "Inaktiv — Fahrzeug ohne Frontkamera",
            angSub: "Externer Lenkwinkelsensor",
            angOn: "Aktiv — Fahrzeug mit separatem Sensor",
            angOff: "Inaktiv — Sensor im Lenkgetriebe integriert (üblich)",
            profSub: "Fahrerprofil-Auswahl",
            profOn: "Wenn aktiv — Lenkkräfte je nach Fahrmodus",
            profOff: "Wenn inaktiv — Konstante Lenkkräfte"
        },
        endstopHelp: {
            barTitle: "Leitfaden Anschlag-Einstellung"
        }
    }
};

function applyEndstopHelpLang(lang) {
    const e = translations[lang]?.endstopHelp;
    const c = translations[lang]?.coding;
    if (!e || !c) return;
    const titleEl = document.getElementById('i18n-action2-help-title');
    if (titleEl) titleEl.textContent = e.barTitle;
    const link = document.getElementById('action2-help-link');
    if (link) {
        link.textContent = c.help;
        link.href = '/guia-topes.html?lang=' + encodeURIComponent(lang);
    }
}

function applyCodingUiLang(lang) {
    const c = translations[lang]?.coding;
    if (!c) return;
    const set = (id, text) => {
        const el = document.getElementById(id);
        if (el) el.textContent = text;
    };
    set('i18n-coding-title', c.codingTitle);
    set('i18n-debug-rx', c.debugRx);
    set('i18n-dsr-sub', c.dsrSub);
    set('i18n-dsr-on', c.dsrOn);
    set('i18n-dsr-off', c.dsrOff);
    set('i18n-park-sub', c.parkSub);
    set('i18n-park-on', c.parkOn);
    set('i18n-park-off', c.parkOff);
    set('i18n-tsc-sub', c.tscSub);
    set('i18n-tsc-0', c.tsc0);
    set('i18n-tsc-4', c.tsc4);
    set('i18n-tsc-8', c.tsc8);
    set('i18n-lane-sub', c.laneSub);
    set('i18n-lane-on', c.laneOn);
    set('i18n-lane-off', c.laneOff);
    set('i18n-ang-sub', c.angSub);
    set('i18n-ang-on', c.angOn);
    set('i18n-ang-off', c.angOff);
    set('i18n-prof-sub', c.profSub);
    set('i18n-prof-on', c.profOn);
    set('i18n-prof-off', c.profOff);
    const help = document.getElementById('help-guide-link');
    if (help) {
        help.textContent = c.help;
        help.href = '/guia-codificacion.html?lang=' + encodeURIComponent(lang);
    }
}

let canData = null;
let currentActionKey = null;
let currentStepIdx = 0;
let xxTouched = false;

const SUPPORTED_LANGS = ['es', 'en', 'fr', 'de'];

function applyLangFromUrl() {
    const q = new URLSearchParams(window.location.search).get('lang');
    if (!q) return;
    const lang = q.toLowerCase();
    if (!SUPPORTED_LANGS.includes(lang)) return;
    const sel = document.getElementById('language-select');
    if (sel) sel.value = lang;
    const cleanPath = window.location.pathname || '/';
    if (window.history && window.history.replaceState) {
        window.history.replaceState(null, '', cleanPath);
    }
}

function getStepOrderForAction(actionKey) {
    // Acción 1: el paso 2 se ejecuta automáticamente dentro del paso 1 del firmware
    // Por UX, ocultamos el "Paso 2" y mostramos solo 4 pantallas: [0,2,3,4]
    if (actionKey === 'action1') return [0, 2, 3, 4];
    return null; // orden natural
}

function getActualStepIdx() {
    const order = getStepOrderForAction(currentActionKey);
    if (!order) return currentStepIdx;
    return order[currentStepIdx] ?? 0;
}

function getTotalUiSteps() {
    const order = getStepOrderForAction(currentActionKey);
    if (!order) return (canData?.[currentActionKey]?.steps?.length ?? 0);
    return order.length;
}

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
    document.documentElement.lang = lang;

    // Main screen texts
    document.getElementById('title').innerText = t.title;
    document.getElementById('status-text').innerText = t.status;
    applyCodingUiLang(lang);
    applyEndstopHelpLang(lang);

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
    document.getElementById('total-steps-num').innerText = getTotalUiSteps();

    const actualIdx = getActualStepIdx();
    const step = steps[actualIdx];
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
        // El byte XX debe estar listo antes del paso 1; lo mostramos también en el paso 1.
        if (currentActionKey === 'action1' && currentStepIdx === 0) {
            selectorCard.classList.remove('hidden');
            updateCodingByte();
        } else {
            selectorCard.classList.add('hidden');
        }
    }

    const action2HelpBar = document.getElementById('action2-help-bar');
    if (action2HelpBar) {
        if (currentActionKey === 'action2') {
            action2HelpBar.classList.remove('hidden');
        } else {
            action2HelpBar.classList.add('hidden');
        }
    }
    applyEndstopHelpLang(lang);

    // Progress
    const totalUi = Math.max(1, getTotalUiSteps());
    const progress = (currentStepIdx / totalUi) * 100;
    document.getElementById('progress-inner').style.width = `${progress}%`;

    // Reset button to default behavior
    const btn = document.getElementById('next-step-btn');
    btn.onclick = executeStep;
    btn.disabled = false;

    if (currentStepIdx === getTotalUiSteps() - 1) {
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

    const actualStepIdx = getActualStepIdx();
    let url = `/api/execute_step?action=${currentActionKey}&step=${actualStepIdx}`;
    // En Acción 1 el XX debe enviarse ya en el Paso 1 (para que el firmware ejecute Paso 2 inmediatamente).
    if (currentActionKey === 'action1' && actualStepIdx === 0) {
        const selectedValue = updateCodingByte();
        if (!xxTouched || !selectedValue) {
            alert(translations[lang].alertNeedByte);
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
                    // Para Acción 1 ocultamos el paso 2 (automático) y saltamos a Reset
                    currentStepIdx = 1;
                    setTimeout(() => updateStepUI(), 300);
                } else {
                    btn.disabled = false;
                }
            } else if ((currentActionKey === 'action1' && (currentStepIdx >= 1 && currentStepIdx <= 3)) ||
                       (currentActionKey === 'action2' && (currentStepIdx === 1 || currentStepIdx === 2 || currentStepIdx === 3))) {
                if (data && data.step_ok) {
                    if (currentStepIdx < getTotalUiSteps() - 1) {
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
            } else if (currentStepIdx < getTotalUiSteps() - 1) {
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
            alert(translations[lang].errorGeneric);
            btn.disabled = false;
        }
    } catch (err) {
        console.error(err);
        btn.disabled = false;
    }
}

document.addEventListener('DOMContentLoaded', () => {
    applyLangFromUrl();
    loadCanData().then(() => {
        changeLanguage();
    });

    const inputs = [
        'bit0-dsr', 'bit1-parking', 'bit4-lane', 'bit5-angle', 'bit7-profile'
    ];
    inputs.forEach(id => {
        const el = document.getElementById(id);
        if (el) el.addEventListener('change', () => { xxTouched = true; updateCodingByte(); });
    });
    document.querySelectorAll('input[name="tsc"]').forEach(el => {
        el.addEventListener('change', () => { xxTouched = true; updateCodingByte(); });
    });

    // Default 0x85: DSR + TSC Lernwerten + perfil conductor
    const bit0 = document.getElementById('bit0-dsr');
    const bit7 = document.getElementById('bit7-profile');
    const tscLern = document.querySelector('input[name="tsc"][value="0x04"]');
    if (bit0) bit0.checked = true;
    if (bit7) bit7.checked = true;
    if (tscLern) tscLern.checked = true;
    updateCodingByte();
    // Con valores por defecto ya hay un XX definido
    xxTouched = true;
});
