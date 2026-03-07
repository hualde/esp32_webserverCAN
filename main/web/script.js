const translations = {
    es: {
        title: "Panel de Control ESP32",
        status: "Dispositivo conectado y listo.",
        btn1: "Acción 1",
        btn2: "Acción 2",
        footer: "Servidor Web ESP32 v5.3.2"
    },
    en: {
        title: "ESP32 Control Panel",
        status: "Device connected and ready.",
        btn1: "Action 1",
        btn2: "Action 2",
        footer: "ESP32 Web Server v5.3.2"
    },
    fr: {
        title: "Tableau de Bord ESP32",
        status: "Appareil connecté et prêt.",
        btn1: "Action 1",
        btn2: "Action 2",
        footer: "Serveur Web ESP32 v5.3.2"
    }
};

function changeLanguage() {
    const lang = document.getElementById('language-select').value;
    const t = translations[lang];

    document.getElementById('title').innerText = t.title;
    document.getElementById('status-text').innerText = t.status;
    document.querySelector('#btn-1 .btn-text').innerText = t.btn1;
    document.querySelector('#btn-2 .btn-text').innerText = t.btn2;
    document.getElementById('footer-text').innerText = t.footer;

    // Optional: Notify the ESP32 about language change
    fetch(`/api/set_lang?lang=${lang}`, { method: 'POST' });
}

function handleAction(id) {
    const statusText = document.getElementById('status-text');
    const lang = document.getElementById('language-select').value;

    const messages = {
        es: `Ejecutando Acción ${id}...`,
        en: `Executing Action ${id}...`,
        fr: `Exécution de l'Action ${id}...`
    };

    statusText.innerText = messages[lang];
    statusText.style.color = 'var(--primary-color)';

    fetch(`/api/action${id}`, { method: 'POST' })
        .then(response => {
            setTimeout(() => {
                statusText.innerText = translations[lang].status;
                statusText.style.color = '#888';
            }, 2000);
        })
        .catch(err => {
            statusText.innerText = "Error!";
            statusText.style.color = "#ff4b2b";
        });
}

// Initial set (should be ES by default as per HTML)
document.addEventListener('DOMContentLoaded', () => {
    // Check if we have a saved language or default to ES
    // For now just ensure it's synced
    changeLanguage();
});
