const translations = {
    es: {
        titleBrand: "Lizarte On",
        titleSuffix: "! Configurator",
        status: "Dispositivo conectado y listo.",
        next: "Ejecutar Paso",
        finish: "Finalizar",
        completed: "¡Acción completada!",
        alertNeedByte: "Configura el byte XX y luego ejecuta el Paso 1.",
        errorGeneric: "Error",
        backStep: "Volver a la pantalla principal",
        coding: {
            codingTitle: "Configuración",
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
        },
        a2fb: {
            title: "Estado de flancos",
            hint: "Gira el volante a tope a un lado, al otro y al centro (repite si hace falta) hasta que los CINCO indicadores estén en verde. Avanza solo.",
            f0: "Flanco 1", f1: "Flanco 2", f2: "Flanco 3", f3: "Flanco 4", f4: "Flanco 5",
            tqTitle: "Par contra los topes",
            tqHint: "Gira a tope y APRIETA FUERTE (más de 7,5 Nm) a cada lado hasta que los dos topes estén en verde. Avanza solo.",
            s1: "Tope 1 (> 7,5 Nm)", s2: "Tope 2 (> 7,5 Nm)",
            authWait: "Esperando estado «Autorizado»…",
            authOk: "Autorizado",
            working: "Procesando…"
        }
    },
    en: {
        titleBrand: "Lizarte On",
        titleSuffix: "! Configurator",
        status: "Device connected and ready.",
        next: "Execute step",
        finish: "Finish",
        completed: "Action completed!",
        alertNeedByte: "Configure byte XX and then run step 1.",
        errorGeneric: "Error",
        backStep: "Back to main screen",
        coding: {
            codingTitle: "Configuration",
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
        },
        a2fb: {
            title: "Edge status",
            hint: "Turn the wheel fully to one side, then the other, then centre (repeat if needed) until ALL FIVE indicators are green. It advances automatically.",
            f0: "Edge 1", f1: "Edge 2", f2: "Edge 3", f3: "Edge 4", f4: "Edge 5",
            tqTitle: "Torque against the stops",
            tqHint: "Turn to each stop and PUSH HARD (more than 7.5 Nm) until both stops are green. It advances automatically.",
            s1: "Stop 1 (> 7.5 Nm)", s2: "Stop 2 (> 7.5 Nm)",
            authWait: "Waiting for “Authorised” state…",
            authOk: "Authorised",
            working: "Processing…"
        }
    },
    fr: {
        titleBrand: "Lizarte On",
        titleSuffix: "! Configurator",
        status: "Appareil connecté et prêt.",
        next: "Exécuter l'étape",
        finish: "Terminer",
        completed: "Action terminée !",
        alertNeedByte: "Configurez l'octet XX puis exécutez l'étape 1.",
        errorGeneric: "Erreur",
        backStep: "Retour à l'écran principal",
        coding: {
            codingTitle: "Configuration",
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
        },
        a2fb: {
            title: "État des flancs",
            hint: "Tournez le volant en butée d'un côté, puis de l'autre, puis au centre (répétez si nécessaire) jusqu'à ce que les CINQ indicateurs soient verts. L'avancement est automatique.",
            f0: "Flanc 1", f1: "Flanc 2", f2: "Flanc 3", f3: "Flanc 4", f4: "Flanc 5",
            tqTitle: "Couple contre les butées",
            tqHint: "Tournez en butée et APPUYEZ FORT (plus de 7,5 Nm) de chaque côté jusqu'à ce que les deux butées soient vertes. L'avancement est automatique.",
            s1: "Butée 1 (> 7,5 Nm)", s2: "Butée 2 (> 7,5 Nm)",
            authWait: "En attente de l'état « Autorisé »…",
            authOk: "Autorisé",
            working: "Traitement…"
        }
    },
    de: {
        titleBrand: "Lizarte On",
        titleSuffix: "! Configurator",
        status: "Gerät verbunden und bereit.",
        next: "Schritt ausführen",
        finish: "Abschließen",
        completed: "Aktion abgeschlossen!",
        alertNeedByte: "Byte XX konfigurieren und dann Schritt 1 ausführen.",
        errorGeneric: "Fehler",
        backStep: "Zurück zum Hauptbildschirm",
        coding: {
            codingTitle: "Konfiguration",
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
        },
        a2fb: {
            title: "Flankenstatus",
            hint: "Lenkrad ganz zu einer Seite, dann zur anderen, dann mittig drehen (ggf. wiederholen), bis ALLE FÜNF Anzeigen grün sind. Es geht automatisch weiter.",
            f0: "Flanke 1", f1: "Flanke 2", f2: "Flanke 3", f3: "Flanke 4", f4: "Flanke 5",
            tqTitle: "Moment gegen die Anschläge",
            tqHint: "Bis zum Anschlag drehen und KRÄFTIG DRÜCKEN (mehr als 7,5 Nm), auf beiden Seiten, bis beide Anschläge grün sind. Es geht automatisch weiter.",
            s1: "Anschlag 1 (> 7,5 Nm)", s2: "Anschlag 2 (> 7,5 Nm)",
            authWait: "Warte auf Status „Autorisiert“…",
            authOk: "Autorisiert",
            working: "Verarbeitung…"
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

function applyA2FeedbackLang(lang) {
    const f = translations[lang]?.a2fb;
    if (!f) return;
    const set = (id, text) => { const el = document.getElementById(id); if (el) el.textContent = text; };
    set('i18n-a2fb-title', f.title);
    set('i18n-a2fb-hint', f.hint);
    set('i18n-a2fb-f0', f.f0);
    set('i18n-a2fb-f1', f.f1);
    set('i18n-a2fb-f2', f.f2);
    set('i18n-a2fb-f3', f.f3);
    set('i18n-a2fb-f4', f.f4);
    set('i18n-a2fb-s1', f.s1);
    set('i18n-a2fb-s2', f.s2);
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
    const brandEl = document.getElementById('title-brand');
    const suffixEl = document.getElementById('title-suffix');
    if (brandEl) brandEl.textContent = t.titleBrand;
    if (suffixEl) suffixEl.textContent = t.titleSuffix;
    document.getElementById('status-text').innerText = t.status;
    const stepBack = document.getElementById('step-back-btn');
    if (stepBack) stepBack.setAttribute('aria-label', t.backStep);
    applyCodingUiLang(lang);
    applyEndstopHelpLang(lang);
    applyA2FeedbackLang(lang);

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
    applyA2FeedbackLang(lang);
    // El feedback de flancos se muestra solo durante la ejecución de un paso de action2
    resetFlankFeedback();

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

function resetFlankFeedback() {
    const card = document.getElementById('action2-feedback');
    if (!card) return;
    card.classList.add('hidden');
    ['flank-0', 'flank-1', 'flank-2', 'flank-3', 'flank-4', 'stop-pos', 'stop-neg', 'auth-dot'].forEach(id => {
        const e = document.getElementById(id);
        if (e) e.classList.remove('on');
    });
    ['flank-grid', 'torque-box', 'auth-row'].forEach(id => {
        const e = document.getElementById(id);
        if (e) e.classList.add('hidden');
    });
    const tv = document.getElementById('torque-value');
    if (tv) { tv.textContent = '—'; tv.classList.remove('reached'); }
}

function setDot(id, on) {
    const e = document.getElementById(id);
    if (e) e.classList.toggle('on', !!on);
}

function updateFlankUI(s, lang) {
    const card = document.getElementById('action2-feedback');
    if (!card) return;
    const f = translations[lang]?.a2fb;
    const setText = (id, text) => { const e = document.getElementById(id); if (e && text) e.textContent = text; };
    if (s.flank && s.flank.valid) {
        card.classList.remove('hidden');
        document.getElementById('flank-grid')?.classList.remove('hidden');
        if (f) { setText('i18n-a2fb-title', f.title); setText('i18n-a2fb-hint', f.hint); }
        setDot('flank-0', s.flank.b0 === 1);
        setDot('flank-1', s.flank.b1 === 1);
        setDot('flank-2', s.flank.b2 === 1);
        setDot('flank-3', s.flank.b3 === 1);
        setDot('flank-4', s.flank.b4 === 1);
    }
    if (s.torque && s.torque.valid) {
        card.classList.remove('hidden');
        document.getElementById('torque-box')?.classList.remove('hidden');
        if (f) { setText('i18n-a2fb-title', f.tqTitle); setText('i18n-a2fb-hint', f.tqHint); }
        const nm = (s.torque.mnm || 0) / 1000;
        const tv = document.getElementById('torque-value');
        if (tv) {
            let txt = Math.abs(nm).toFixed(1);
            if (lang !== 'en') txt = txt.replace('.', ',');
            tv.textContent = txt + ' Nm';
            tv.classList.toggle('reached', Math.abs(nm) >= 7.5);
        }
        setDot('stop-pos', !!s.torque.pos);
        setDot('stop-neg', !!s.torque.neg);
    }
    if (s.auth && s.auth.valid) {
        card.classList.remove('hidden');
        const authRow = document.getElementById('auth-row');
        const authLbl = document.getElementById('auth-lbl');
        if (authRow) authRow.classList.remove('hidden');
        const ok = s.auth.a0 === 0;
        setDot('auth-dot', ok);
        if (authLbl && f) authLbl.textContent = ok ? f.authOk : f.authWait;
    }
}

function finishCurrentAction(lang) {
    document.getElementById('progress-inner').style.width = `100%`;
    document.getElementById('step-description').innerText = translations[lang].completed;
    const btn = document.getElementById('next-step-btn');
    btn.innerText = translations[lang].finish;
    btn.disabled = false;
    btn.onclick = showMainScreen;
}

async function executeStepAction2(stepIdx, lang, btn) {
    resetFlankFeedback();
    try {
        const response = await fetch(`/api/execute_step?action=action2&step=${stepIdx}`, { method: 'POST' });
        if (!response.ok) throw new Error('bad response');
        const data = await response.json().catch(() => null);
        if (data && data.started === false) {
            // Ya había un paso en curso; reintenta el sondeo igualmente
        }
    } catch (err) {
        console.error(err);
        alert(translations[lang].errorGeneric);
        btn.disabled = false;
        return;
    }

    const startStepIdx = currentStepIdx;
    let pollFails = 0;
    const timer = setInterval(async () => {
        // Si el usuario cambió de pantalla, deja de sondear
        if (currentActionKey !== 'action2' || currentStepIdx !== startStepIdx) {
            clearInterval(timer);
            return;
        }
        let s = null;
        try {
            const r = await fetch('/api/step_status');
            if (!r.ok) throw new Error('status ' + r.status);
            s = await r.json();
            pollFails = 0;
        } catch (e) {
            // Si el endpoint de estado no responde de forma persistente, no dejar la UI colgada
            if (++pollFails >= 30) {
                clearInterval(timer);
                alert(translations[lang].errorGeneric);
                btn.disabled = false;
            }
            return; // reintenta en el siguiente tick
        }
        updateFlankUI(s, lang);
        refreshKeyDebug();

        if (s.done) {
            clearInterval(timer);
            if (s.success) {
                if (currentStepIdx < getTotalUiSteps() - 1) {
                    currentStepIdx++;
                    setTimeout(() => updateStepUI(), 400);
                } else {
                    resetFlankFeedback();
                    finishCurrentAction(lang);
                }
            } else {
                // Falló o agotó el tiempo: deja el feedback visible y reactiva el botón
                btn.disabled = false;
            }
        }
    }, 500);
}

async function executeStep() {
    if (!currentActionKey) return;

    const btn = document.getElementById('next-step-btn');
    btn.disabled = true;

    const lang = document.getElementById('language-select').value;
    const actualStepIdx = getActualStepIdx();

    if (currentActionKey === 'action2') {
        return executeStepAction2(actualStepIdx, lang, btn);
    }

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
                       (currentActionKey === 'action2' && (currentStepIdx >= 1 && currentStepIdx <= 4))) {
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
