#pragma once

const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head><meta charset="UTF-8"><title>TOTP Authenticator</title><meta name="viewport" content="width=device-width, initial-scale=1"><link rel="icon" type="image/svg+xml" href="/favicon.svg"><link rel="alternate icon" href="/favicon.ico"><style>
@keyframes gradient-animation {
    0% { background-position: 0% 50%; }
    50% { background-position: 100% 50%; }
    100% { background-position: 0% 50%; }
}

@keyframes pulse {
    0% { transform: scale(1); opacity: 1; }
    50% { transform: scale(1.05); opacity: 0.8; }
    100% { transform: scale(1); opacity: 1; }
}

body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background: linear-gradient(-45deg, #1a1a2e, #16213e, #0f3460, #2e4a62);
    background-size: 400% 400%;
    animation: gradient-animation 15s ease infinite;
    color: #e0e0e0;
    margin: 0;
    padding: 20px;
    min-height: 100vh;
}

h2, h3 {
    color: #ffffff;
    text-align: center;
    font-weight: 300;
    letter-spacing: 1px;
    margin-bottom: 2rem;
}

.form-container, .content-box {
    max-width: 800px;
    margin: 20px auto;
    padding: 25px;
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    backdrop-filter: blur(10px);
    border-radius: 15px;
    box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
}

table {
    width: 100%;
    border-collapse: collapse;
    background: rgba(255, 255, 255, 0.02);
    border-radius: 8px;
    overflow: hidden;
}

th, td {
    padding: 15px;
    text-align: center;
    vertical-align: middle;
    border: 1px solid rgba(255, 255, 255, 0.1);
}

th {
    background: rgba(90, 158, 238, 0.2);
    color: #ffffff;
    font-weight: 500;
}

td.code {
    font-family: 'SF Mono', 'Monaco', 'Inconsolata', 'Fira Code', 'Fira Mono', 'Droid Sans Mono', 'Consolas', monospace;
    font-size: 1.4em;
    font-weight: bold;
    color: #5a9eee;
    background: rgba(90, 158, 238, 0.1);
    cursor: pointer;
    transition: all 0.3s ease;
    user-select: none;
}

td.code:hover {
    background: rgba(90, 158, 238, 0.2);
    transform: scale(1.05);
    box-shadow: 0 2px 8px rgba(90, 158, 238, 0.3);
}

input[type="text"], input[type="password"], input[type="number"], input[type="file"], select {
    width: calc(100% - 22px);
    padding: 12px;
    margin-bottom: 15px;
    background-color: rgba(0, 0, 0, 0.2);
    border: 1px solid rgba(255, 255, 255, 0.2);
    color: #e0e0e0;
    border-radius: 8px;
    transition: all 0.3s ease;
    font-size: 1rem;
}

input:focus, select:focus {
    outline: none;
    border-color: #5a9eee;
    box-shadow: 0 0 0 3px rgba(90, 158, 238, 0.3);
}

label {
    color: #b0b0b0;
    font-size: 0.9rem;
    margin-bottom: 8px;
    display: block;
}

.button, .button-copy, .button-delete, .button-action {
    display: inline-block;
    padding: 12px 20px;
    border: none;
    border-radius: 8px;
    color: white;
    cursor: pointer;
    text-decoration: none;
    margin-right: 10px;
    margin-bottom: 10px;
    font-size: 1rem;
    font-weight: 500;
    transition: all 0.3s ease;
}

.button, .button-copy {
    background-color: #5a9eee;
}

.button:hover {
    background-color: #4a8bdb;
    transform: translateY(-1px);
}

.button-delete {
    background-color: #e74c3c;
}

.button-delete:hover {
    background-color: #c0392b;
    transform: translateY(-1px);
}

.button-action {
    background-color: #6c757d;
}

.button-action:hover {
    background-color: #5a6268;
    transform: translateY(-1px);
}

/* Стильный switch toggle */
.switch {
    position: relative;
    display: inline-block;
    width: 60px;
    height: 34px;
}

.switch input {
    opacity: 0;
    width: 0;
    height: 0;
}

.slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: #ccc;
    transition: .4s;
    border-radius: 34px;
}

.slider:before {
    position: absolute;
    content: "";
    height: 26px;
    width: 26px;
    left: 4px;
    bottom: 4px;
    background-color: white;
    transition: .4s;
    border-radius: 50%;
}

input:checked + .slider {
    background-color: #5a9eee;
}

input:focus + .slider {
    box-shadow: 0 0 1px #5a9eee;
}

input:checked + .slider:before {
    transform: translateX(26px);
}

.tabs {
    overflow: hidden;
    background: rgba(255, 255, 255, 0.05);
    backdrop-filter: blur(10px);
    border: 1px solid rgba(255, 255, 255, 0.1);
    max-width: 820px;
    margin: auto;
    border-radius: 15px 15px 0 0;
}

#widget-bar {
    max-width: 820px;
    margin: 0 auto;
    background: rgba(255, 255, 255, 0.03);
    border-left: 1px solid rgba(255, 255, 255, 0.1);
    border-right: 1px solid rgba(255, 255, 255, 0.1);
    border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    height: 28px;
    box-sizing: border-box;
    overflow: visible;
    display: flex;
    align-items: center;
    justify-content: space-between;
}

#battery-widget {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    height: 100%;
    padding: 0 10px;
    gap: 5px;
    font-size: 11px;
    color: rgba(255,255,255,0.55);
    font-family: monospace;
    transition: color 0.3s;
    box-sizing: border-box;
    overflow: hidden;
}

#battery-widget.critical { color: #ff5555; }

#lang-switcher {
    position: relative;
    height: 100%;
    display: flex;
    align-items: center;
    padding: 0 8px;
    flex-shrink: 0;
}

#lang-flag {
    cursor: pointer;
    font-size: 14px;
    line-height: 1;
    user-select: none;
    opacity: 0.85;
    transition: opacity 0.2s;
}

#lang-flag:hover { opacity: 1; }

#lang-dropdown {
    position: absolute;
    top: 100%;
    left: 0;
    background: #1a1a2e;
    border: 1px solid rgba(255,255,255,0.15);
    border-radius: 4px;
    z-index: 1000;
    min-width: 130px;
    box-shadow: 0 4px 12px rgba(0,0,0,0.4);
    overflow: hidden;
}

.lang-option {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 6px 10px;
    cursor: pointer;
    font-size: 12px;
    color: rgba(255,255,255,0.75);
    white-space: nowrap;
    transition: background 0.15s;
}

.lang-option:hover { background: rgba(255,255,255,0.08); }

.lang-option.active { color: #fff; background: rgba(255,255,255,0.05); }

.tabs button {
    background-color: transparent;
    float: left;
    border: none;
    outline: none;
    cursor: pointer;
    padding: 16px 20px;
    transition: all 0.3s ease;
    color: #b0b0b0;
    font-weight: 500;
}

.tabs button:hover {
    background-color: rgba(255, 255, 255, 0.1);
    color: #ffffff;
}

.tabs button.active {
    background-color: rgba(90, 158, 238, 0.2);
    color: #5a9eee;
}

.tab-content {
    display: none;
    padding: 25px;
    border-top: none;
    background: rgba(255, 255, 255, 0.02);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-top: none;
    max-width: 820px;
    margin: auto;
    border-radius: 0 0 15px 15px;
    backdrop-filter: blur(5px);
}

.status-message {
    text-align: center;
    padding: 15px;
    margin: 20px auto;
    border-radius: 8px;
    max-width: 800px;
    backdrop-filter: blur(10px);
}

.status-ok {
    background: rgba(76, 175, 80, 0.2);
    border: 1px solid rgba(76, 175, 80, 0.3);
    color: #81c784;
}

.status-err {
    background: rgba(244, 67, 54, 0.2);
    border: 1px solid rgba(244, 67, 54, 0.3);
    color: #e57373;
}

code {
    background: rgba(255, 255, 255, 0.1);
    border-radius: 4px;
    font-family: 'SF Mono', monospace;
    padding: 4px 8px;
    color: #5a9eee;
}

.modal {
    display: none;
    position: fixed;
    z-index: 1000;
    left: 0;
    top: 0;
    width: 100%;
    height: 100%;
    background-color: rgba(0, 0, 0, 0.7);
    backdrop-filter: blur(5px);
}

.modal-content {
    background: rgba(255, 255, 255, 0.05);
    backdrop-filter: blur(15px);
    border: 1px solid rgba(255, 255, 255, 0.1);
    margin: 10% auto;
    padding: 30px;
    width: 90%;
    max-width: 500px;
    border-radius: 15px;
    box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.5);
    color: #e0e0e0;
}

.close {
    color: #b0b0b0;
    float: right;
    font-size: 28px;
    font-weight: bold;
    cursor: pointer;
    transition: color 0.3s ease;
}

.close:hover {
    color: #ffffff;
}

progress {
    width: 100%;
    height: 8px;
    border-radius: 4px;
    background: rgba(255, 255, 255, 0.1);
    border: none;
}

progress::-webkit-progress-bar {
    background: rgba(255, 255, 255, 0.1);
    border-radius: 4px;
}

progress::-webkit-progress-value {
    background: linear-gradient(90deg, #5a9eee, #4a8bdb);
    border-radius: 4px;
}

.checkbox-label {
    display: flex;
    align-items: center;
    margin: 15px 0;
    color: #b0b0b0;
}

.checkbox-label input[type="checkbox"] {
    margin-right: 10px;
    width: auto;
}


.password-criteria {
    list-style: none;
    padding: 0;
    margin: 10px 0 15px 0;
    text-align: left;
    font-size: 0.8rem;
}

.password-criteria li {
    color: #e57373;
    margin-bottom: 6px;
    transition: color 0.3s ease;
}

.password-criteria li.valid {
    color: #81c784;
}


#password-confirm-message {
    font-size: 0.8rem;
    margin: 10px 0 15px 0;
    height: 1rem;
    text-align: left;
}

.password-match { 
    color: #81c784; 
}

.password-no-match { 
    color: #e57373; 
}

.login-display-container {
    text-align: center;
    margin-bottom: 15px;
    font-size: 1rem;
    color: #b0b0b0;
}
.login-display-container strong {
    color: #5a9eee;
    font-weight: 500;
}
.modern-hr {
    border: 0;
    height: 1px;
    background: rgba(255, 255, 255, 0.1);
    margin: 20px 0;
}

.info-box {
    background: rgba(90, 158, 238, 0.1);
    border-left: 4px solid #5a9eee;
    padding: 15px;
    margin-top: 20px;
    border-radius: 0 8px 8px 0;
    font-size: 0.9rem;
}
.info-box h5 {
    margin-top: 0;
    color: #ffffff;
    font-weight: 500;
}
.info-box ul {
    list-style-type: none;
    padding-left: 0;
}
.info-box li {
    margin-bottom: 8px;
}
.info-box code {
    display: block;
    white-space: pre-wrap;
    word-wrap: break-word;
    padding: 10px;
    margin-top: 10px;
    background: rgba(0, 0, 0, 0.3);
    border-radius: 5px;
    font-size: 0.85rem;
}

/* Password visibility toggle */
.password-input-container {
    position: relative;
    display: inline-block;
    width: 100%;
}

.password-toggle {
    position: absolute;
    right: 12px;
    top: 50%;
    transform: translateY(-50%);
    cursor: pointer;
    color: #b0b0b0;
    font-size: 1.2rem;
    user-select: none;
    transition: color 0.3s ease;
}

.password-toggle:hover {
    color: #5a9eee;
}

/* Password Type Toggle Switch */
.password-type-selector {
    margin-bottom: 25px;
    text-align: center;
}

.toggle-container {
    display: inline-flex;
    align-items: center;
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 50px;
    padding: 4px;
    position: relative;
    backdrop-filter: blur(10px);
}

.toggle-option {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 12px 20px;
    border-radius: 50px;
    cursor: pointer;
    transition: all 0.3s ease;
    color: #b0b0b0;
    font-weight: 500;
    position: relative;
    z-index: 2;
}

.toggle-option.active {
    color: #ffffff;
}

.toggle-option.web-active {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    box-shadow: 0 4px 15px rgba(102, 126, 234, 0.3);
}

.toggle-option.wifi-active {
    background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
    box-shadow: 0 4px 15px rgba(240, 147, 251, 0.3);
}

.toggle-icon {
    font-size: 1.1em;
    opacity: 0.8;
}

.toggle-option.active .toggle-icon {
    opacity: 1;
}

.password-form-title {
    text-align: center;
    margin-bottom: 20px;
    color: #ffffff;
    font-weight: 400;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 10px;
}

.password-form-title .title-icon {
    font-size: 1.2em;
    opacity: 0.8;
}

.password-type-description {
    text-align: center;
    margin-bottom: 20px;
    padding: 12px;
    background: rgba(90, 158, 238, 0.1);
    border-left: 4px solid #5a9eee;
    border-radius: 0 8px 8px 0;
    font-size: 0.9rem;
    color: #b0b0b0;
}

.password-generate {
    position: absolute;
    right: 40px;
    top: 50%;
    transform: translateY(-50%);
    cursor: pointer;
    color: #b0b0b0;
    font-size: 1.2rem;
    user-select: none;
    transition: color 0.3s ease;
    font-weight: bold;
}

.password-generate:hover {
    color: #5a9eee;
}

/* Password Strength Indicator */
.password-strength-container {
    margin: 15px 0;
}

.password-strength-bar {
    width: 100%;
    height: 8px;
    background: rgba(255, 255, 255, 0.1);
    border-radius: 4px;
    overflow: hidden;
    margin: 8px 0;
}

.password-strength-fill {
    height: 100%;
    width: 0%;
    border-radius: 4px;
    transition: all 0.3s ease;
}

.strength-weak .password-strength-fill {
    background: linear-gradient(90deg, #e74c3c, #c0392b);
}

.strength-medium .password-strength-fill {
    background: linear-gradient(90deg, #f39c12, #e67e22);
}

.strength-strong .password-strength-fill {
    background: linear-gradient(90deg, #27ae60, #2ecc71);
}

.strength-encryption .password-strength-fill {
    background: linear-gradient(90deg, #8e44ad, #9b59b6);
}

.password-strength-text {
    font-size: 0.9rem;
    font-weight: 500;
    text-align: center;
    margin-top: 5px;
}

.strength-weak .password-strength-text {
    color: #e74c3c;
}

.strength-medium .password-strength-text {
    color: #f39c12;
}

.strength-strong .password-strength-text {
    color: #27ae60;
}

.strength-encryption .password-strength-text {
    color: #8e44ad;
}

/* Drag and Drop Styles */
.draggable-row {
    cursor: move;
    cursor: grab;
    transition: all 0.2s ease;
}

.draggable-row:active {
    cursor: grabbing;
}

.draggable-row.dragging {
    opacity: 0.5;
    transform: scale(1.02);
    box-shadow: 0 5px 15px rgba(90, 158, 238, 0.3);
    background: rgba(90, 158, 238, 0.1);
}

.drop-zone {
    border: 2px dashed rgba(90, 158, 238, 0.5);
    background: rgba(90, 158, 238, 0.05);
}

/* Copy notification styles */
.copy-notification {
    position: fixed;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    background: linear-gradient(135deg, #4CAF50, #45a049);
    color: white;
    padding: 20px 30px;
    border-radius: 12px;
    font-size: 1.1em;
    font-weight: 500;
    box-shadow: 0 8px 32px rgba(76, 175, 80, 0.3);
    z-index: 10000;
    opacity: 0;
    scale: 0.8;
    transition: all 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
    backdrop-filter: blur(10px);
    border: 1px solid rgba(255, 255, 255, 0.2);
}

.copy-notification.show {
    opacity: 1;
    scale: 1;
}

}

.drag-handle {
    display: inline-block;
    width: 20px;
    text-align: center;
    color: #b0b0b0;
    cursor: grab;
    font-size: 1.2rem;
    transition: color 0.3s ease;
}

.drag-handle:hover {
    color: #5a9eee;
}

.drag-handle:active {
    cursor: grabbing;
}

/* Mobile touch improvements */
@media (pointer: coarse) {
    .draggable-row {
        cursor: default;
    }
    
    .drag-handle {
        cursor: default;
        padding: 10px 5px;
        font-size: 1.4rem;
    }
}

/* Responsive design */
@media (max-width: 768px) {
    .form-container, .content-box, .tabs, .tab-content {
        margin: 10px;
        padding: 20px;
    }

    #widget-bar {
        margin: 0 10px;
    }
    
    .tabs button {
        padding: 12px 16px;
        font-size: 0.9rem;
    }

    /* Table container with horizontal scroll */
    .content-box {
        overflow-x: auto;
        -webkit-overflow-scrolling: touch;
    }

    table {
        min-width: 250px; /* Минимальная ширина для удобства */
        width: 100%;
        font-size: 0.9rem;
        table-layout: fixed; /* Фиксированная разметка для контроля ширины колонок */
    }

    /* Keys table column widths for all screens */
    #keys-table th:nth-child(1), #keys-table td:nth-child(1) { width: 30px; } /* Drag */
    #keys-table th:nth-child(2), #keys-table td:nth-child(2) { width: 120px; } /* Name */
    #keys-table th:nth-child(3), #keys-table td:nth-child(3) { width: 100px; } /* Code */
    #keys-table th:nth-child(4), #keys-table td:nth-child(4) { width: 60px; } /* Timer */
    #keys-table th:nth-child(5), #keys-table td:nth-child(5) { width: 80px; } /* Progress */
    #keys-table th:nth-child(6), #keys-table td:nth-child(6) { width: 100px; } /* Actions */

    /* Passwords table column widths for all screens */
    #passwords-table th:nth-child(1), #passwords-table td:nth-child(1) { width: 30px; } /* Drag */
    #passwords-table th:nth-child(2), #passwords-table td:nth-child(2) { width: 200px; } /* Name */
    #passwords-table th:nth-child(3), #passwords-table td:nth-child(3) { width: 150px; } /* Actions */

    @media (max-width: 768px) {
        #passwords-table td:nth-child(3) {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            gap: 3px;
            width: 100%;
            box-sizing: border-box;
        }
    }

    th, td {
        padding: 12px 8px;
        white-space: nowrap;
    }

    /* Keys table mobile optimization */
    #keys-table th:nth-child(5), /* Progress */
    #keys-table td:nth-child(5) {
        display: none;
    }

    #keys-table th:nth-child(2), /* Name */
    #keys-table td:nth-child(2) {
        width: 50px !important;
        overflow: hidden;
        text-overflow: ellipsis;
    }

    #keys-table th:nth-child(3), /* Code */
    #keys-table td:nth-child(3) {
        width: 70px !important;
        font-size: 1.1rem;
    }

    #keys-table th:nth-child(4), /* Timer */
    #keys-table td:nth-child(4) {
        width: 35px !important;
    }

    #keys-table th:nth-child(6), /* Actions */
    #keys-table td:nth-child(6) {
        width: 50px !important;
    }

    #keys-table th:nth-child(6),
    #keys-table td:nth-child(6) {
        width: auto !important;
        white-space: nowrap;
        padding: 2px 4px;
    }

    #keys-table td:nth-child(6) {
        text-align: center;
        vertical-align: middle;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        gap: 3px;
    }

    #keys-table .button-delete,
    #keys-table .button-action {
        width: 26px;
        height: 26px;
        padding: 0;
        border-radius: 4px;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        margin: 0;
        font-size: 0;
        min-width: 26px;
        min-height: 26px;
        flex-shrink: 0;
    }

    #keys-table .button-delete::before {
        content: "🗑️";
        font-size: 0.85rem;
        line-height: 1;
    }

    #keys-table .button-qr::before {
        content: "📱";
        font-size: 0.85rem;
        line-height: 1;
    }

    #keys-table .button-hotp::before {
        content: "🔄";
        font-size: 0.85rem;
        line-height: 1;
    }

    /* Passwords table mobile optimization */
    #passwords-table th:nth-child(2), /* Name */
    #passwords-table td:nth-child(2) {
        width: 150px !important;
        overflow: hidden;
        text-overflow: ellipsis;
    }

    #passwords-table .button-copy,
    #passwords-table .button-action,
    #passwords-table .button-delete {
        width: 26px;
        height: 26px;
        padding: 0;
        border-radius: 4px;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        margin: 0 2px;
        font-size: 0;
        min-width: 26px;
        min-height: 26px;
        flex-shrink: 0;
    }

    #passwords-table .button-copy::before {
        content: "📋";
        font-size: 0.85rem;
        line-height: 1;
    }

    #passwords-table .button-action::before {
        content: "✏️";
        font-size: 0.85rem;
        line-height: 1;
    }

    #passwords-table .button-delete::before {
        content: "🗑️";
        font-size: 0.85rem;
        line-height: 1;
    }
}

/* Extra small screens (phones in portrait) */
@media (max-width: 480px) {
    .form-container, .content-box, .tabs, .tab-content {
        margin: 5px;
        padding: 15px;
    }

    table {
        min-width: 300px;
        font-size: 0.85rem;
    }

    th, td {
        padding: 10px 6px;
    }

    #keys-table td.code {
        font-size: 1.1rem;
    }

    /* Stack action buttons vertically on very small screens */
    #passwords-table th:nth-child(3),
    #passwords-table td:nth-child(3) {
        width: 100px;
    }

    #passwords-table .button-copy,
    #passwords-table .button-action,
    #passwords-table .button-delete {
        width: 36px;
        height: 36px;
        padding: 0;
        margin: 1px;
        border-radius: 6px;
    }
}

/* QR Scanner Styles */
#qr-status.success {
    background: rgba(76, 175, 80, 0.2);
    border: 1px solid rgba(76, 175, 80, 0.5);
    color: #4caf50;
}

#qr-status.error {
    background: rgba(244, 67, 54, 0.2);
    border: 1px solid rgba(244, 67, 54, 0.5);
    color: #f44336;
}

#scan-qr-file:hover {
    background: rgba(90, 158, 238, 0.4);
    transform: translateY(-50%) scale(1.1);
}

#scan-qr-file:active {
    transform: translateY(-50%) scale(0.95);
}

/* Radio button styling improvements */
input[type="radio"] {
    accent-color: #5a9eee;
    cursor: pointer;
}

input[type="radio"]:checked + span {
    color: #ffffff;
    font-weight: 500;
}

/* Select dropdown improvements */
select {
    cursor: pointer;
    appearance: none;
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 12 12'%3E%3Cpath fill='%23e0e0e0' d='M6 9L1 4h10z'/%3E%3C/svg%3E");
    background-repeat: no-repeat;
    background-position: right 12px center;
    padding-right: 35px;
}

/* Number input styling */
input[type="number"] {
    -moz-appearance: textfield;
}

input[type="number"]::-webkit-inner-spin-button,
input[type="number"]::-webkit-outer-spin-button {
    -webkit-appearance: none;
    margin: 0;
}

/* Small helper text */
small {
    font-size: 0.85rem;
    line-height: 1.4;
}

</style>
<script>
/* elliptic 6.5.4 - P-256 ECDH - browser wrapper */
var module = { exports: {} };
var exports = module.exports;
!function(e){"object"==typeof exports&&"undefined"!=typeof module?module.exports=e():"function"==typeof define&&define.amd?define([],e):("undefined"!=typeof window?window:"undefined"!=typeof global?global:"undefined"!=typeof self?self:this).elliptic=e()}(function(){return function r(f,d,n){function a(t,e){if(!d[t]){if(!f[t]){var i="function"==typeof require&&require;if(!e&&i)return i(t,!0);if(s)return s(t,!0);throw(i=new Error("Cannot find module '"+t+"'")).code="MODULE_NOT_FOUND",i}i=d[t]={exports:{}},f[t][0].call(i.exports,function(e){return a(f[t][1][e]||e)},i,i.exports,r,f,d,n)}return d[t].exports}for(var s="function"==typeof require&&require,e=0;e<n.length;e++)a(n[e]);return a}({1:[function(e,t,i){"use strict";i.version=e("../package.json").version,i.utils=e("./elliptic/utils"),i.rand=e("brorand"),i.curve=e("./elliptic/curve"),i.curves=e("./elliptic/curves"),i.ec=e("./elliptic/ec"),i.eddsa=e("./elliptic/eddsa")},{"../package.json":35,"./elliptic/curve":4,"./elliptic/curves":7,"./elliptic/ec":8,"./elliptic/eddsa":11,"./elliptic/utils":15,brorand:17}],2:[function(e,t,i){"use strict";var r=e("bn.js"),f=e("../utils"),x=f.getNAF,I=f.getJSF,o=f.assert;function d(e,t){this.type=e,this.p=new r(t.p,16),this.red=t.prime?r.red(t.prime):r.mont(this.p),this.zero=new r(0).toRed(this.red),this.one=new r(1).toRed(this.red),this.two=new r(2).toRed(this.red),this.n=t.n&&new r(t.n,16),this.g=t.g&&this.pointFromJSON(t.g,t.gRed),this._wnafT1=new Array(4),this._wnafT2=new Array(4),this._wnafT3=new Array(4),this._wnafT4=new Array(4),this._bitLength=this.n?this.n.bitLength():0;t=this.n&&this.p.div(this.n);!t||0<t.cmpn(100)?this.redN=null:(this._maxwellTrick=!0,this.redN=this.n.toRed(this.red))}function n(e,t){this.curve=e,this.type=t,this.precomputed=null}(t.exports=d).prototype.point=function(){throw new Error("Not implemented")},d.prototype.validate=function(){throw new Error("Not implemented")},d.prototype._fixedNafMul=function(e,t){o(e.precomputed);var i=e._getDoubles(),r=x(t,1,this._bitLength),t=(1<<i.step+1)-(i.step%2==0?2:1);t/=3;for(var f,d=[],n=0;n<r.length;n+=i.step){f=0;for(var a=n+i.step-1;n<=a;a--)f=(f<<1)+r[a];d.push(f)}for(var s=this.jpoint(null,null,null),c=this.jpoint(null,null,null),h=t;0<h;h--){for(n=0;n<d.length;n++)(f=d[n])===h?c=c.mixedAdd(i.points[n]):f===-h&&(c=c.mixedAdd(i.points[n].neg()));s=s.add(c)}return s.toP()},d.prototype._wnafMul=function(e,t){for(var i=e._getNAFPoints(4),r=i.wnd,f=i.points,d=x(t,r,this._bitLength),n=this.jpoint(null,null,null),a=d.length-1;0<=a;a--){for(var s=0;0<=a&&0===d[a];a--)s++;if(0<=a&&s++,n=n.dblp(s),a<0)break;var c=d[a];o(0!==c),n="affine"===e.type?0<c?n.mixedAdd(f[c-1>>1]):n.mixedAdd(f[-c-1>>1].neg()):0<c?n.add(f[c-1>>1]):n.add(f[-c-1>>1].neg())}return"affine"===e.type?n.toP():n},d.prototype._wnafMulAdd=function(e,t,i,r,f){for(var d,n=this._wnafT1,a=this._wnafT2,s=this._wnafT3,c=0,h=0;h<r;h++){var o=(d=t[h])._getNAFPoints(e);n[h]=o.wnd,a[h]=o.points}for(h=r-1;1<=h;h-=2){var u=h-1,b=h;if(1===n[u]&&1===n[b]){var l=[t[u],null,null,t[b]];0===t[u].y.cmp(t[b].y)?(l[1]=t[u].add(t[b]),l[2]=t[u].toJ().mixedAdd(t[b].neg())):0===t[u].y.cmp(t[b].y.redNeg())?(l[1]=t[u].toJ().mixedAdd(t[b]),l[2]=t[u].add(t[b].neg())):(l[1]=t[u].toJ().mixedAdd(t[b]),l[2]=t[u].toJ().mixedAdd(t[b].neg()));var p=[-3,-1,-5,-7,0,7,5,1,3],m=I(i[u],i[b]),c=Math.max(m[0].length,c);for(s[u]=new Array(c),s[b]=new Array(c),_=0;_<c;_++){var v=0|m[0][_],g=0|m[1][_];s[u][_]=p[3*(1+v)+(1+g)],s[b][_]=0,a[u]=l}}else s[u]=x(i[u],n[u],this._bitLength),s[b]=x(i[b],n[b],this._bitLength),c=Math.max(s[u].length,c),c=Math.max(s[b].length,c)}var y=this.jpoint(null,null,null),M=this._wnafT4;for(h=c;0<=h;h--){for(var w=0;0<=h;){for(var S=!0,_=0;_<r;_++)M[_]=0|s[_][h],0!==M[_]&&(S=!1);if(!S)break;w++,h--}if(0<=h&&w++,y=y.dblp(w),h<0)break;for(_=0;_<r;_++){var A=M[_];0!==A&&(0<A?d=a[_][A-1>>1]:A<0&&(d=a[_][-A-1>>1].neg()),y="affine"===d.type?y.mixedAdd(d):y.add(d))}}for(h=0;h<r;h++)a[h]=null;return f?y:y.toP()},(d.BasePoint=n).prototype.eq=function(){throw new Error("Not implemented")},n.prototype.validate=function(){return this.curve.validate(this)},d.prototype.decodePoint=function(e,t){e=f.toArray(e,t);t=this.p.byteLength();if((4===e[0]||6===e[0]||7===e[0])&&e.length-1==2*t)return 6===e[0]?o(e[e.length-1]%2==0):7===e[0]&&o(e[e.length-1]%2==1),this.point(e.slice(1,1+t),e.slice(1+t,1+2*t));if((2===e[0]||3===e[0])&&e.length-1===t)return this.pointFromX(e.slice(1,1+t),3===e[0]);throw new Error("Unknown point format")},n.prototype.encodeCompressed=function(e){return this.encode(e,!0)},n.prototype._encode=function(e){var t=this.curve.p.byteLength(),i=this.getX().toArray("be",t);return e?[this.getY().isEven()?2:3].concat(i):[4].concat(i,this.getY().toArray("be",t))},n.prototype.encode=function(e,t){return f.encode(this._encode(t),e)},n.prototype.precompute=function(e){if(this.precomputed)return this;var t={doubles:null,naf:null,beta:null};return t.naf=this._getNAFPoints(8),t.doubles=this._getDoubles(4,e),t.beta=this._getBeta(),this.precomputed=t,this},n.prototype._hasDoubles=function(e){if(!this.precomputed)return!1;var t=this.precomputed.doubles;return!!t&&t.points.length>=Math.ceil((e.bitLength()+1)/t.step)},n.prototype._getDoubles=function(e,t){if(this.precomputed&&this.precomputed.doubles)return this.precomputed.doubles;for(var i=[this],r=this,f=0;f<t;f+=e){for(var d=0;d<e;d++)r=r.dbl();i.push(r)}return{step:e,points:i}},n.prototype._getNAFPoints=function(e){if(this.precomputed&&this.precomputed.naf)return this.precomputed.naf;for(var t=[this],i=(1<<e)-1,r=1==i?null:this.dbl(),f=1;f<i;f++)t[f]=t[f-1].add(r);return{wnd:e,points:t}},n.prototype._getBeta=function(){return null},n.prototype.dblp=function(e){for(var t=this,i=0;i<e;i++)t=t.dbl();return t}},{"../utils":15,"bn.js":16}],3:[function(e,t,i){"use strict";var r=e("../utils"),d=e("bn.js"),f=e("inherits"),n=e("./base"),a=r.assert;function s(e){this.twisted=1!=(0|e.a),this.mOneA=this.twisted&&-1==(0|e.a),this.extended=this.mOneA,n.call(this,"edwards",e),this.a=new d(e.a,16).umod(this.red.m),this.a=this.a.toRed(this.red),this.c=new d(e.c,16).toRed(this.red),this.c2=this.c.redSqr(),this.d=new d(e.d,16).toRed(this.red),this.dd=this.d.redAdd(this.d),a(!this.twisted||0===this.c.fromRed().cmpn(1)),this.oneC=1==(0|e.c)}function c(e,t,i,r,f){n.BasePoint.call(this,e,"projective"),null===t&&null===i&&null===r?(this.x=this.curve.zero,this.y=this.curve.one,this.z=this.curve.one,this.t=this.curve.zero,this.zOne=!0):(this.x=new d(t,16),this.y=new d(i,16),this.z=r?new d(r,16):this.curve.one,this.t=f&&new d(f,16),this.x.red||(this.x=this.x.toRed(this.curve.red)),this.y.red||(this.y=this.y.toRed(this.curve.red)),this.z.red||(this.z=this.z.toRed(this.curve.red)),this.t&&!this.t.red&&(this.t=this.t.toRed(this.curve.red)),this.zOne=this.z===this.curve.one,this.curve.extended&&!this.t&&(this.t=this.x.redMul(this.y),this.zOne||(this.t=this.t.redMul(this.z.redInvm()))))}f(s,n),(t.exports=s).prototype._mulA=function(e){return this.mOneA?e.redNeg():this.a.redMul(e)},s.prototype._mulC=function(e){return this.oneC?e:this.c.redMul(e)},s.prototype.jpoint=function(e,t,i,r){return this.point(e,t,i,r)},s.prototype.pointFromX=function(e,t){var i=(e=!(e=new d(e,16)).red?e.toRed(this.red):e).redSqr(),r=this.c2.redSub(this.a.redMul(i)),i=this.one.redSub(this.c2.redMul(this.d).redMul(i)),r=r.redMul(i.redInvm()),i=r.redSqrt();if(0!==i.redSqr().redSub(r).cmp(this.zero))throw new Error("invalid point");r=i.fromRed().isOdd();return(t&&!r||!t&&r)&&(i=i.redNeg()),this.point(e,i)},s.prototype.pointFromY=function(e,t){var i=(e=!(e=new d(e,16)).red?e.toRed(this.red):e).redSqr(),r=i.redSub(this.c2),i=i.redMul(this.d).redMul(this.c2).redSub(this.a),r=r.redMul(i.redInvm());if(0===r.cmp(this.zero)){if(t)throw new Error("invalid point");return this.point(this.zero,e)}i=r.redSqrt();if(0!==i.redSqr().redSub(r).cmp(this.zero))throw new Error("invalid point");return i.fromRed().isOdd()!==t&&(i=i.redNeg()),this.point(i,e)},s.prototype.validate=function(e){if(e.isInfinity())return!0;e.normalize();var t=e.x.redSqr(),i=e.y.redSqr(),e=t.redMul(this.a).redAdd(i),i=this.c2.redMul(this.one.redAdd(this.d.redMul(t).redMul(i)));return 0===e.cmp(i)},f(c,n.BasePoint),s.prototype.pointFromJSON=function(e){return c.fromJSON(this,e)},s.prototype.point=function(e,t,i,r){return new c(this,e,t,i,r)},c.fromJSON=function(e,t){return new c(e,t[0],t[1],t[2])},c.prototype.inspect=function(){return this.isInfinity()?"<EC Point Infinity>":"<EC Point x: "+this.x.fromRed().toString(16,2)+" y: "+this.y.fromRed().toString(16,2)+" z: "+this.z.fromRed().toString(16,2)+">"},c.prototype.isInfinity=function(){return 0===this.x.cmpn(0)&&(0===this.y.cmp(this.z)||this.zOne&&0===this.y.cmp(this.curve.c))},c.prototype._extDbl=function(){var e=this.x.redSqr(),t=this.y.redSqr(),i=(i=this.z.redSqr()).redIAdd(i),r=this.curve._mulA(e),f=this.x.redAdd(this.y).redSqr().redISub(e).redISub(t),d=r.redAdd(t),e=d.redSub(i),i=r.redSub(t),r=f.redMul(e),t=d.redMul(i),i=f.redMul(i),d=e.redMul(d);return this.curve.point(r,t,d,i)},c.prototype._projDbl=function(){var e,t,i,r,f,d,n=this.x.redAdd(this.y).redSqr(),a=this.x.redSqr(),s=this.y.redSqr();return d=this.curve.twisted?(f=(i=this.curve._mulA(a)).redAdd(s),this.zOne?(e=n.redSub(a).redSub(s).redMul(f.redSub(this.curve.two)),t=f.redMul(i.redSub(s)),f.redSqr().redSub(f).redSub(f)):(r=this.z.redSqr(),d=f.redSub(r).redISub(r),e=n.redSub(a).redISub(s).redMul(d),t=f.redMul(i.redSub(s)),f.redMul(d))):(i=a.redAdd(s),r=this.curve._mulC(this.z).redSqr(),d=i.redSub(r).redSub(r),e=this.curve._mulC(n.redISub(i)).redMul(d),t=this.curve._mulC(i).redMul(a.redISub(s)),i.redMul(d)),this.curve.point(e,t,d)},c.prototype.dbl=function(){return this.isInfinity()?this:this.curve.extended?this._extDbl():this._projDbl()},c.prototype._extAdd=function(e){var t=this.y.redSub(this.x).redMul(e.y.redSub(e.x)),i=this.y.redAdd(this.x).redMul(e.y.redAdd(e.x)),r=this.t.redMul(this.curve.dd).redMul(e.t),f=this.z.redMul(e.z.redAdd(e.z)),d=i.redSub(t),e=f.redSub(r),f=f.redAdd(r),r=i.redAdd(t),i=d.redMul(e),t=f.redMul(r),r=d.redMul(r),f=e.redMul(f);return this.curve.point(i,t,f,r)},c.prototype._projAdd=function(e){var t,i=this.z.redMul(e.z),r=i.redSqr(),f=this.x.redMul(e.x),d=this.y.redMul(e.y),n=this.curve.d.redMul(f).redMul(d),a=r.redSub(n),n=r.redAdd(n),e=this.x.redAdd(this.y).redMul(e.x.redAdd(e.y)).redISub(f).redISub(d),e=i.redMul(a).redMul(e),n=this.curve.twisted?(t=i.redMul(n).redMul(d.redSub(this.curve._mulA(f))),a.redMul(n)):(t=i.redMul(n).redMul(d.redSub(f)),this.curve._mulC(a).redMul(n));return this.curve.point(e,t,n)},c.prototype.add=function(e){return this.isInfinity()?e:e.isInfinity()?this:this.curve.extended?this._extAdd(e):this._projAdd(e)},c.prototype.mul=function(e){return this._hasDoubles(e)?this.curve._fixedNafMul(this,e):this.curve._wnafMul(this,e)},c.prototype.mulAdd=function(e,t,i){return this.curve._wnafMulAdd(1,[this,t],[e,i],2,!1)},c.prototype.jmulAdd=function(e,t,i){return this.curve._wnafMulAdd(1,[this,t],[e,i],2,!0)},c.prototype.normalize=function(){if(this.zOne)return this;var e=this.z.redInvm();return this.x=this.x.redMul(e),this.y=this.y.redMul(e),this.t&&(this.t=this.t.redMul(e)),this.z=this.curve.one,this.zOne=!0,this},c.prototype.neg=function(){return this.curve.point(this.x.redNeg(),this.y,this.z,this.t&&this.t.redNeg())},c.prototype.getX=function(){return this.normalize(),this.x.fromRed()},c.prototype.getY=function(){return this.normalize(),this.y.fromRed()},c.prototype.eq=function(e){return this===e||0===this.getX().cmp(e.getX())&&0===this.getY().cmp(e.getY())},c.prototype.eqXToP=function(e){var t=e.toRed(this.curve.red).redMul(this.z);if(0===this.x.cmp(t))return!0;for(var i=e.clone(),r=this.curve.redN.redMul(this.z);;){if(i.iadd(this.curve.n),0<=i.cmp(this.curve.p))return!1;if(t.redIAdd(r),0===this.x.cmp(t))return!0}},c.prototype.toP=c.prototype.normalize,c.prototype.mixedAdd=c.prototype.add},{"../utils":15,"./base":2,"bn.js":16,inherits:32}],4:[function(e,t,i){"use strict";i.base=e("./base"),i.short=e("./short"),i.mont=e("./mont"),i.edwards=e("./edwards")},{"./base":2,"./edwards":3,"./mont":5,"./short":6}],5:[function(e,t,i){"use strict";var r=e("bn.js"),f=e("inherits"),d=e("./base"),n=e("../utils");function a(e){d.call(this,"mont",e),this.a=new r(e.a,16).toRed(this.red),this.b=new r(e.b,16).toRed(this.red),this.i4=new r(4).toRed(this.red).redInvm(),this.two=new r(2).toRed(this.red),this.a24=this.i4.redMul(this.a.redAdd(this.two))}function s(e,t,i){d.BasePoint.call(this,e,"projective"),null===t&&null===i?(this.x=this.curve.one,this.z=this.curve.zero):(this.x=new r(t,16),this.z=new r(i,16),this.x.red||(this.x=this.x.toRed(this.curve.red)),this.z.red||(this.z=this.z.toRed(this.curve.red)))}f(a,d),(t.exports=a).prototype.validate=function(e){var t=e.normalize().x,e=t.redSqr(),t=e.redMul(t).redAdd(e.redMul(this.a)).redAdd(t);return 0===t.redSqrt().redSqr().cmp(t)},f(s,d.BasePoint),a.prototype.decodePoint=function(e,t){return this.point(n.toArray(e,t),1)},a.prototype.point=function(e,t){return new s(this,e,t)},a.prototype.pointFromJSON=function(e){return s.fromJSON(this,e)},s.prototype.precompute=function(){},s.prototype._encode=function(){return this.getX().toArray("be",this.curve.p.byteLength())},s.fromJSON=function(e,t){return new s(e,t[0],t[1]||e.one)},s.prototype.inspect=function(){return this.isInfinity()?"<EC Point Infinity>":"<EC Point x: "+this.x.fromRed().toString(16,2)+" z: "+this.z.fromRed().toString(16,2)+">"},s.prototype.isInfinity=function(){return 0===this.z.cmpn(0)},s.prototype.dbl=function(){var e=this.x.redAdd(this.z).redSqr(),t=this.x.redSub(this.z).redSqr(),i=e.redSub(t),e=e.redMul(t),i=i.redMul(t.redAdd(this.curve.a24.redMul(i)));return this.curve.point(e,i)},s.prototype.add=function(){throw new Error("Not supported on Montgomery curve")},s.prototype.diffAdd=function(e,t){var i=this.x.redAdd(this.z),r=this.x.redSub(this.z),f=e.x.redAdd(e.z),i=e.x.redSub(e.z).redMul(i),f=f.redMul(r),r=t.z.redMul(i.redAdd(f).redSqr()),f=t.x.redMul(i.redISub(f).redSqr());return this.curve.point(r,f)},s.prototype.mul=function(e){for(var t=e.clone(),i=this,r=this.curve.point(null,null),f=[];0!==t.cmpn(0);t.iushrn(1))f.push(t.andln(1));for(var d=f.length-1;0<=d;d--)0===f[d]?(i=i.diffAdd(r,this),r=r.dbl()):(r=i.diffAdd(r,this),i=i.dbl());return r},s.prototype.mulAdd=function(){throw new Error("Not supported on Montgomery curve")},s.prototype.jumlAdd=function(){throw new Error("Not supported on Montgomery curve")},s.prototype.eq=function(e){return 0===this.getX().cmp(e.getX())},s.prototype.normalize=function(){return this.x=this.x.redMul(this.z.redInvm()),this.z=this.curve.one,this},s.prototype.getX=function(){return this.normalize(),this.x.fromRed()}},{"../utils":15,"./base":2,"bn.js":16,inherits:32}],6:[function(e,t,i){"use strict";var r=e("../utils"),y=e("bn.js"),f=e("inherits"),d=e("./base"),n=r.assert;function a(e){d.call(this,"short",e),this.a=new y(e.a,16).toRed(this.red),this.b=new y(e.b,16).toRed(this.red),this.tinv=this.two.redInvm(),this.zeroA=0===this.a.fromRed().cmpn(0),this.threeA=0===this.a.fromRed().sub(this.p).cmpn(-3),this.endo=this._getEndomorphism(e),this._endoWnafT1=new Array(4),this._endoWnafT2=new Array(4)}function s(e,t,i,r){d.BasePoint.call(this,e,"affine"),null===t&&null===i?(this.x=null,this.y=null,this.inf=!0):(this.x=new y(t,16),this.y=new y(i,16),r&&(this.x.forceRed(this.curve.red),this.y.forceRed(this.curve.red)),this.x.red||(this.x=this.x.toRed(this.curve.red)),this.y.red||(this.y=this.y.toRed(this.curve.red)),this.inf=!1)}function c(e,t,i,r){d.BasePoint.call(this,e,"jacobian"),null===t&&null===i&&null===r?(this.x=this.curve.one,this.y=this.curve.one,this.z=new y(0)):(this.x=new y(t,16),this.y=new y(i,16),this.z=new y(r,16)),this.x.red||(this.x=this.x.toRed(this.curve.red)),this.y.red||(this.y=this.y.toRed(this.curve.red)),this.z.red||(this.z=this.z.toRed(this.curve.red)),this.zOne=this.z===this.curve.one}f(a,d),(t.exports=a).prototype._getEndomorphism=function(e){var t,i,r;if(this.zeroA&&this.g&&this.n&&1===this.p.modn(3))return i=(e.beta?new y(e.beta,16):i=(r=this._getEndoRoots(this.p))[0].cmp(r[1])<0?r[0]:r[1]).toRed(this.red),e.lambda?t=new y(e.lambda,16):(r=this._getEndoRoots(this.n),0===this.g.mul(r[0]).x.cmp(this.g.x.redMul(i))?t=r[0]:(t=r[1],n(0===this.g.mul(t).x.cmp(this.g.x.redMul(i))))),{beta:i,lambda:t,basis:e.basis?e.basis.map(function(e){return{a:new y(e.a,16),b:new y(e.b,16)}}):this._getEndoBasis(t)}},a.prototype._getEndoRoots=function(e){var t=e===this.p?this.red:y.mont(e),i=new y(2).toRed(t).redInvm(),e=i.redNeg(),i=new y(3).toRed(t).redNeg().redSqrt().redMul(i);return[e.redAdd(i).fromRed(),e.redSub(i).fromRed()]},a.prototype._getEndoBasis=function(e){for(var t,i,r,f,d,n,a,s=this.n.ushrn(Math.floor(this.n.bitLength()/2)),c=e,h=this.n.clone(),o=new y(1),u=new y(0),b=new y(0),l=new y(1),p=0;0!==c.cmpn(0);){var m=h.div(c),v=h.sub(m.mul(c)),g=b.sub(m.mul(o)),m=l.sub(m.mul(u));if(!r&&v.cmp(s)<0)t=a.neg(),i=o,r=v.neg(),f=g;else if(r&&2==++p)break;h=c,c=a=v,b=o,o=g,l=u,u=m}d=v.neg(),n=g;e=r.sqr().add(f.sqr());return 0<=d.sqr().add(n.sqr()).cmp(e)&&(d=t,n=i),r.negative&&(r=r.neg(),f=f.neg()),d.negative&&(d=d.neg(),n=n.neg()),[{a:r,b:f},{a:d,b:n}]},a.prototype._endoSplit=function(e){var t=this.endo.basis,i=t[0],r=t[1],f=r.b.mul(e).divRound(this.n),d=i.b.neg().mul(e).divRound(this.n),n=f.mul(i.a),t=d.mul(r.a),i=f.mul(i.b),r=d.mul(r.b);return{k1:e.sub(n).sub(t),k2:i.add(r).neg()}},a.prototype.pointFromX=function(e,t){var i=(e=!(e=new y(e,16)).red?e.toRed(this.red):e).redSqr().redMul(e).redIAdd(e.redMul(this.a)).redIAdd(this.b),r=i.redSqrt();if(0!==r.redSqr().redSub(i).cmp(this.zero))throw new Error("invalid point");i=r.fromRed().isOdd();return(t&&!i||!t&&i)&&(r=r.redNeg()),this.point(e,r)},a.prototype.validate=function(e){if(e.inf)return!0;var t=e.x,i=e.y,e=this.a.redMul(t),e=t.redSqr().redMul(t).redIAdd(e).redIAdd(this.b);return 0===i.redSqr().redISub(e).cmpn(0)},a.prototype._endoWnafMulAdd=function(e,t,i){for(var r=this._endoWnafT1,f=this._endoWnafT2,d=0;d<e.length;d++){var n=this._endoSplit(t[d]),a=e[d],s=a._getBeta();n.k1.negative&&(n.k1.ineg(),a=a.neg(!0)),n.k2.negative&&(n.k2.ineg(),s=s.neg(!0)),r[2*d]=a,r[2*d+1]=s,f[2*d]=n.k1,f[2*d+1]=n.k2}for(var i=this._wnafMulAdd(1,r,f,2*d,i),c=0;c<2*d;c++)r[c]=null,f[c]=null;return i},f(s,d.BasePoint),a.prototype.point=function(e,t,i){return new s(this,e,t,i)},a.prototype.pointFromJSON=function(e,t){return s.fromJSON(this,e,t)},s.prototype._getBeta=function(){if(this.curve.endo){var e=this.precomputed;if(e&&e.beta)return e.beta;var t,i,r=this.curve.point(this.x.redMul(this.curve.endo.beta),this.y);return e&&(t=this.curve,i=function(e){return t.point(e.x.redMul(t.endo.beta),e.y)},(e.beta=r).precomputed={beta:null,naf:e.naf&&{wnd:e.naf.wnd,points:e.naf.points.map(i)},doubles:e.doubles&&{step:e.doubles.step,points:e.doubles.points.map(i)}}),r}},s.prototype.toJSON=function(){return this.precomputed?[this.x,this.y,this.precomputed&&{doubles:this.precomputed.doubles&&{step:this.precomputed.doubles.step,points:this.precomputed.doubles.points.slice(1)},naf:this.precomputed.naf&&{wnd:this.precomputed.naf.wnd,points:this.precomputed.naf.points.slice(1)}}]:[this.x,this.y]},s.fromJSON=function(t,e,i){"string"==typeof e&&(e=JSON.parse(e));var r=t.point(e[0],e[1],i);if(!e[2])return r;function f(e){return t.point(e[0],e[1],i)}e=e[2];return r.precomputed={beta:null,doubles:e.doubles&&{step:e.doubles.step,points:[r].concat(e.doubles.points.map(f))},naf:e.naf&&{wnd:e.naf.wnd,points:[r].concat(e.naf.points.map(f))}},r},s.prototype.inspect=function(){return this.isInfinity()?"<EC Point Infinity>":"<EC Point x: "+this.x.fromRed().toString(16,2)+" y: "+this.y.fromRed().toString(16,2)+">"},s.prototype.isInfinity=function(){return this.inf},s.prototype.add=function(e){if(this.inf)return e;if(e.inf)return this;if(this.eq(e))return this.dbl();if(this.neg().eq(e))return this.curve.point(null,null);if(0===this.x.cmp(e.x))return this.curve.point(null,null);var t=this.y.redSub(e.y),e=(t=0!==t.cmpn(0)?t.redMul(this.x.redSub(e.x).redInvm()):t).redSqr().redISub(this.x).redISub(e.x),t=t.redMul(this.x.redSub(e)).redISub(this.y);return this.curve.point(e,t)},s.prototype.dbl=function(){if(this.inf)return this;var e=this.y.redAdd(this.y);if(0===e.cmpn(0))return this.curve.point(null,null);var t=this.curve.a,i=this.x.redSqr(),e=e.redInvm(),t=i.redAdd(i).redIAdd(i).redIAdd(t).redMul(e),e=t.redSqr().redISub(this.x.redAdd(this.x)),t=t.redMul(this.x.redSub(e)).redISub(this.y);return this.curve.point(e,t)},s.prototype.getX=function(){return this.x.fromRed()},s.prototype.getY=function(){return this.y.fromRed()},s.prototype.mul=function(e){return e=new y(e,16),this.isInfinity()?this:this._hasDoubles(e)?this.curve._fixedNafMul(this,e):this.curve.endo?this.curve._endoWnafMulAdd([this],[e]):this.curve._wnafMul(this,e)},s.prototype.mulAdd=function(e,t,i){t=[this,t],i=[e,i];return this.curve.endo?this.curve._endoWnafMulAdd(t,i):this.curve._wnafMulAdd(1,t,i,2)},s.prototype.jmulAdd=function(e,t,i){t=[this,t],i=[e,i];return this.curve.endo?this.curve._endoWnafMulAdd(t,i,!0):this.curve._wnafMulAdd(1,t,i,2,!0)},s.prototype.eq=function(e){return this===e||this.inf===e.inf&&(this.inf||0===this.x.cmp(e.x)&&0===this.y.cmp(e.y))},s.prototype.neg=function(e){if(this.inf)return this;var t,i=this.curve.point(this.x,this.y.redNeg());return e&&this.precomputed&&(t=this.precomputed,e=function(e){return e.neg()},i.precomputed={naf:t.naf&&{wnd:t.naf.wnd,points:t.naf.points.map(e)},doubles:t.doubles&&{step:t.doubles.step,points:t.doubles.points.map(e)}}),i},s.prototype.toJ=function(){return this.inf?this.curve.jpoint(null,null,null):this.curve.jpoint(this.x,this.y,this.curve.one)},f(c,d.BasePoint),a.prototype.jpoint=function(e,t,i){return new c(this,e,t,i)},c.prototype.toP=function(){if(this.isInfinity())return this.curve.point(null,null);var e=this.z.redInvm(),t=e.redSqr(),i=this.x.redMul(t),e=this.y.redMul(t).redMul(e);return this.curve.point(i,e)},c.prototype.neg=function(){return this.curve.jpoint(this.x,this.y.redNeg(),this.z)},c.prototype.add=function(e){if(this.isInfinity())return e;if(e.isInfinity())return this;var t=e.z.redSqr(),i=this.z.redSqr(),r=this.x.redMul(t),f=e.x.redMul(i),d=this.y.redMul(t.redMul(e.z)),n=e.y.redMul(i.redMul(this.z)),t=r.redSub(f),i=d.redSub(n);if(0===t.cmpn(0))return 0!==i.cmpn(0)?this.curve.jpoint(null,null,null):this.dbl();f=t.redSqr(),n=f.redMul(t),r=r.redMul(f),f=i.redSqr().redIAdd(n).redISub(r).redISub(r),n=i.redMul(r.redISub(f)).redISub(d.redMul(n)),t=this.z.redMul(e.z).redMul(t);return this.curve.jpoint(f,n,t)},c.prototype.mixedAdd=function(e){if(this.isInfinity())return e.toJ();if(e.isInfinity())return this;var t=this.z.redSqr(),i=this.x,r=e.x.redMul(t),f=this.y,d=e.y.redMul(t).redMul(this.z),e=i.redSub(r),t=f.redSub(d);if(0===e.cmpn(0))return 0!==t.cmpn(0)?this.curve.jpoint(null,null,null):this.dbl();r=e.redSqr(),d=r.redMul(e),i=i.redMul(r),r=t.redSqr().redIAdd(d).redISub(i).redISub(i),d=t.redMul(i.redISub(r)).redISub(f.redMul(d)),e=this.z.redMul(e);return this.curve.jpoint(r,d,e)},c.prototype.dblp=function(e){if(0===e)return this;if(this.isInfinity())return this;if(!e)return this.dbl();if(this.curve.zeroA||this.curve.threeA){for(var t=this,i=0;i<e;i++)t=t.dbl();return t}var r=this.curve.a,f=this.curve.tinv,d=this.x,n=this.y,a=this.z,s=a.redSqr().redSqr(),c=n.redAdd(n);for(i=0;i<e;i++){var h=d.redSqr(),o=c.redSqr(),u=o.redSqr(),b=h.redAdd(h).redIAdd(h).redIAdd(r.redMul(s)),h=d.redMul(o),o=b.redSqr().redISub(h.redAdd(h)),h=h.redISub(o),b=(b=b.redMul(h)).redIAdd(b).redISub(u),h=c.redMul(a);i+1<e&&(s=s.redMul(u)),d=o,a=h,c=b}return this.curve.jpoint(d,c.redMul(f),a)},c.prototype.dbl=function(){return this.isInfinity()?this:this.curve.zeroA?this._zeroDbl():this.curve.threeA?this._threeDbl():this._dbl()},c.prototype._zeroDbl=function(){var e,t,i,r,f,d=this.zOne?(i=this.x.redSqr(),e=(r=this.y.redSqr()).redSqr(),f=(f=this.x.redAdd(r).redSqr().redISub(i).redISub(e)).redIAdd(f),r=(t=i.redAdd(i).redIAdd(i)).redSqr().redISub(f).redISub(f),i=(i=(i=e.redIAdd(e)).redIAdd(i)).redIAdd(i),e=r,t=t.redMul(f.redISub(r)).redISub(i),this.y.redAdd(this.y)):(f=this.x.redSqr(),d=(r=this.y.redSqr()).redSqr(),i=(i=this.x.redAdd(r).redSqr().redISub(f).redISub(d)).redIAdd(i),f=(r=f.redAdd(f).redIAdd(f)).redSqr(),d=(d=(d=d.redIAdd(d)).redIAdd(d)).redIAdd(d),e=f.redISub(i).redISub(i),t=r.redMul(i.redISub(e)).redISub(d),(d=this.y.redMul(this.z)).redIAdd(d));return this.curve.jpoint(e,t,d)},c.prototype._threeDbl=function(){var e,t,i,r,f,d,n,a;return this.zOne?(e=this.x.redSqr(),r=(t=this.y.redSqr()).redSqr(),n=(n=this.x.redAdd(t).redSqr().redISub(e).redISub(r)).redIAdd(n),i=f=(a=e.redAdd(e).redIAdd(e).redIAdd(this.curve.a)).redSqr().redISub(n).redISub(n),d=(d=(d=r.redIAdd(r)).redIAdd(d)).redIAdd(d),t=a.redMul(n.redISub(f)).redISub(d),e=this.y.redAdd(this.y)):(r=this.z.redSqr(),a=this.y.redSqr(),n=this.x.redMul(a),f=(f=this.x.redSub(r).redMul(this.x.redAdd(r))).redAdd(f).redIAdd(f),n=(d=(d=n.redIAdd(n)).redIAdd(d)).redAdd(d),i=f.redSqr().redISub(n),e=this.y.redAdd(this.z).redSqr().redISub(a).redISub(r),a=(a=(a=(a=a.redSqr()).redIAdd(a)).redIAdd(a)).redIAdd(a),t=f.redMul(d.redISub(i)).redISub(a)),this.curve.jpoint(i,t,e)},c.prototype._dbl=function(){var e=this.curve.a,t=this.x,i=this.y,r=this.z,f=r.redSqr().redSqr(),d=t.redSqr(),n=i.redSqr(),e=d.redAdd(d).redIAdd(d).redIAdd(e.redMul(f)),f=t.redAdd(t),t=(f=f.redIAdd(f)).redMul(n),f=e.redSqr().redISub(t.redAdd(t)),t=t.redISub(f),n=n.redSqr();n=(n=(n=n.redIAdd(n)).redIAdd(n)).redIAdd(n);n=e.redMul(t).redISub(n),r=i.redAdd(i).redMul(r);return this.curve.jpoint(f,n,r)},c.prototype.trpl=function(){if(!this.curve.zeroA)return this.dbl().add(this);var e=this.x.redSqr(),t=this.y.redSqr(),i=this.z.redSqr(),r=t.redSqr(),f=e.redAdd(e).redIAdd(e),d=f.redSqr(),n=this.x.redAdd(t).redSqr().redISub(e).redISub(r),e=(n=(n=(n=n.redIAdd(n)).redAdd(n).redIAdd(n)).redISub(d)).redSqr(),r=r.redIAdd(r);r=(r=(r=r.redIAdd(r)).redIAdd(r)).redIAdd(r);d=f.redIAdd(n).redSqr().redISub(d).redISub(e).redISub(r),t=t.redMul(d);t=(t=t.redIAdd(t)).redIAdd(t);t=this.x.redMul(e).redISub(t);t=(t=t.redIAdd(t)).redIAdd(t);d=this.y.redMul(d.redMul(r.redISub(d)).redISub(n.redMul(e)));d=(d=(d=d.redIAdd(d)).redIAdd(d)).redIAdd(d);e=this.z.redAdd(n).redSqr().redISub(i).redISub(e);return this.curve.jpoint(t,d,e)},c.prototype.mul=function(e,t){return e=new y(e,t),this.curve._wnafMul(this,e)},c.prototype.eq=function(e){if("affine"===e.type)return this.eq(e.toJ());if(this===e)return!0;var t=this.z.redSqr(),i=e.z.redSqr();if(0!==this.x.redMul(i).redISub(e.x.redMul(t)).cmpn(0))return!1;t=t.redMul(this.z),i=i.redMul(e.z);return 0===this.y.redMul(i).redISub(e.y.redMul(t)).cmpn(0)},c.prototype.eqXToP=function(e){var t=this.z.redSqr(),i=e.toRed(this.curve.red).redMul(t);if(0===this.x.cmp(i))return!0;for(var r=e.clone(),f=this.curve.redN.redMul(t);;){if(r.iadd(this.curve.n),0<=r.cmp(this.curve.p))return!1;if(i.redIAdd(f),0===this.x.cmp(i))return!0}},c.prototype.inspect=function(){return this.isInfinity()?"<EC JPoint Infinity>":"<EC JPoint x: "+this.x.toString(16,2)+" y: "+this.y.toString(16,2)+" z: "+this.z.toString(16,2)+">"},c.prototype.isInfinity=function(){return 0===this.z.cmpn(0)}},{"../utils":15,"./base":2,"bn.js":16,inherits:32}],7:[function(e,t,i){"use strict";var r,f=i,i=e("hash.js"),d=e("./curve"),n=e("./utils").assert;function a(e){"short"===e.type?this.curve=new d.short(e):"edwards"===e.type?this.curve=new d.edwards(e):this.curve=new d.mont(e),this.g=this.curve.g,this.n=this.curve.n,this.hash=e.hash,n(this.g.validate(),"Invalid curve"),n(this.g.mul(this.n).isInfinity(),"Invalid curve, G*N != O")}function s(t,i){Object.defineProperty(f,t,{configurable:!0,enumerable:!0,get:function(){var e=new a(i);return Object.defineProperty(f,t,{configurable:!0,enumerable:!0,value:e}),e}})}f.PresetCurve=a,s("p192",{type:"short",prime:"p192",p:"ffffffff ffffffff ffffffff fffffffe ffffffff ffffffff",a:"ffffffff ffffffff ffffffff fffffffe ffffffff fffffffc",b:"64210519 e59c80e7 0fa7e9ab 72243049 feb8deec c146b9b1",n:"ffffffff ffffffff ffffffff 99def836 146bc9b1 b4d22831",hash:i.sha256,gRed:!1,g:["188da80e b03090f6 7cbf20eb 43a18800 f4ff0afd 82ff1012","07192b95 ffc8da78 631011ed 6b24cdd5 73f977a1 1e794811"]}),s("p224",{type:"short",prime:"p224",p:"ffffffff ffffffff ffffffff ffffffff 00000000 00000000 00000001",a:"ffffffff ffffffff ffffffff fffffffe ffffffff ffffffff fffffffe",b:"b4050a85 0c04b3ab f5413256 5044b0b7 d7bfd8ba 270b3943 2355ffb4",n:"ffffffff ffffffff ffffffff ffff16a2 e0b8f03e 13dd2945 5c5c2a3d",hash:i.sha256,gRed:!1,g:["b70e0cbd 6bb4bf7f 321390b9 4a03c1d3 56c21122 343280d6 115c1d21","bd376388 b5f723fb 4c22dfe6 cd4375a0 5a074764 44d58199 85007e34"]}),s("p256",{type:"short",prime:null,p:"ffffffff 00000001 00000000 00000000 00000000 ffffffff ffffffff ffffffff",a:"ffffffff 00000001 00000000 00000000 00000000 ffffffff ffffffff fffffffc",b:"5ac635d8 aa3a93e7 b3ebbd55 769886bc 651d06b0 cc53b0f6 3bce3c3e 27d2604b",n:"ffffffff 00000000 ffffffff ffffffff bce6faad a7179e84 f3b9cac2 fc632551",hash:i.sha256,gRed:!1,g:["6b17d1f2 e12c4247 f8bce6e5 63a440f2 77037d81 2deb33a0 f4a13945 d898c296","4fe342e2 fe1a7f9b 8ee7eb4a 7c0f9e16 2bce3357 6b315ece cbb64068 37bf51f5"]}),s("p384",{type:"short",prime:null,p:"ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe ffffffff 00000000 00000000 ffffffff",a:"ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe ffffffff 00000000 00000000 fffffffc",b:"b3312fa7 e23ee7e4 988e056b e3f82d19 181d9c6e fe814112 0314088f 5013875a c656398d 8a2ed19d 2a85c8ed d3ec2aef",n:"ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff c7634d81 f4372ddf 581a0db2 48b0a77a ecec196a ccc52973",hash:i.sha384,gRed:!1,g:["aa87ca22 be8b0537 8eb1c71e f320ad74 6e1d3b62 8ba79b98 59f741e0 82542a38 5502f25d bf55296c 3a545e38 72760ab7","3617de4a 96262c6f 5d9e98bf 9292dc29 f8f41dbd 289a147c e9da3113 b5f0b8c0 0a60b1ce 1d7e819d 7a431d7c 90ea0e5f"]}),s("p521",{type:"short",prime:null,p:"000001ff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff",a:"000001ff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffc",b:"00000051 953eb961 8e1c9a1f 929a21a0 b68540ee a2da725b 99b315f3 b8b48991 8ef109e1 56193951 ec7e937b 1652c0bd 3bb1bf07 3573df88 3d2c34f1 ef451fd4 6b503f00",n:"000001ff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffa 51868783 bf2f966b 7fcc0148 f709a5d0 3bb5c9b8 899c47ae bb6fb71e 91386409",hash:i.sha512,gRed:!1,g:["000000c6 858e06b7 0404e9cd 9e3ecb66 2395b442 9c648139 053fb521 f828af60 6b4d3dba a14b5e77 efe75928 fe1dc127 a2ffa8de 3348b3c1 856a429b f97e7e31 c2e5bd66","00000118 39296a78 9a3bc004 5c8a5fb4 2c7d1bd9 98f54449 579b4468 17afbd17 273e662c 97ee7299 5ef42640 c550b901 3fad0761 353c7086 a272c240 88be9476 9fd16650"]}),s("curve25519",{type:"mont",prime:"p25519",p:"7fffffffffffffff ffffffffffffffff ffffffffffffffff ffffffffffffffed",a:"76d06",b:"1",n:"1000000000000000 0000000000000000 14def9dea2f79cd6 5812631a5cf5d3ed",hash:i.sha256,gRed:!1,g:["9"]}),s("ed25519",{type:"edwards",prime:"p25519",p:"7fffffffffffffff ffffffffffffffff ffffffffffffffff ffffffffffffffed",a:"-1",c:"1",d:"52036cee2b6ffe73 8cc740797779e898 00700a4d4141d8ab 75eb4dca135978a3",n:"1000000000000000 0000000000000000 14def9dea2f79cd6 5812631a5cf5d3ed",hash:i.sha256,gRed:!1,g:["216936d3cd6e53fec0a4e231fdd6dc5c692cc7609525a7b2c9562d608f25d51a","6666666666666666666666666666666666666666666666666666666666666658"]});try{r=e("./precomputed/secp256k1")}catch(e){r=void 0}s("secp256k1",{type:"short",prime:"k256",p:"ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe fffffc2f",a:"0",b:"7",n:"ffffffff ffffffff ffffffff fffffffe baaedce6 af48a03b bfd25e8c d0364141",h:"1",hash:i.sha256,beta:"7ae96a2b657c07106e64479eac3434e99cf0497512f58995c1396c28719501ee",lambda:"5363ad4cc05c30e0a5261c028812645a122e22ea20816678df02967c1b23bd72",basis:[{a:"3086d221a7d46bcde86c90e49284eb15",b:"-e4437ed6010e88286f547fa90abfe4c3"},{a:"114ca50f7a8e2f3f657c1108d9d44cfd8",b:"3086d221a7d46bcde86c90e49284eb15"}],gRed:!1,g:["79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798","483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8",r]})},{"./curve":4,"./precomputed/secp256k1":14,"./utils":15,"hash.js":19}],8:[function(e,t,i){"use strict";var u=e("bn.js"),b=e("hmac-drbg"),r=e("../utils"),f=e("../curves"),d=e("brorand"),a=r.assert,n=e("./key"),l=e("./signature");function s(e){if(!(this instanceof s))return new s(e);"string"==typeof e&&(a(Object.prototype.hasOwnProperty.call(f,e),"Unknown curve "+e),e=f[e]),e instanceof f.PresetCurve&&(e={curve:e}),this.curve=e.curve.curve,this.n=this.curve.n,this.nh=this.n.ushrn(1),this.g=this.curve.g,this.g=e.curve.g,this.g.precompute(e.curve.n.bitLength()+1),this.hash=e.hash||e.curve.hash}(t.exports=s).prototype.keyPair=function(e){return new n(this,e)},s.prototype.keyFromPrivate=function(e,t){return n.fromPrivate(this,e,t)},s.prototype.keyFromPublic=function(e,t){return n.fromPublic(this,e,t)},s.prototype.genKeyPair=function(e){e=e||{};for(var t=new b({hash:this.hash,pers:e.pers,persEnc:e.persEnc||"utf8",entropy:e.entropy||d(this.hash.hmacStrength),entropyEnc:e.entropy&&e.entropyEnc||"utf8",nonce:this.n.toArray()}),i=this.n.byteLength(),r=this.n.sub(new u(2));;){var f=new u(t.generate(i));if(!(0<f.cmp(r)))return f.iaddn(1),this.keyFromPrivate(f)}},s.prototype._truncateToN=function(e,t){var i=8*e.byteLength()-this.n.bitLength();return 0<i&&(e=e.ushrn(i)),!t&&0<=e.cmp(this.n)?e.sub(this.n):e},s.prototype.sign=function(e,t,i,r){"object"==typeof i&&(r=i,i=null),r=r||{},t=this.keyFromPrivate(t,i),e=this._truncateToN(new u(e,16));for(var f=this.n.byteLength(),i=t.getPrivate().toArray("be",f),f=e.toArray("be",f),d=new b({hash:this.hash,entropy:i,nonce:f,pers:r.pers,persEnc:r.persEnc||"utf8"}),n=this.n.sub(new u(1)),a=0;;a++){var s=r.k?r.k(a):new u(d.generate(this.n.byteLength()));if(!((s=this._truncateToN(s,!0)).cmpn(1)<=0||0<=s.cmp(n))){var c=this.g.mul(s);if(!c.isInfinity()){var h=c.getX(),o=h.umod(this.n);if(0!==o.cmpn(0)){s=s.invm(this.n).mul(o.mul(t.getPrivate()).iadd(e));if(0!==(s=s.umod(this.n)).cmpn(0)){h=(c.getY().isOdd()?1:0)|(0!==h.cmp(o)?2:0);return r.canonical&&0<s.cmp(this.nh)&&(s=this.n.sub(s),h^=1),new l({r:o,s:s,recoveryParam:h})}}}}}},s.prototype.verify=function(e,t,i,r){e=this._truncateToN(new u(e,16)),i=this.keyFromPublic(i,r);r=(t=new l(t,"hex")).r,t=t.s;if(r.cmpn(1)<0||0<=r.cmp(this.n))return!1;if(t.cmpn(1)<0||0<=t.cmp(this.n))return!1;var f,t=t.invm(this.n),e=t.mul(e).umod(this.n),t=t.mul(r).umod(this.n);return this.curve._maxwellTrick?!(f=this.g.jmulAdd(e,i.getPublic(),t)).isInfinity()&&f.eqXToP(r):!(f=this.g.mulAdd(e,i.getPublic(),t)).isInfinity()&&0===f.getX().umod(this.n).cmp(r)},s.prototype.recoverPubKey=function(e,t,i,r){a((3&i)===i,"The recovery param is more than two bits"),t=new l(t,r);var f=this.n,d=new u(e),n=t.r,r=t.s,e=1&i,i=i>>1;if(0<=n.cmp(this.curve.p.umod(this.curve.n))&&i)throw new Error("Unable to find sencond key candinate");n=i?this.curve.pointFromX(n.add(this.curve.n),e):this.curve.pointFromX(n,e);t=t.r.invm(f),d=f.sub(d).mul(t).umod(f),f=r.mul(t).umod(f);return this.g.mulAdd(d,n,f)},s.prototype.getKeyRecoveryParam=function(e,t,i,r){if(null!==(t=new l(t,r)).recoveryParam)return t.recoveryParam;for(var f,d=0;d<4;d++){try{f=this.recoverPubKey(e,t,d)}catch(e){continue}if(f.eq(i))return d}throw new Error("Unable to find valid recovery factor")}},{"../curves":7,"../utils":15,"./key":9,"./signature":10,"bn.js":16,brorand:17,"hmac-drbg":31}],9:[function(e,t,i){"use strict";var r=e("bn.js"),f=e("../utils").assert;function d(e,t){this.ec=e,this.priv=null,this.pub=null,t.priv&&this._importPrivate(t.priv,t.privEnc),t.pub&&this._importPublic(t.pub,t.pubEnc)}(t.exports=d).fromPublic=function(e,t,i){return t instanceof d?t:new d(e,{pub:t,pubEnc:i})},d.fromPrivate=function(e,t,i){return t instanceof d?t:new d(e,{priv:t,privEnc:i})},d.prototype.validate=function(){var e=this.getPublic();return e.isInfinity()?{result:!1,reason:"Invalid public key"}:e.validate()?e.mul(this.ec.curve.n).isInfinity()?{result:!0,reason:null}:{result:!1,reason:"Public key * N != O"}:{result:!1,reason:"Public key is not a point"}},d.prototype.getPublic=function(e,t){return"string"==typeof e&&(t=e,e=null),this.pub||(this.pub=this.ec.g.mul(this.priv)),t?this.pub.encode(t,e):this.pub},d.prototype.getPrivate=function(e){return"hex"===e?this.priv.toString(16,2):this.priv},d.prototype._importPrivate=function(e,t){this.priv=new r(e,t||16),this.priv=this.priv.umod(this.ec.curve.n)},d.prototype._importPublic=function(e,t){if(e.x||e.y)return"mont"===this.ec.curve.type?f(e.x,"Need x coordinate"):"short"!==this.ec.curve.type&&"edwards"!==this.ec.curve.type||f(e.x&&e.y,"Need both x and y coordinate"),void(this.pub=this.ec.curve.point(e.x,e.y));this.pub=this.ec.curve.decodePoint(e,t)},d.prototype.derive=function(e){return e.validate()||f(e.validate(),"public point not validated"),e.mul(this.priv).getX()},d.prototype.sign=function(e,t,i){return this.ec.sign(e,this,t,i)},d.prototype.verify=function(e,t){return this.ec.verify(e,t,this)},d.prototype.inspect=function(){return"<Key priv: "+(this.priv&&this.priv.toString(16,2))+" pub: "+(this.pub&&this.pub.inspect())+" >"}},{"../utils":15,"bn.js":16}],10:[function(e,t,i){"use strict";var f=e("bn.js"),d=e("../utils"),r=d.assert;function n(e,t){if(e instanceof n)return e;this._importDER(e,t)||(r(e.r&&e.s,"Signature without r or s"),this.r=new f(e.r,16),this.s=new f(e.s,16),void 0===e.recoveryParam?this.recoveryParam=null:this.recoveryParam=e.recoveryParam)}function a(){this.place=0}function s(e,t){var i=e[t.place++];if(!(128&i))return i;var r=15&i;if(0==r||4<r)return!1;for(var f=0,d=0,n=t.place;d<r;d++,n++)f<<=8,f|=e[n],f>>>=0;return!(f<=127)&&(t.place=n,f)}function c(e){for(var t=0,i=e.length-1;!e[t]&&!(128&e[t+1])&&t<i;)t++;return 0===t?e:e.slice(t)}function h(e,t){if(t<128)e.push(t);else{var i=1+(Math.log(t)/Math.LN2>>>3);for(e.push(128|i);--i;)e.push(t>>>(i<<3)&255);e.push(t)}}(t.exports=n).prototype._importDER=function(e,t){e=d.toArray(e,t);var i=new a;if(48!==e[i.place++])return!1;var r=s(e,i);if(!1===r)return!1;if(r+i.place!==e.length)return!1;if(2!==e[i.place++])return!1;t=s(e,i);if(!1===t)return!1;r=e.slice(i.place,t+i.place);if(i.place+=t,2!==e[i.place++])return!1;t=s(e,i);if(!1===t)return!1;if(e.length!==t+i.place)return!1;i=e.slice(i.place,t+i.place);if(0===r[0]){if(!(128&r[1]))return!1;r=r.slice(1)}if(0===i[0]){if(!(128&i[1]))return!1;i=i.slice(1)}return this.r=new f(r),this.s=new f(i),!(this.recoveryParam=null)},n.prototype.toDER=function(e){var t=this.r.toArray(),i=this.s.toArray();for(128&t[0]&&(t=[0].concat(t)),128&i[0]&&(i=[0].concat(i)),t=c(t),i=c(i);!(i[0]||128&i[1]);)i=i.slice(1);var r=[2];h(r,t.length),(r=r.concat(t)).push(2),h(r,i.length);t=r.concat(i),r=[48];return h(r,t.length),r=r.concat(t),d.encode(r,e)}},{"../utils":15,"bn.js":16}],11:[function(e,t,i){"use strict";var r=e("hash.js"),f=e("../curves"),d=e("../utils"),n=d.assert,a=d.parseBytes,s=e("./key"),c=e("./signature");function h(e){if(n("ed25519"===e,"only tested with ed25519 so far"),!(this instanceof h))return new h(e);e=f[e].curve,this.curve=e,this.g=e.g,this.g.precompute(e.n.bitLength()+1),this.pointClass=e.point().constructor,this.encodingLength=Math.ceil(e.n.bitLength()/8),this.hash=r.sha512}(t.exports=h).prototype.sign=function(e,t){e=a(e);var i=this.keyFromSecret(t),r=this.hashInt(i.messagePrefix(),e),f=this.g.mul(r),t=this.encodePoint(f),i=this.hashInt(t,i.pubBytes(),e).mul(i.priv()),i=r.add(i).umod(this.curve.n);return this.makeSignature({R:f,S:i,Rencoded:t})},h.prototype.verify=function(e,t,i){e=a(e),t=this.makeSignature(t);var r=this.keyFromPublic(i),i=this.hashInt(t.Rencoded(),r.pubBytes(),e),e=this.g.mul(t.S());return t.R().add(r.pub().mul(i)).eq(e)},h.prototype.hashInt=function(){for(var e=this.hash(),t=0;t<arguments.length;t++)e.update(arguments[t]);return d.intFromLE(e.digest()).umod(this.curve.n)},h.prototype.keyFromPublic=function(e){return s.fromPublic(this,e)},h.prototype.keyFromSecret=function(e){return s.fromSecret(this,e)},h.prototype.makeSignature=function(e){return e instanceof c?e:new c(this,e)},h.prototype.encodePoint=function(e){var t=e.getY().toArray("le",this.encodingLength);return t[this.encodingLength-1]|=e.getX().isOdd()?128:0,t},h.prototype.decodePoint=function(e){var t=(e=d.parseBytes(e)).length-1,i=e.slice(0,t).concat(-129&e[t]),t=0!=(128&e[t]),i=d.intFromLE(i);return this.curve.pointFromY(i,t)},h.prototype.encodeInt=function(e){return e.toArray("le",this.encodingLength)},h.prototype.decodeInt=function(e){return d.intFromLE(e)},h.prototype.isPoint=function(e){return e instanceof this.pointClass}},{"../curves":7,"../utils":15,"./key":12,"./signature":13,"hash.js":19}],12:[function(e,t,i){"use strict";var r=e("../utils"),f=r.assert,d=r.parseBytes,e=r.cachedProperty;function n(e,t){this.eddsa=e,this._secret=d(t.secret),e.isPoint(t.pub)?this._pub=t.pub:this._pubBytes=d(t.pub)}n.fromPublic=function(e,t){return t instanceof n?t:new n(e,{pub:t})},n.fromSecret=function(e,t){return t instanceof n?t:new n(e,{secret:t})},n.prototype.secret=function(){return this._secret},e(n,"pubBytes",function(){return this.eddsa.encodePoint(this.pub())}),e(n,"pub",function(){return this._pubBytes?this.eddsa.decodePoint(this._pubBytes):this.eddsa.g.mul(this.priv())}),e(n,"privBytes",function(){var e=this.eddsa,t=this.hash(),i=e.encodingLength-1,e=t.slice(0,e.encodingLength);return e[0]&=248,e[i]&=127,e[i]|=64,e}),e(n,"priv",function(){return this.eddsa.decodeInt(this.privBytes())}),e(n,"hash",function(){return this.eddsa.hash().update(this.secret()).digest()}),e(n,"messagePrefix",function(){return this.hash().slice(this.eddsa.encodingLength)}),n.prototype.sign=function(e){return f(this._secret,"KeyPair can only verify"),this.eddsa.sign(e,this)},n.prototype.verify=function(e,t){return this.eddsa.verify(e,t,this)},n.prototype.getSecret=function(e){return f(this._secret,"KeyPair is public only"),r.encode(this.secret(),e)},n.prototype.getPublic=function(e){return r.encode(this.pubBytes(),e)},t.exports=n},{"../utils":15}],13:[function(e,t,i){"use strict";var r=e("bn.js"),f=e("../utils"),d=f.assert,e=f.cachedProperty,n=f.parseBytes;function a(e,t){this.eddsa=e,"object"!=typeof t&&(t=n(t)),Array.isArray(t)&&(t={R:t.slice(0,e.encodingLength),S:t.slice(e.encodingLength)}),d(t.R&&t.S,"Signature without R or S"),e.isPoint(t.R)&&(this._R=t.R),t.S instanceof r&&(this._S=t.S),this._Rencoded=Array.isArray(t.R)?t.R:t.Rencoded,this._Sencoded=Array.isArray(t.S)?t.S:t.Sencoded}e(a,"S",function(){return this.eddsa.decodeInt(this.Sencoded())}),e(a,"R",function(){return this.eddsa.decodePoint(this.Rencoded())}),e(a,"Rencoded",function(){return this.eddsa.encodePoint(this.R())}),e(a,"Sencoded",function(){return this.eddsa.encodeInt(this.S())}),a.prototype.toBytes=function(){return this.Rencoded().concat(this.Sencoded())},a.prototype.toHex=function(){return f.encode(this.toBytes(),"hex").toUpperCase()},t.exports=a},{"../utils":15,"bn.js":16}],14:[function(e,t,i){t.exports={doubles:{step:4,points:[["e60fce93b59e9ec53011aabc21c23e97b2a31369b87a5ae9c44ee89e2a6dec0a","f7e3507399e595929db99f34f57937101296891e44d23f0be1f32cce69616821"],["8282263212c609d9ea2a6e3e172de238d8c39cabd5ac1ca10646e23fd5f51508","11f8a8098557dfe45e8256e830b60ace62d613ac2f7b17bed31b6eaff6e26caf"],["175e159f728b865a72f99cc6c6fc846de0b93833fd2222ed73fce5b551e5b739","d3506e0d9e3c79eba4ef97a51ff71f5eacb5955add24345c6efa6ffee9fed695"],["363d90d447b00c9c99ceac05b6262ee053441c7e55552ffe526bad8f83ff4640","4e273adfc732221953b445397f3363145b9a89008199ecb62003c7f3bee9de9"],["8b4b5f165df3c2be8c6244b5b745638843e4a781a15bcd1b69f79a55dffdf80c","4aad0a6f68d308b4b3fbd7813ab0da04f9e336546162ee56b3eff0c65fd4fd36"],["723cbaa6e5db996d6bf771c00bd548c7b700dbffa6c0e77bcb6115925232fcda","96e867b5595cc498a921137488824d6e2660a0653779494801dc069d9eb39f5f"],["eebfa4d493bebf98ba5feec812c2d3b50947961237a919839a533eca0e7dd7fa","5d9a8ca3970ef0f269ee7edaf178089d9ae4cdc3a711f712ddfd4fdae1de8999"],["100f44da696e71672791d0a09b7bde459f1215a29b3c03bfefd7835b39a48db0","cdd9e13192a00b772ec8f3300c090666b7ff4a18ff5195ac0fbd5cd62bc65a09"],["e1031be262c7ed1b1dc9227a4a04c017a77f8d4464f3b3852c8acde6e534fd2d","9d7061928940405e6bb6a4176597535af292dd419e1ced79a44f18f29456a00d"],["feea6cae46d55b530ac2839f143bd7ec5cf8b266a41d6af52d5e688d9094696d","e57c6b6c97dce1bab06e4e12bf3ecd5c981c8957cc41442d3155debf18090088"],["da67a91d91049cdcb367be4be6ffca3cfeed657d808583de33fa978bc1ec6cb1","9bacaa35481642bc41f463f7ec9780e5dec7adc508f740a17e9ea8e27a68be1d"],["53904faa0b334cdda6e000935ef22151ec08d0f7bb11069f57545ccc1a37b7c0","5bc087d0bc80106d88c9eccac20d3c1c13999981e14434699dcb096b022771c8"],["8e7bcd0bd35983a7719cca7764ca906779b53a043a9b8bcaeff959f43ad86047","10b7770b2a3da4b3940310420ca9514579e88e2e47fd68b3ea10047e8460372a"],["385eed34c1cdff21e6d0818689b81bde71a7f4f18397e6690a841e1599c43862","283bebc3e8ea23f56701de19e9ebf4576b304eec2086dc8cc0458fe5542e5453"],["6f9d9b803ecf191637c73a4413dfa180fddf84a5947fbc9c606ed86c3fac3a7","7c80c68e603059ba69b8e2a30e45c4d47ea4dd2f5c281002d86890603a842160"],["3322d401243c4e2582a2147c104d6ecbf774d163db0f5e5313b7e0e742d0e6bd","56e70797e9664ef5bfb019bc4ddaf9b72805f63ea2873af624f3a2e96c28b2a0"],["85672c7d2de0b7da2bd1770d89665868741b3f9af7643397721d74d28134ab83","7c481b9b5b43b2eb6374049bfa62c2e5e77f17fcc5298f44c8e3094f790313a6"],["948bf809b1988a46b06c9f1919413b10f9226c60f668832ffd959af60c82a0a","53a562856dcb6646dc6b74c5d1c3418c6d4dff08c97cd2bed4cb7f88d8c8e589"],["6260ce7f461801c34f067ce0f02873a8f1b0e44dfc69752accecd819f38fd8e8","bc2da82b6fa5b571a7f09049776a1ef7ecd292238051c198c1a84e95b2b4ae17"],["e5037de0afc1d8d43d8348414bbf4103043ec8f575bfdc432953cc8d2037fa2d","4571534baa94d3b5f9f98d09fb990bddbd5f5b03ec481f10e0e5dc841d755bda"],["e06372b0f4a207adf5ea905e8f1771b4e7e8dbd1c6a6c5b725866a0ae4fce725","7a908974bce18cfe12a27bb2ad5a488cd7484a7787104870b27034f94eee31dd"],["213c7a715cd5d45358d0bbf9dc0ce02204b10bdde2a3f58540ad6908d0559754","4b6dad0b5ae462507013ad06245ba190bb4850f5f36a7eeddff2c27534b458f2"],["4e7c272a7af4b34e8dbb9352a5419a87e2838c70adc62cddf0cc3a3b08fbd53c","17749c766c9d0b18e16fd09f6def681b530b9614bff7dd33e0b3941817dcaae6"],["fea74e3dbe778b1b10f238ad61686aa5c76e3db2be43057632427e2840fb27b6","6e0568db9b0b13297cf674deccb6af93126b596b973f7b77701d3db7f23cb96f"],["76e64113f677cf0e10a2570d599968d31544e179b760432952c02a4417bdde39","c90ddf8dee4e95cf577066d70681f0d35e2a33d2b56d2032b4b1752d1901ac01"],["c738c56b03b2abe1e8281baa743f8f9a8f7cc643df26cbee3ab150242bcbb891","893fb578951ad2537f718f2eacbfbbbb82314eef7880cfe917e735d9699a84c3"],["d895626548b65b81e264c7637c972877d1d72e5f3a925014372e9f6588f6c14b","febfaa38f2bc7eae728ec60818c340eb03428d632bb067e179363ed75d7d991f"],["b8da94032a957518eb0f6433571e8761ceffc73693e84edd49150a564f676e03","2804dfa44805a1e4d7c99cc9762808b092cc584d95ff3b511488e4e74efdf6e7"],["e80fea14441fb33a7d8adab9475d7fab2019effb5156a792f1a11778e3c0df5d","eed1de7f638e00771e89768ca3ca94472d155e80af322ea9fcb4291b6ac9ec78"],["a301697bdfcd704313ba48e51d567543f2a182031efd6915ddc07bbcc4e16070","7370f91cfb67e4f5081809fa25d40f9b1735dbf7c0a11a130c0d1a041e177ea1"],["90ad85b389d6b936463f9d0512678de208cc330b11307fffab7ac63e3fb04ed4","e507a3620a38261affdcbd9427222b839aefabe1582894d991d4d48cb6ef150"],["8f68b9d2f63b5f339239c1ad981f162ee88c5678723ea3351b7b444c9ec4c0da","662a9f2dba063986de1d90c2b6be215dbbea2cfe95510bfdf23cbf79501fff82"],["e4f3fb0176af85d65ff99ff9198c36091f48e86503681e3e6686fd5053231e11","1e63633ad0ef4f1c1661a6d0ea02b7286cc7e74ec951d1c9822c38576feb73bc"],["8c00fa9b18ebf331eb961537a45a4266c7034f2f0d4e1d0716fb6eae20eae29e","efa47267fea521a1a9dc343a3736c974c2fadafa81e36c54e7d2a4c66702414b"],["e7a26ce69dd4829f3e10cec0a9e98ed3143d084f308b92c0997fddfc60cb3e41","2a758e300fa7984b471b006a1aafbb18d0a6b2c0420e83e20e8a9421cf2cfd51"],["b6459e0ee3662ec8d23540c223bcbdc571cbcb967d79424f3cf29eb3de6b80ef","67c876d06f3e06de1dadf16e5661db3c4b3ae6d48e35b2ff30bf0b61a71ba45"],["d68a80c8280bb840793234aa118f06231d6f1fc67e73c5a5deda0f5b496943e8","db8ba9fff4b586d00c4b1f9177b0e28b5b0e7b8f7845295a294c84266b133120"],["324aed7df65c804252dc0270907a30b09612aeb973449cea4095980fc28d3d5d","648a365774b61f2ff130c0c35aec1f4f19213b0c7e332843967224af96ab7c84"],["4df9c14919cde61f6d51dfdbe5fee5dceec4143ba8d1ca888e8bd373fd054c96","35ec51092d8728050974c23a1d85d4b5d506cdc288490192ebac06cad10d5d"],["9c3919a84a474870faed8a9c1cc66021523489054d7f0308cbfc99c8ac1f98cd","ddb84f0f4a4ddd57584f044bf260e641905326f76c64c8e6be7e5e03d4fc599d"],["6057170b1dd12fdf8de05f281d8e06bb91e1493a8b91d4cc5a21382120a959e5","9a1af0b26a6a4807add9a2daf71df262465152bc3ee24c65e899be932385a2a8"],["a576df8e23a08411421439a4518da31880cef0fba7d4df12b1a6973eecb94266","40a6bf20e76640b2c92b97afe58cd82c432e10a7f514d9f3ee8be11ae1b28ec8"],["7778a78c28dec3e30a05fe9629de8c38bb30d1f5cf9a3a208f763889be58ad71","34626d9ab5a5b22ff7098e12f2ff580087b38411ff24ac563b513fc1fd9f43ac"],["928955ee637a84463729fd30e7afd2ed5f96274e5ad7e5cb09eda9c06d903ac","c25621003d3f42a827b78a13093a95eeac3d26efa8a8d83fc5180e935bcd091f"],["85d0fef3ec6db109399064f3a0e3b2855645b4a907ad354527aae75163d82751","1f03648413a38c0be29d496e582cf5663e8751e96877331582c237a24eb1f962"],["ff2b0dce97eece97c1c9b6041798b85dfdfb6d8882da20308f5404824526087e","493d13fef524ba188af4c4dc54d07936c7b7ed6fb90e2ceb2c951e01f0c29907"],["827fbbe4b1e880ea9ed2b2e6301b212b57f1ee148cd6dd28780e5e2cf856e241","c60f9c923c727b0b71bef2c67d1d12687ff7a63186903166d605b68baec293ec"],["eaa649f21f51bdbae7be4ae34ce6e5217a58fdce7f47f9aa7f3b58fa2120e2b3","be3279ed5bbbb03ac69a80f89879aa5a01a6b965f13f7e59d47a5305ba5ad93d"],["e4a42d43c5cf169d9391df6decf42ee541b6d8f0c9a137401e23632dda34d24f","4d9f92e716d1c73526fc99ccfb8ad34ce886eedfa8d8e4f13a7f7131deba9414"],["1ec80fef360cbdd954160fadab352b6b92b53576a88fea4947173b9d4300bf19","aeefe93756b5340d2f3a4958a7abbf5e0146e77f6295a07b671cdc1cc107cefd"],["146a778c04670c2f91b00af4680dfa8bce3490717d58ba889ddb5928366642be","b318e0ec3354028add669827f9d4b2870aaa971d2f7e5ed1d0b297483d83efd0"],["fa50c0f61d22e5f07e3acebb1aa07b128d0012209a28b9776d76a8793180eef9","6b84c6922397eba9b72cd2872281a68a5e683293a57a213b38cd8d7d3f4f2811"],["da1d61d0ca721a11b1a5bf6b7d88e8421a288ab5d5bba5220e53d32b5f067ec2","8157f55a7c99306c79c0766161c91e2966a73899d279b48a655fba0f1ad836f1"],["a8e282ff0c9706907215ff98e8fd416615311de0446f1e062a73b0610d064e13","7f97355b8db81c09abfb7f3c5b2515888b679a3e50dd6bd6cef7c73111f4cc0c"],["174a53b9c9a285872d39e56e6913cab15d59b1fa512508c022f382de8319497c","ccc9dc37abfc9c1657b4155f2c47f9e6646b3a1d8cb9854383da13ac079afa73"],["959396981943785c3d3e57edf5018cdbe039e730e4918b3d884fdff09475b7ba","2e7e552888c331dd8ba0386a4b9cd6849c653f64c8709385e9b8abf87524f2fd"],["d2a63a50ae401e56d645a1153b109a8fcca0a43d561fba2dbb51340c9d82b151","e82d86fb6443fcb7565aee58b2948220a70f750af484ca52d4142174dcf89405"],["64587e2335471eb890ee7896d7cfdc866bacbdbd3839317b3436f9b45617e073","d99fcdd5bf6902e2ae96dd6447c299a185b90a39133aeab358299e5e9faf6589"],["8481bde0e4e4d885b3a546d3e549de042f0aa6cea250e7fd358d6c86dd45e458","38ee7b8cba5404dd84a25bf39cecb2ca900a79c42b262e556d64b1b59779057e"],["13464a57a78102aa62b6979ae817f4637ffcfed3c4b1ce30bcd6303f6caf666b","69be159004614580ef7e433453ccb0ca48f300a81d0942e13f495a907f6ecc27"],["bc4a9df5b713fe2e9aef430bcc1dc97a0cd9ccede2f28588cada3a0d2d83f366","d3a81ca6e785c06383937adf4b798caa6e8a9fbfa547b16d758d666581f33c1"],["8c28a97bf8298bc0d23d8c749452a32e694b65e30a9472a3954ab30fe5324caa","40a30463a3305193378fedf31f7cc0eb7ae784f0451cb9459e71dc73cbef9482"],["8ea9666139527a8c1dd94ce4f071fd23c8b350c5a4bb33748c4ba111faccae0","620efabbc8ee2782e24e7c0cfb95c5d735b783be9cf0f8e955af34a30e62b945"],["dd3625faef5ba06074669716bbd3788d89bdde815959968092f76cc4eb9a9787","7a188fa3520e30d461da2501045731ca941461982883395937f68d00c644a573"],["f710d79d9eb962297e4f6232b40e8f7feb2bc63814614d692c12de752408221e","ea98e67232d3b3295d3b535532115ccac8612c721851617526ae47a9c77bfc82"]]},naf:{wnd:7,points:[["f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9","388f7b0f632de8140fe337e62a37f3566500a99934c2231b6cb9fd7584b8e672"],["2f8bde4d1a07209355b4a7250a5c5128e88b84bddc619ab7cba8d569b240efe4","d8ac222636e5e3d6d4dba9dda6c9c426f788271bab0d6840dca87d3aa6ac62d6"],["5cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc","6aebca40ba255960a3178d6d861a54dba813d0b813fde7b5a5082628087264da"],["acd484e2f0c7f65309ad178a9f559abde09796974c57e714c35f110dfc27ccbe","cc338921b0a7d9fd64380971763b61e9add888a4375f8e0f05cc262ac64f9c37"],["774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb","d984a032eb6b5e190243dd56d7b7b365372db1e2dff9d6a8301d74c9c953c61b"],["f28773c2d975288bc7d1d205c3748651b075fbc6610e58cddeeddf8f19405aa8","ab0902e8d880a89758212eb65cdaf473a1a06da521fa91f29b5cb52db03ed81"],["d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e","581e2872a86c72a683842ec228cc6defea40af2bd896d3a5c504dc9ff6a26b58"],["defdea4cdb677750a420fee807eacf21eb9898ae79b9768766e4faa04a2d4a34","4211ab0694635168e997b0ead2a93daeced1f4a04a95c0f6cfb199f69e56eb77"],["2b4ea0a797a443d293ef5cff444f4979f06acfebd7e86d277475656138385b6c","85e89bc037945d93b343083b5a1c86131a01f60c50269763b570c854e5c09b7a"],["352bbf4a4cdd12564f93fa332ce333301d9ad40271f8107181340aef25be59d5","321eb4075348f534d59c18259dda3e1f4a1b3b2e71b1039c67bd3d8bcf81998c"],["2fa2104d6b38d11b0230010559879124e42ab8dfeff5ff29dc9cdadd4ecacc3f","2de1068295dd865b64569335bd5dd80181d70ecfc882648423ba76b532b7d67"],["9248279b09b4d68dab21a9b066edda83263c3d84e09572e269ca0cd7f5453714","73016f7bf234aade5d1aa71bdea2b1ff3fc0de2a887912ffe54a32ce97cb3402"],["daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729","a69dce4a7d6c98e8d4a1aca87ef8d7003f83c230f3afa726ab40e52290be1c55"],["c44d12c7065d812e8acf28d7cbb19f9011ecd9e9fdf281b0e6a3b5e87d22e7db","2119a460ce326cdc76c45926c982fdac0e106e861edf61c5a039063f0e0e6482"],["6a245bf6dc698504c89a20cfded60853152b695336c28063b61c65cbd269e6b4","e022cf42c2bd4a708b3f5126f16a24ad8b33ba48d0423b6efd5e6348100d8a82"],["1697ffa6fd9de627c077e3d2fe541084ce13300b0bec1146f95ae57f0d0bd6a5","b9c398f186806f5d27561506e4557433a2cf15009e498ae7adee9d63d01b2396"],["605bdb019981718b986d0f07e834cb0d9deb8360ffb7f61df982345ef27a7479","2972d2de4f8d20681a78d93ec96fe23c26bfae84fb14db43b01e1e9056b8c49"],["62d14dab4150bf497402fdc45a215e10dcb01c354959b10cfe31c7e9d87ff33d","80fc06bd8cc5b01098088a1950eed0db01aa132967ab472235f5642483b25eaf"],["80c60ad0040f27dade5b4b06c408e56b2c50e9f56b9b8b425e555c2f86308b6f","1c38303f1cc5c30f26e66bad7fe72f70a65eed4cbe7024eb1aa01f56430bd57a"],["7a9375ad6167ad54aa74c6348cc54d344cc5dc9487d847049d5eabb0fa03c8fb","d0e3fa9eca8726909559e0d79269046bdc59ea10c70ce2b02d499ec224dc7f7"],["d528ecd9b696b54c907a9ed045447a79bb408ec39b68df504bb51f459bc3ffc9","eecf41253136e5f99966f21881fd656ebc4345405c520dbc063465b521409933"],["49370a4b5f43412ea25f514e8ecdad05266115e4a7ecb1387231808f8b45963","758f3f41afd6ed428b3081b0512fd62a54c3f3afbb5b6764b653052a12949c9a"],["77f230936ee88cbbd73df930d64702ef881d811e0e1498e2f1c13eb1fc345d74","958ef42a7886b6400a08266e9ba1b37896c95330d97077cbbe8eb3c7671c60d6"],["f2dac991cc4ce4b9ea44887e5c7c0bce58c80074ab9d4dbaeb28531b7739f530","e0dedc9b3b2f8dad4da1f32dec2531df9eb5fbeb0598e4fd1a117dba703a3c37"],["463b3d9f662621fb1b4be8fbbe2520125a216cdfc9dae3debcba4850c690d45b","5ed430d78c296c3543114306dd8622d7c622e27c970a1de31cb377b01af7307e"],["f16f804244e46e2a09232d4aff3b59976b98fac14328a2d1a32496b49998f247","cedabd9b82203f7e13d206fcdf4e33d92a6c53c26e5cce26d6579962c4e31df6"],["caf754272dc84563b0352b7a14311af55d245315ace27c65369e15f7151d41d1","cb474660ef35f5f2a41b643fa5e460575f4fa9b7962232a5c32f908318a04476"],["2600ca4b282cb986f85d0f1709979d8b44a09c07cb86d7c124497bc86f082120","4119b88753c15bd6a693b03fcddbb45d5ac6be74ab5f0ef44b0be9475a7e4b40"],["7635ca72d7e8432c338ec53cd12220bc01c48685e24f7dc8c602a7746998e435","91b649609489d613d1d5e590f78e6d74ecfc061d57048bad9e76f302c5b9c61"],["754e3239f325570cdbbf4a87deee8a66b7f2b33479d468fbc1a50743bf56cc18","673fb86e5bda30fb3cd0ed304ea49a023ee33d0197a695d0c5d98093c536683"],["e3e6bd1071a1e96aff57859c82d570f0330800661d1c952f9fe2694691d9b9e8","59c9e0bba394e76f40c0aa58379a3cb6a5a2283993e90c4167002af4920e37f5"],["186b483d056a033826ae73d88f732985c4ccb1f32ba35f4b4cc47fdcf04aa6eb","3b952d32c67cf77e2e17446e204180ab21fb8090895138b4a4a797f86e80888b"],["df9d70a6b9876ce544c98561f4be4f725442e6d2b737d9c91a8321724ce0963f","55eb2dafd84d6ccd5f862b785dc39d4ab157222720ef9da217b8c45cf2ba2417"],["5edd5cc23c51e87a497ca815d5dce0f8ab52554f849ed8995de64c5f34ce7143","efae9c8dbc14130661e8cec030c89ad0c13c66c0d17a2905cdc706ab7399a868"],["290798c2b6476830da12fe02287e9e777aa3fba1c355b17a722d362f84614fba","e38da76dcd440621988d00bcf79af25d5b29c094db2a23146d003afd41943e7a"],["af3c423a95d9f5b3054754efa150ac39cd29552fe360257362dfdecef4053b45","f98a3fd831eb2b749a93b0e6f35cfb40c8cd5aa667a15581bc2feded498fd9c6"],["766dbb24d134e745cccaa28c99bf274906bb66b26dcf98df8d2fed50d884249a","744b1152eacbe5e38dcc887980da38b897584a65fa06cedd2c924f97cbac5996"],["59dbf46f8c94759ba21277c33784f41645f7b44f6c596a58ce92e666191abe3e","c534ad44175fbc300f4ea6ce648309a042ce739a7919798cd85e216c4a307f6e"],["f13ada95103c4537305e691e74e9a4a8dd647e711a95e73cb62dc6018cfd87b8","e13817b44ee14de663bf4bc808341f326949e21a6a75c2570778419bdaf5733d"],["7754b4fa0e8aced06d4167a2c59cca4cda1869c06ebadfb6488550015a88522c","30e93e864e669d82224b967c3020b8fa8d1e4e350b6cbcc537a48b57841163a2"],["948dcadf5990e048aa3874d46abef9d701858f95de8041d2a6828c99e2262519","e491a42537f6e597d5d28a3224b1bc25df9154efbd2ef1d2cbba2cae5347d57e"],["7962414450c76c1689c7b48f8202ec37fb224cf5ac0bfa1570328a8a3d7c77ab","100b610ec4ffb4760d5c1fc133ef6f6b12507a051f04ac5760afa5b29db83437"],["3514087834964b54b15b160644d915485a16977225b8847bb0dd085137ec47ca","ef0afbb2056205448e1652c48e8127fc6039e77c15c2378b7e7d15a0de293311"],["d3cc30ad6b483e4bc79ce2c9dd8bc54993e947eb8df787b442943d3f7b527eaf","8b378a22d827278d89c5e9be8f9508ae3c2ad46290358630afb34db04eede0a4"],["1624d84780732860ce1c78fcbfefe08b2b29823db913f6493975ba0ff4847610","68651cf9b6da903e0914448c6cd9d4ca896878f5282be4c8cc06e2a404078575"],["733ce80da955a8a26902c95633e62a985192474b5af207da6df7b4fd5fc61cd4","f5435a2bd2badf7d485a4d8b8db9fcce3e1ef8e0201e4578c54673bc1dc5ea1d"],["15d9441254945064cf1a1c33bbd3b49f8966c5092171e699ef258dfab81c045c","d56eb30b69463e7234f5137b73b84177434800bacebfc685fc37bbe9efe4070d"],["a1d0fcf2ec9de675b612136e5ce70d271c21417c9d2b8aaaac138599d0717940","edd77f50bcb5a3cab2e90737309667f2641462a54070f3d519212d39c197a629"],["e22fbe15c0af8ccc5780c0735f84dbe9a790badee8245c06c7ca37331cb36980","a855babad5cd60c88b430a69f53a1a7a38289154964799be43d06d77d31da06"],["311091dd9860e8e20ee13473c1155f5f69635e394704eaa74009452246cfa9b3","66db656f87d1f04fffd1f04788c06830871ec5a64feee685bd80f0b1286d8374"],["34c1fd04d301be89b31c0442d3e6ac24883928b45a9340781867d4232ec2dbdf","9414685e97b1b5954bd46f730174136d57f1ceeb487443dc5321857ba73abee"],["f219ea5d6b54701c1c14de5b557eb42a8d13f3abbcd08affcc2a5e6b049b8d63","4cb95957e83d40b0f73af4544cccf6b1f4b08d3c07b27fb8d8c2962a400766d1"],["d7b8740f74a8fbaab1f683db8f45de26543a5490bca627087236912469a0b448","fa77968128d9c92ee1010f337ad4717eff15db5ed3c049b3411e0315eaa4593b"],["32d31c222f8f6f0ef86f7c98d3a3335ead5bcd32abdd94289fe4d3091aa824bf","5f3032f5892156e39ccd3d7915b9e1da2e6dac9e6f26e961118d14b8462e1661"],["7461f371914ab32671045a155d9831ea8793d77cd59592c4340f86cbc18347b5","8ec0ba238b96bec0cbdddcae0aa442542eee1ff50c986ea6b39847b3cc092ff6"],["ee079adb1df1860074356a25aa38206a6d716b2c3e67453d287698bad7b2b2d6","8dc2412aafe3be5c4c5f37e0ecc5f9f6a446989af04c4e25ebaac479ec1c8c1e"],["16ec93e447ec83f0467b18302ee620f7e65de331874c9dc72bfd8616ba9da6b5","5e4631150e62fb40d0e8c2a7ca5804a39d58186a50e497139626778e25b0674d"],["eaa5f980c245f6f038978290afa70b6bd8855897f98b6aa485b96065d537bd99","f65f5d3e292c2e0819a528391c994624d784869d7e6ea67fb18041024edc07dc"],["78c9407544ac132692ee1910a02439958ae04877151342ea96c4b6b35a49f51","f3e0319169eb9b85d5404795539a5e68fa1fbd583c064d2462b675f194a3ddb4"],["494f4be219a1a77016dcd838431aea0001cdc8ae7a6fc688726578d9702857a5","42242a969283a5f339ba7f075e36ba2af925ce30d767ed6e55f4b031880d562c"],["a598a8030da6d86c6bc7f2f5144ea549d28211ea58faa70ebf4c1e665c1fe9b5","204b5d6f84822c307e4b4a7140737aec23fc63b65b35f86a10026dbd2d864e6b"],["c41916365abb2b5d09192f5f2dbeafec208f020f12570a184dbadc3e58595997","4f14351d0087efa49d245b328984989d5caf9450f34bfc0ed16e96b58fa9913"],["841d6063a586fa475a724604da03bc5b92a2e0d2e0a36acfe4c73a5514742881","73867f59c0659e81904f9a1c7543698e62562d6744c169ce7a36de01a8d6154"],["5e95bb399a6971d376026947f89bde2f282b33810928be4ded112ac4d70e20d5","39f23f366809085beebfc71181313775a99c9aed7d8ba38b161384c746012865"],["36e4641a53948fd476c39f8a99fd974e5ec07564b5315d8bf99471bca0ef2f66","d2424b1b1abe4eb8164227b085c9aa9456ea13493fd563e06fd51cf5694c78fc"],["336581ea7bfbbb290c191a2f507a41cf5643842170e914faeab27c2c579f726","ead12168595fe1be99252129b6e56b3391f7ab1410cd1e0ef3dcdcabd2fda224"],["8ab89816dadfd6b6a1f2634fcf00ec8403781025ed6890c4849742706bd43ede","6fdcef09f2f6d0a044e654aef624136f503d459c3e89845858a47a9129cdd24e"],["1e33f1a746c9c5778133344d9299fcaa20b0938e8acff2544bb40284b8c5fb94","60660257dd11b3aa9c8ed618d24edff2306d320f1d03010e33a7d2057f3b3b6"],["85b7c1dcb3cec1b7ee7f30ded79dd20a0ed1f4cc18cbcfcfa410361fd8f08f31","3d98a9cdd026dd43f39048f25a8847f4fcafad1895d7a633c6fed3c35e999511"],["29df9fbd8d9e46509275f4b125d6d45d7fbe9a3b878a7af872a2800661ac5f51","b4c4fe99c775a606e2d8862179139ffda61dc861c019e55cd2876eb2a27d84b"],["a0b1cae06b0a847a3fea6e671aaf8adfdfe58ca2f768105c8082b2e449fce252","ae434102edde0958ec4b19d917a6a28e6b72da1834aff0e650f049503a296cf2"],["4e8ceafb9b3e9a136dc7ff67e840295b499dfb3b2133e4ba113f2e4c0e121e5","cf2174118c8b6d7a4b48f6d534ce5c79422c086a63460502b827ce62a326683c"],["d24a44e047e19b6f5afb81c7ca2f69080a5076689a010919f42725c2b789a33b","6fb8d5591b466f8fc63db50f1c0f1c69013f996887b8244d2cdec417afea8fa3"],["ea01606a7a6c9cdd249fdfcfacb99584001edd28abbab77b5104e98e8e3b35d4","322af4908c7312b0cfbfe369f7a7b3cdb7d4494bc2823700cfd652188a3ea98d"],["af8addbf2b661c8a6c6328655eb96651252007d8c5ea31be4ad196de8ce2131f","6749e67c029b85f52a034eafd096836b2520818680e26ac8f3dfbcdb71749700"],["e3ae1974566ca06cc516d47e0fb165a674a3dabcfca15e722f0e3450f45889","2aeabe7e4531510116217f07bf4d07300de97e4874f81f533420a72eeb0bd6a4"],["591ee355313d99721cf6993ffed1e3e301993ff3ed258802075ea8ced397e246","b0ea558a113c30bea60fc4775460c7901ff0b053d25ca2bdeee98f1a4be5d196"],["11396d55fda54c49f19aa97318d8da61fa8584e47b084945077cf03255b52984","998c74a8cd45ac01289d5833a7beb4744ff536b01b257be4c5767bea93ea57a4"],["3c5d2a1ba39c5a1790000738c9e0c40b8dcdfd5468754b6405540157e017aa7a","b2284279995a34e2f9d4de7396fc18b80f9b8b9fdd270f6661f79ca4c81bd257"],["cc8704b8a60a0defa3a99a7299f2e9c3fbc395afb04ac078425ef8a1793cc030","bdd46039feed17881d1e0862db347f8cf395b74fc4bcdc4e940b74e3ac1f1b13"],["c533e4f7ea8555aacd9777ac5cad29b97dd4defccc53ee7ea204119b2889b197","6f0a256bc5efdf429a2fb6242f1a43a2d9b925bb4a4b3a26bb8e0f45eb596096"],["c14f8f2ccb27d6f109f6d08d03cc96a69ba8c34eec07bbcf566d48e33da6593","c359d6923bb398f7fd4473e16fe1c28475b740dd098075e6c0e8649113dc3a38"],["a6cbc3046bc6a450bac24789fa17115a4c9739ed75f8f21ce441f72e0b90e6ef","21ae7f4680e889bb130619e2c0f95a360ceb573c70603139862afd617fa9b9f"],["347d6d9a02c48927ebfb86c1359b1caf130a3c0267d11ce6344b39f99d43cc38","60ea7f61a353524d1c987f6ecec92f086d565ab687870cb12689ff1e31c74448"],["da6545d2181db8d983f7dcb375ef5866d47c67b1bf31c8cf855ef7437b72656a","49b96715ab6878a79e78f07ce5680c5d6673051b4935bd897fea824b77dc208a"],["c40747cc9d012cb1a13b8148309c6de7ec25d6945d657146b9d5994b8feb1111","5ca560753be2a12fc6de6caf2cb489565db936156b9514e1bb5e83037e0fa2d4"],["4e42c8ec82c99798ccf3a610be870e78338c7f713348bd34c8203ef4037f3502","7571d74ee5e0fb92a7a8b33a07783341a5492144cc54bcc40a94473693606437"],["3775ab7089bc6af823aba2e1af70b236d251cadb0c86743287522a1b3b0dedea","be52d107bcfa09d8bcb9736a828cfa7fac8db17bf7a76a2c42ad961409018cf7"],["cee31cbf7e34ec379d94fb814d3d775ad954595d1314ba8846959e3e82f74e26","8fd64a14c06b589c26b947ae2bcf6bfa0149ef0be14ed4d80f448a01c43b1c6d"],["b4f9eaea09b6917619f6ea6a4eb5464efddb58fd45b1ebefcdc1a01d08b47986","39e5c9925b5a54b07433a4f18c61726f8bb131c012ca542eb24a8ac07200682a"],["d4263dfc3d2df923a0179a48966d30ce84e2515afc3dccc1b77907792ebcc60e","62dfaf07a0f78feb30e30d6295853ce189e127760ad6cf7fae164e122a208d54"],["48457524820fa65a4f8d35eb6930857c0032acc0a4a2de422233eeda897612c4","25a748ab367979d98733c38a1fa1c2e7dc6cc07db2d60a9ae7a76aaa49bd0f77"],["dfeeef1881101f2cb11644f3a2afdfc2045e19919152923f367a1767c11cceda","ecfb7056cf1de042f9420bab396793c0c390bde74b4bbdff16a83ae09a9a7517"],["6d7ef6b17543f8373c573f44e1f389835d89bcbc6062ced36c82df83b8fae859","cd450ec335438986dfefa10c57fea9bcc521a0959b2d80bbf74b190dca712d10"],["e75605d59102a5a2684500d3b991f2e3f3c88b93225547035af25af66e04541f","f5c54754a8f71ee540b9b48728473e314f729ac5308b06938360990e2bfad125"],["eb98660f4c4dfaa06a2be453d5020bc99a0c2e60abe388457dd43fefb1ed620c","6cb9a8876d9cb8520609af3add26cd20a0a7cd8a9411131ce85f44100099223e"],["13e87b027d8514d35939f2e6892b19922154596941888336dc3563e3b8dba942","fef5a3c68059a6dec5d624114bf1e91aac2b9da568d6abeb2570d55646b8adf1"],["ee163026e9fd6fe017c38f06a5be6fc125424b371ce2708e7bf4491691e5764a","1acb250f255dd61c43d94ccc670d0f58f49ae3fa15b96623e5430da0ad6c62b2"],["b268f5ef9ad51e4d78de3a750c2dc89b1e626d43505867999932e5db33af3d80","5f310d4b3c99b9ebb19f77d41c1dee018cf0d34fd4191614003e945a1216e423"],["ff07f3118a9df035e9fad85eb6c7bfe42b02f01ca99ceea3bf7ffdba93c4750d","438136d603e858a3a5c440c38eccbaddc1d2942114e2eddd4740d098ced1f0d8"],["8d8b9855c7c052a34146fd20ffb658bea4b9f69e0d825ebec16e8c3ce2b526a1","cdb559eedc2d79f926baf44fb84ea4d44bcf50fee51d7ceb30e2e7f463036758"],["52db0b5384dfbf05bfa9d472d7ae26dfe4b851ceca91b1eba54263180da32b63","c3b997d050ee5d423ebaf66a6db9f57b3180c902875679de924b69d84a7b375"],["e62f9490d3d51da6395efd24e80919cc7d0f29c3f3fa48c6fff543becbd43352","6d89ad7ba4876b0b22c2ca280c682862f342c8591f1daf5170e07bfd9ccafa7d"],["7f30ea2476b399b4957509c88f77d0191afa2ff5cb7b14fd6d8e7d65aaab1193","ca5ef7d4b231c94c3b15389a5f6311e9daff7bb67b103e9880ef4bff637acaec"],["5098ff1e1d9f14fb46a210fada6c903fef0fb7b4a1dd1d9ac60a0361800b7a00","9731141d81fc8f8084d37c6e7542006b3ee1b40d60dfe5362a5b132fd17ddc0"],["32b78c7de9ee512a72895be6b9cbefa6e2f3c4ccce445c96b9f2c81e2778ad58","ee1849f513df71e32efc3896ee28260c73bb80547ae2275ba497237794c8753c"],["e2cb74fddc8e9fbcd076eef2a7c72b0ce37d50f08269dfc074b581550547a4f7","d3aa2ed71c9dd2247a62df062736eb0baddea9e36122d2be8641abcb005cc4a4"],["8438447566d4d7bedadc299496ab357426009a35f235cb141be0d99cd10ae3a8","c4e1020916980a4da5d01ac5e6ad330734ef0d7906631c4f2390426b2edd791f"],["4162d488b89402039b584c6fc6c308870587d9c46f660b878ab65c82c711d67e","67163e903236289f776f22c25fb8a3afc1732f2b84b4e95dbda47ae5a0852649"],["3fad3fa84caf0f34f0f89bfd2dcf54fc175d767aec3e50684f3ba4a4bf5f683d","cd1bc7cb6cc407bb2f0ca647c718a730cf71872e7d0d2a53fa20efcdfe61826"],["674f2600a3007a00568c1a7ce05d0816c1fb84bf1370798f1c69532faeb1a86b","299d21f9413f33b3edf43b257004580b70db57da0b182259e09eecc69e0d38a5"],["d32f4da54ade74abb81b815ad1fb3b263d82d6c692714bcff87d29bd5ee9f08f","f9429e738b8e53b968e99016c059707782e14f4535359d582fc416910b3eea87"],["30e4e670435385556e593657135845d36fbb6931f72b08cb1ed954f1e3ce3ff6","462f9bce619898638499350113bbc9b10a878d35da70740dc695a559eb88db7b"],["be2062003c51cc3004682904330e4dee7f3dcd10b01e580bf1971b04d4cad297","62188bc49d61e5428573d48a74e1c655b1c61090905682a0d5558ed72dccb9bc"],["93144423ace3451ed29e0fb9ac2af211cb6e84a601df5993c419859fff5df04a","7c10dfb164c3425f5c71a3f9d7992038f1065224f72bb9d1d902a6d13037b47c"],["b015f8044f5fcbdcf21ca26d6c34fb8197829205c7b7d2a7cb66418c157b112c","ab8c1e086d04e813744a655b2df8d5f83b3cdc6faa3088c1d3aea1454e3a1d5f"],["d5e9e1da649d97d89e4868117a465a3a4f8a18de57a140d36b3f2af341a21b52","4cb04437f391ed73111a13cc1d4dd0db1693465c2240480d8955e8592f27447a"],["d3ae41047dd7ca065dbf8ed77b992439983005cd72e16d6f996a5316d36966bb","bd1aeb21ad22ebb22a10f0303417c6d964f8cdd7df0aca614b10dc14d125ac46"],["463e2763d885f958fc66cdd22800f0a487197d0a82e377b49f80af87c897b065","bfefacdb0e5d0fd7df3a311a94de062b26b80c61fbc97508b79992671ef7ca7f"],["7985fdfd127c0567c6f53ec1bb63ec3158e597c40bfe747c83cddfc910641917","603c12daf3d9862ef2b25fe1de289aed24ed291e0ec6708703a5bd567f32ed03"],["74a1ad6b5f76e39db2dd249410eac7f99e74c59cb83d2d0ed5ff1543da7703e9","cc6157ef18c9c63cd6193d83631bbea0093e0968942e8c33d5737fd790e0db08"],["30682a50703375f602d416664ba19b7fc9bab42c72747463a71d0896b22f6da3","553e04f6b018b4fa6c8f39e7f311d3176290d0e0f19ca73f17714d9977a22ff8"],["9e2158f0d7c0d5f26c3791efefa79597654e7a2b2464f52b1ee6c1347769ef57","712fcdd1b9053f09003a3481fa7762e9ffd7c8ef35a38509e2fbf2629008373"],["176e26989a43c9cfeba4029c202538c28172e566e3c4fce7322857f3be327d66","ed8cc9d04b29eb877d270b4878dc43c19aefd31f4eee09ee7b47834c1fa4b1c3"],["75d46efea3771e6e68abb89a13ad747ecf1892393dfc4f1b7004788c50374da8","9852390a99507679fd0b86fd2b39a868d7efc22151346e1a3ca4726586a6bed8"],["809a20c67d64900ffb698c4c825f6d5f2310fb0451c869345b7319f645605721","9e994980d9917e22b76b061927fa04143d096ccc54963e6a5ebfa5f3f8e286c1"],["1b38903a43f7f114ed4500b4eac7083fdefece1cf29c63528d563446f972c180","4036edc931a60ae889353f77fd53de4a2708b26b6f5da72ad3394119daf408f9"]]}}},{}],15:[function(e,t,i){"use strict";var r=i,f=e("bn.js"),i=e("minimalistic-assert"),e=e("minimalistic-crypto-utils");r.assert=i,r.toArray=e.toArray,r.zero2=e.zero2,r.toHex=e.toHex,r.encode=e.encode,r.getNAF=function(e,t,i){var r=new Array(Math.max(e.bitLength(),i)+1);r.fill(0);for(var f=1<<t+1,d=e.clone(),n=0;n<r.length;n++){var a,s=d.andln(f-1);d.isOdd()?(a=(f>>1)-1<s?(f>>1)-s:s,d.isubn(a)):a=0,r[n]=a,d.iushrn(1)}return r},r.getJSF=function(e,t){var i=[[],[]];e=e.clone(),t=t.clone();for(var r,f=0,d=0;0<e.cmpn(-f)||0<t.cmpn(-d);){var n,a=e.andln(3)+f&3,s=t.andln(3)+d&3;3===s&&(s=-1),n=0==(1&(a=3===a?-1:a))?0:3!==(r=e.andln(7)+f&7)&&5!==r||2!==s?a:-a,i[0].push(n),s=0==(1&s)?0:3!==(r=t.andln(7)+d&7)&&5!==r||2!==a?s:-s,i[1].push(s),2*f===n+1&&(f=1-f),2*d===s+1&&(d=1-d),e.iushrn(1),t.iushrn(1)}return i},r.cachedProperty=function(e,t,i){var r="_"+t;e.prototype[t]=function(){return void 0!==this[r]?this[r]:this[r]=i.call(this)}},r.parseBytes=function(e){return"string"==typeof e?r.toArray(e,"hex"):e},r.intFromLE=function(e){return new f(e,"hex","le")}},{"bn.js":16,"minimalistic-assert":33,"minimalistic-crypto-utils":34}],16:[function(_,e,t){!function(e,t){"use strict";function p(e,t){if(!e)throw new Error(t||"Assertion failed")}function i(e,t){e.super_=t;function i(){}i.prototype=t.prototype,e.prototype=new i,e.prototype.constructor=e}function m(e,t,i){if(m.isBN(e))return e;this.negative=0,this.words=null,this.length=0,(this.red=null)!==e&&("le"!==t&&"be"!==t||(i=t,t=10),this._init(e||0,t||10,i||"be"))}var r;"object"==typeof e?e.exports=m:t.BN=m,(m.BN=m).wordSize=26;try{r=_("buffer").Buffer}catch(e){}function n(e,t,i){for(var r=0,f=Math.min(e.length,i),d=t;d<f;d++){var n=e.charCodeAt(d)-48;r<<=4,r|=49<=n&&n<=54?n-49+10:17<=n&&n<=22?n-17+10:15&n}return r}function o(e,t,i,r){for(var f=0,d=Math.min(e.length,i),n=t;n<d;n++){var a=e.charCodeAt(n)-48;f*=r,f+=49<=a?a-49+10:17<=a?a-17+10:a}return f}m.isBN=function(e){return e instanceof m||null!==e&&"object"==typeof e&&e.constructor.wordSize===m.wordSize&&Array.isArray(e.words)},m.max=function(e,t){return 0<e.cmp(t)?e:t},m.min=function(e,t){return e.cmp(t)<0?e:t},m.prototype._init=function(e,t,i){if("number"==typeof e)return this._initNumber(e,t,i);if("object"==typeof e)return this._initArray(e,t,i);p((t="hex"===t?16:t)===(0|t)&&2<=t&&t<=36);var r=0;"-"===(e=e.toString().replace(/\s+/g,""))[0]&&r++,16===t?this._parseHex(e,r):this._parseBase(e,t,r),"-"===e[0]&&(this.negative=1),this.strip(),"le"===i&&this._initArray(this.toArray(),t,i)},m.prototype._initNumber=function(e,t,i){e<0&&(this.negative=1,e=-e),e<67108864?(this.words=[67108863&e],this.length=1):e<4503599627370496?(this.words=[67108863&e,e/67108864&67108863],this.length=2):(p(e<9007199254740992),this.words=[67108863&e,e/67108864&67108863,1],this.length=3),"le"===i&&this._initArray(this.toArray(),t,i)},m.prototype._initArray=function(e,t,i){if(p("number"==typeof e.length),e.length<=0)return this.words=[0],this.length=1,this;this.length=Math.ceil(e.length/3),this.words=new Array(this.length);for(var r,f,d=0;d<this.length;d++)this.words[d]=0;var n=0;if("be"===i)for(d=e.length-1,r=0;0<=d;d-=3)f=e[d]|e[d-1]<<8|e[d-2]<<16,this.words[r]|=f<<n&67108863,this.words[r+1]=f>>>26-n&67108863,26<=(n+=24)&&(n-=26,r++);else if("le"===i)for(r=d=0;d<e.length;d+=3)f=e[d]|e[d+1]<<8|e[d+2]<<16,this.words[r]|=f<<n&67108863,this.words[r+1]=f>>>26-n&67108863,26<=(n+=24)&&(n-=26,r++);return this.strip()},m.prototype._parseHex=function(e,t){this.length=Math.ceil((e.length-t)/6),this.words=new Array(this.length);for(var i,r=0;r<this.length;r++)this.words[r]=0;for(var f=0,r=e.length-6,d=0;t<=r;r-=6)i=n(e,r,r+6),this.words[d]|=i<<f&67108863,this.words[d+1]|=i>>>26-f&4194303,26<=(f+=24)&&(f-=26,d++);r+6!==t&&(i=n(e,t,r+6),this.words[d]|=i<<f&67108863,this.words[d+1]|=i>>>26-f&4194303),this.strip()},m.prototype._parseBase=function(e,t,i){this.words=[0];for(var r=0,f=this.length=1;f<=67108863;f*=t)r++;r--,f=f/t|0;for(var d=e.length-i,n=d%r,a=Math.min(d,d-n)+i,s=0,c=i;c<a;c+=r)s=o(e,c,c+r,t),this.imuln(f),this.words[0]+s<67108864?this.words[0]+=s:this._iaddn(s);if(0!=n){for(var h=1,s=o(e,c,e.length,t),c=0;c<n;c++)h*=t;this.imuln(h),this.words[0]+s<67108864?this.words[0]+=s:this._iaddn(s)}},m.prototype.copy=function(e){e.words=new Array(this.length);for(var t=0;t<this.length;t++)e.words[t]=this.words[t];e.length=this.length,e.negative=this.negative,e.red=this.red},m.prototype.clone=function(){var e=new m(null);return this.copy(e),e},m.prototype._expand=function(e){for(;this.length<e;)this.words[this.length++]=0;return this},m.prototype.strip=function(){for(;1<this.length&&0===this.words[this.length-1];)this.length--;return this._normSign()},m.prototype._normSign=function(){return 1===this.length&&0===this.words[0]&&(this.negative=0),this},m.prototype.inspect=function(){return(this.red?"<BN-R: ":"<BN: ")+this.toString(16)+">"};var u=["","0","00","000","0000","00000","000000","0000000","00000000","000000000","0000000000","00000000000","000000000000","0000000000000","00000000000000","000000000000000","0000000000000000","00000000000000000","000000000000000000","0000000000000000000","00000000000000000000","000000000000000000000","0000000000000000000000","00000000000000000000000","000000000000000000000000","0000000000000000000000000"],b=[0,0,25,16,12,11,10,9,8,8,7,7,7,7,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5],l=[0,0,33554432,43046721,16777216,48828125,60466176,40353607,16777216,43046721,1e7,19487171,35831808,62748517,7529536,11390625,16777216,24137569,34012224,47045881,64e6,4084101,5153632,6436343,7962624,9765625,11881376,14348907,17210368,20511149,243e5,28629151,33554432,39135393,45435424,52521875,60466176];function f(e,t,i){i.negative=t.negative^e.negative;var r=e.length+t.length|0,r=(i.length=r)-1|0,f=67108863&(o=(0|e.words[0])*(0|t.words[0])),d=o/67108864|0;i.words[0]=f;for(var n=1;n<r;n++){for(var a=d>>>26,s=67108863&d,c=Math.min(n,t.length-1),h=Math.max(0,n-e.length+1);h<=c;h++){var o,u=n-h|0;a+=(o=(0|e.words[u])*(0|t.words[h])+s)/67108864|0,s=67108863&o}i.words[n]=0|s,d=0|a}return 0!==d?i.words[n]=0|d:i.length--,i.strip()}m.prototype.toString=function(e,t){if(t=0|t||1,16===(e=e||10)||"hex"===e){a="";for(var i=0,r=0,f=0;f<this.length;f++){var d=this.words[f],n=(16777215&(d<<i|r)).toString(16),a=0!==(r=d>>>24-i&16777215)||f!==this.length-1?u[6-n.length]+n+a:n+a;26<=(i+=2)&&(i-=26,f--)}for(0!==r&&(a=r.toString(16)+a);a.length%t!=0;)a="0"+a;return a=0!==this.negative?"-"+a:a}if(e===(0|e)&&2<=e&&e<=36){var s=b[e],c=l[e];for(a="",(h=this.clone()).negative=0;!h.isZero();){var h,o=h.modn(c).toString(e);a=(h=h.idivn(c)).isZero()?o+a:u[s-o.length]+o+a}for(this.isZero()&&(a="0"+a);a.length%t!=0;)a="0"+a;return a=0!==this.negative?"-"+a:a}p(!1,"Base should be between 2 and 36")},m.prototype.toNumber=function(){var e=this.words[0];return 2===this.length?e+=67108864*this.words[1]:3===this.length&&1===this.words[2]?e+=4503599627370496+67108864*this.words[1]:2<this.length&&p(!1,"Number can only safely store up to 53 bits"),0!==this.negative?-e:e},m.prototype.toJSON=function(){return this.toString(16)},m.prototype.toBuffer=function(e,t){return p(void 0!==r),this.toArrayLike(r,e,t)},m.prototype.toArray=function(e,t){return this.toArrayLike(Array,e,t)},m.prototype.toArrayLike=function(e,t,i){var r=this.byteLength(),f=i||Math.max(1,r);p(r<=f,"byte array longer than desired length"),p(0<f,"Requested array length <= 0"),this.strip();var d,n,t="le"===t,a=new e(f),s=this.clone();if(t){for(n=0;!s.isZero();n++)d=s.andln(255),s.iushrn(8),a[n]=d;for(;n<f;n++)a[n]=0}else{for(n=0;n<f-r;n++)a[n]=0;for(n=0;!s.isZero();n++)d=s.andln(255),s.iushrn(8),a[f-n-1]=d}return a},Math.clz32?m.prototype._countBits=function(e){return 32-Math.clz32(e)}:m.prototype._countBits=function(e){var t=e,e=0;return 4096<=t&&(e+=13,t>>>=13),64<=t&&(e+=7,t>>>=7),8<=t&&(e+=4,t>>>=4),2<=t&&(e+=2,t>>>=2),e+t},m.prototype._zeroBits=function(e){if(0===e)return 26;var t=e,e=0;return 0==(8191&t)&&(e+=13,t>>>=13),0==(127&t)&&(e+=7,t>>>=7),0==(15&t)&&(e+=4,t>>>=4),0==(3&t)&&(e+=2,t>>>=2),0==(1&t)&&e++,e},m.prototype.bitLength=function(){var e=this.words[this.length-1],e=this._countBits(e);return 26*(this.length-1)+e},m.prototype.zeroBits=function(){if(this.isZero())return 0;for(var e=0,t=0;t<this.length;t++){var i=this._zeroBits(this.words[t]);if(e+=i,26!==i)break}return e},m.prototype.byteLength=function(){return Math.ceil(this.bitLength()/8)},m.prototype.toTwos=function(e){return 0!==this.negative?this.abs().inotn(e).iaddn(1):this.clone()},m.prototype.fromTwos=function(e){return this.testn(e-1)?this.notn(e).iaddn(1).ineg():this.clone()},m.prototype.isNeg=function(){return 0!==this.negative},m.prototype.neg=function(){return this.clone().ineg()},m.prototype.ineg=function(){return this.isZero()||(this.negative^=1),this},m.prototype.iuor=function(e){for(;this.length<e.length;)this.words[this.length++]=0;for(var t=0;t<e.length;t++)this.words[t]=this.words[t]|e.words[t];return this.strip()},m.prototype.ior=function(e){return p(0==(this.negative|e.negative)),this.iuor(e)},m.prototype.or=function(e){return this.length>e.length?this.clone().ior(e):e.clone().ior(this)},m.prototype.uor=function(e){return this.length>e.length?this.clone().iuor(e):e.clone().iuor(this)},m.prototype.iuand=function(e){for(var t=this.length>e.length?e:this,i=0;i<t.length;i++)this.words[i]=this.words[i]&e.words[i];return this.length=t.length,this.strip()},m.prototype.iand=function(e){return p(0==(this.negative|e.negative)),this.iuand(e)},m.prototype.and=function(e){return this.length>e.length?this.clone().iand(e):e.clone().iand(this)},m.prototype.uand=function(e){return this.length>e.length?this.clone().iuand(e):e.clone().iuand(this)},m.prototype.iuxor=function(e){for(var t,i=this.length>e.length?(t=this,e):(t=e,this),r=0;r<i.length;r++)this.words[r]=t.words[r]^i.words[r];if(this!==t)for(;r<t.length;r++)this.words[r]=t.words[r];return this.length=t.length,this.strip()},m.prototype.ixor=function(e){return p(0==(this.negative|e.negative)),this.iuxor(e)},m.prototype.xor=function(e){return this.length>e.length?this.clone().ixor(e):e.clone().ixor(this)},m.prototype.uxor=function(e){return this.length>e.length?this.clone().iuxor(e):e.clone().iuxor(this)},m.prototype.inotn=function(e){p("number"==typeof e&&0<=e);var t=0|Math.ceil(e/26),e=e%26;this._expand(t),0<e&&t--;for(var i=0;i<t;i++)this.words[i]=67108863&~this.words[i];return 0<e&&(this.words[i]=~this.words[i]&67108863>>26-e),this.strip()},m.prototype.notn=function(e){return this.clone().inotn(e)},m.prototype.setn=function(e,t){p("number"==typeof e&&0<=e);var i=e/26|0,e=e%26;return this._expand(1+i),this.words[i]=t?this.words[i]|1<<e:this.words[i]&~(1<<e),this.strip()},m.prototype.iadd=function(e){var t,i,r;if(0!==this.negative&&0===e.negative)return this.negative=0,t=this.isub(e),this.negative^=1,this._normSign();if(0===this.negative&&0!==e.negative)return e.negative=0,t=this.isub(e),e.negative=1,t._normSign();r=this.length>e.length?(i=this,e):(i=e,this);for(var f=0,d=0;d<r.length;d++)t=(0|i.words[d])+(0|r.words[d])+f,this.words[d]=67108863&t,f=t>>>26;for(;0!==f&&d<i.length;d++)t=(0|i.words[d])+f,this.words[d]=67108863&t,f=t>>>26;if(this.length=i.length,0!==f)this.words[this.length]=f,this.length++;else if(i!==this)for(;d<i.length;d++)this.words[d]=i.words[d];return this},m.prototype.add=function(e){var t;return 0!==e.negative&&0===this.negative?(e.negative=0,t=this.sub(e),e.negative^=1,t):0===e.negative&&0!==this.negative?(this.negative=0,t=e.sub(this),this.negative=1,t):this.length>e.length?this.clone().iadd(e):e.clone().iadd(this)},m.prototype.isub=function(e){if(0!==e.negative){e.negative=0;var t=this.iadd(e);return e.negative=1,t._normSign()}if(0!==this.negative)return this.negative=0,this.iadd(e),this.negative=1,this._normSign();var i,r,f=this.cmp(e);if(0===f)return this.negative=0,this.length=1,this.words[0]=0,this;r=0<f?(i=this,e):(i=e,this);for(var d=0,n=0;n<r.length;n++)d=(t=(0|i.words[n])-(0|r.words[n])+d)>>26,this.words[n]=67108863&t;for(;0!==d&&n<i.length;n++)d=(t=(0|i.words[n])+d)>>26,this.words[n]=67108863&t;if(0===d&&n<i.length&&i!==this)for(;n<i.length;n++)this.words[n]=i.words[n];return this.length=Math.max(this.length,n),i!==this&&(this.negative=1),this.strip()},m.prototype.sub=function(e){return this.clone().isub(e)};var d=function(e,t,i){var r=e.words,f=t.words,d=i.words,n=0|r[0],a=8191&n,s=n>>>13,c=0|r[1],h=8191&c,o=c>>>13,u=0|r[2],b=8191&u,l=u>>>13,p=0|r[3],m=8191&p,v=p>>>13,g=0|r[4],y=8191&g,M=g>>>13,w=0|r[5],S=8191&w,_=w>>>13,A=0|r[6],x=8191&A,I=A>>>13,z=0|r[7],q=8191&z,R=z>>>13,k=0|r[8],P=8191&k,j=k>>>13,N=0|r[9],E=8191&N,B=N>>>13,L=0|f[0],O=8191&L,F=L>>>13,T=0|f[1],C=8191&T,Z=T>>>13,J=0|f[2],H=8191&J,D=J>>>13,X=0|f[3],K=8191&X,V=X>>>13,W=0|f[4],U=8191&W,Y=W>>>13,G=0|f[5],Q=8191&G,$=G>>>13,n=0|f[6],c=8191&n,u=n>>>13,p=0|f[7],g=8191&p,w=p>>>13,A=0|f[8],z=8191&A,k=A>>>13,r=0|f[9],N=8191&r,L=r>>>13;i.negative=e.negative^t.negative,i.length=19;var X=(0+Math.imul(a,O)|0)+((8191&(J=Math.imul(a,F)+Math.imul(s,O)|0))<<13)|0,ee=(Math.imul(s,F)+(J>>>13)|0)+(X>>>26)|0;X&=67108863,T=Math.imul(h,O),J=Math.imul(h,F)+Math.imul(o,O)|0,W=Math.imul(o,F);G=(ee+(T+Math.imul(a,C)|0)|0)+((8191&(J=(J+Math.imul(a,Z)|0)+Math.imul(s,C)|0))<<13)|0;ee=((W+Math.imul(s,Z)|0)+(J>>>13)|0)+(G>>>26)|0,G&=67108863,T=Math.imul(b,O),J=Math.imul(b,F)+Math.imul(l,O)|0,W=Math.imul(l,F),T=T+Math.imul(h,C)|0,J=(J+Math.imul(h,Z)|0)+Math.imul(o,C)|0,W=W+Math.imul(o,Z)|0;n=(ee+(T+Math.imul(a,H)|0)|0)+((8191&(J=(J+Math.imul(a,D)|0)+Math.imul(s,H)|0))<<13)|0;ee=((W+Math.imul(s,D)|0)+(J>>>13)|0)+(n>>>26)|0,n&=67108863,T=Math.imul(m,O),J=Math.imul(m,F)+Math.imul(v,O)|0,W=Math.imul(v,F),T=T+Math.imul(b,C)|0,J=(J+Math.imul(b,Z)|0)+Math.imul(l,C)|0,W=W+Math.imul(l,Z)|0,T=T+Math.imul(h,H)|0,J=(J+Math.imul(h,D)|0)+Math.imul(o,H)|0,W=W+Math.imul(o,D)|0;p=(ee+(T+Math.imul(a,K)|0)|0)+((8191&(J=(J+Math.imul(a,V)|0)+Math.imul(s,K)|0))<<13)|0;ee=((W+Math.imul(s,V)|0)+(J>>>13)|0)+(p>>>26)|0,p&=67108863,T=Math.imul(y,O),J=Math.imul(y,F)+Math.imul(M,O)|0,W=Math.imul(M,F),T=T+Math.imul(m,C)|0,J=(J+Math.imul(m,Z)|0)+Math.imul(v,C)|0,W=W+Math.imul(v,Z)|0,T=T+Math.imul(b,H)|0,J=(J+Math.imul(b,D)|0)+Math.imul(l,H)|0,W=W+Math.imul(l,D)|0,T=T+Math.imul(h,K)|0,J=(J+Math.imul(h,V)|0)+Math.imul(o,K)|0,W=W+Math.imul(o,V)|0;A=(ee+(T+Math.imul(a,U)|0)|0)+((8191&(J=(J+Math.imul(a,Y)|0)+Math.imul(s,U)|0))<<13)|0;ee=((W+Math.imul(s,Y)|0)+(J>>>13)|0)+(A>>>26)|0,A&=67108863,T=Math.imul(S,O),J=Math.imul(S,F)+Math.imul(_,O)|0,W=Math.imul(_,F),T=T+Math.imul(y,C)|0,J=(J+Math.imul(y,Z)|0)+Math.imul(M,C)|0,W=W+Math.imul(M,Z)|0,T=T+Math.imul(m,H)|0,J=(J+Math.imul(m,D)|0)+Math.imul(v,H)|0,W=W+Math.imul(v,D)|0,T=T+Math.imul(b,K)|0,J=(J+Math.imul(b,V)|0)+Math.imul(l,K)|0,W=W+Math.imul(l,V)|0,T=T+Math.imul(h,U)|0,J=(J+Math.imul(h,Y)|0)+Math.imul(o,U)|0,W=W+Math.imul(o,Y)|0;f=(ee+(T+Math.imul(a,Q)|0)|0)+((8191&(J=(J+Math.imul(a,$)|0)+Math.imul(s,Q)|0))<<13)|0;ee=((W+Math.imul(s,$)|0)+(J>>>13)|0)+(f>>>26)|0,f&=67108863,T=Math.imul(x,O),J=Math.imul(x,F)+Math.imul(I,O)|0,W=Math.imul(I,F),T=T+Math.imul(S,C)|0,J=(J+Math.imul(S,Z)|0)+Math.imul(_,C)|0,W=W+Math.imul(_,Z)|0,T=T+Math.imul(y,H)|0,J=(J+Math.imul(y,D)|0)+Math.imul(M,H)|0,W=W+Math.imul(M,D)|0,T=T+Math.imul(m,K)|0,J=(J+Math.imul(m,V)|0)+Math.imul(v,K)|0,W=W+Math.imul(v,V)|0,T=T+Math.imul(b,U)|0,J=(J+Math.imul(b,Y)|0)+Math.imul(l,U)|0,W=W+Math.imul(l,Y)|0,T=T+Math.imul(h,Q)|0,J=(J+Math.imul(h,$)|0)+Math.imul(o,Q)|0,W=W+Math.imul(o,$)|0;r=(ee+(T+Math.imul(a,c)|0)|0)+((8191&(J=(J+Math.imul(a,u)|0)+Math.imul(s,c)|0))<<13)|0;ee=((W+Math.imul(s,u)|0)+(J>>>13)|0)+(r>>>26)|0,r&=67108863,T=Math.imul(q,O),J=Math.imul(q,F)+Math.imul(R,O)|0,W=Math.imul(R,F),T=T+Math.imul(x,C)|0,J=(J+Math.imul(x,Z)|0)+Math.imul(I,C)|0,W=W+Math.imul(I,Z)|0,T=T+Math.imul(S,H)|0,J=(J+Math.imul(S,D)|0)+Math.imul(_,H)|0,W=W+Math.imul(_,D)|0,T=T+Math.imul(y,K)|0,J=(J+Math.imul(y,V)|0)+Math.imul(M,K)|0,W=W+Math.imul(M,V)|0,T=T+Math.imul(m,U)|0,J=(J+Math.imul(m,Y)|0)+Math.imul(v,U)|0,W=W+Math.imul(v,Y)|0,T=T+Math.imul(b,Q)|0,J=(J+Math.imul(b,$)|0)+Math.imul(l,Q)|0,W=W+Math.imul(l,$)|0,T=T+Math.imul(h,c)|0,J=(J+Math.imul(h,u)|0)+Math.imul(o,c)|0,W=W+Math.imul(o,u)|0;e=(ee+(T+Math.imul(a,g)|0)|0)+((8191&(J=(J+Math.imul(a,w)|0)+Math.imul(s,g)|0))<<13)|0;ee=((W+Math.imul(s,w)|0)+(J>>>13)|0)+(e>>>26)|0,e&=67108863,T=Math.imul(P,O),J=Math.imul(P,F)+Math.imul(j,O)|0,W=Math.imul(j,F),T=T+Math.imul(q,C)|0,J=(J+Math.imul(q,Z)|0)+Math.imul(R,C)|0,W=W+Math.imul(R,Z)|0,T=T+Math.imul(x,H)|0,J=(J+Math.imul(x,D)|0)+Math.imul(I,H)|0,W=W+Math.imul(I,D)|0,T=T+Math.imul(S,K)|0,J=(J+Math.imul(S,V)|0)+Math.imul(_,K)|0,W=W+Math.imul(_,V)|0,T=T+Math.imul(y,U)|0,J=(J+Math.imul(y,Y)|0)+Math.imul(M,U)|0,W=W+Math.imul(M,Y)|0,T=T+Math.imul(m,Q)|0,J=(J+Math.imul(m,$)|0)+Math.imul(v,Q)|0,W=W+Math.imul(v,$)|0,T=T+Math.imul(b,c)|0,J=(J+Math.imul(b,u)|0)+Math.imul(l,c)|0,W=W+Math.imul(l,u)|0,T=T+Math.imul(h,g)|0,J=(J+Math.imul(h,w)|0)+Math.imul(o,g)|0,W=W+Math.imul(o,w)|0;t=(ee+(T+Math.imul(a,z)|0)|0)+((8191&(J=(J+Math.imul(a,k)|0)+Math.imul(s,z)|0))<<13)|0;ee=((W+Math.imul(s,k)|0)+(J>>>13)|0)+(t>>>26)|0,t&=67108863,T=Math.imul(E,O),J=Math.imul(E,F)+Math.imul(B,O)|0,W=Math.imul(B,F),T=T+Math.imul(P,C)|0,J=(J+Math.imul(P,Z)|0)+Math.imul(j,C)|0,W=W+Math.imul(j,Z)|0,T=T+Math.imul(q,H)|0,J=(J+Math.imul(q,D)|0)+Math.imul(R,H)|0,W=W+Math.imul(R,D)|0,T=T+Math.imul(x,K)|0,J=(J+Math.imul(x,V)|0)+Math.imul(I,K)|0,W=W+Math.imul(I,V)|0,T=T+Math.imul(S,U)|0,J=(J+Math.imul(S,Y)|0)+Math.imul(_,U)|0,W=W+Math.imul(_,Y)|0,T=T+Math.imul(y,Q)|0,J=(J+Math.imul(y,$)|0)+Math.imul(M,Q)|0,W=W+Math.imul(M,$)|0,T=T+Math.imul(m,c)|0,J=(J+Math.imul(m,u)|0)+Math.imul(v,c)|0,W=W+Math.imul(v,u)|0,T=T+Math.imul(b,g)|0,J=(J+Math.imul(b,w)|0)+Math.imul(l,g)|0,W=W+Math.imul(l,w)|0,T=T+Math.imul(h,z)|0,J=(J+Math.imul(h,k)|0)+Math.imul(o,z)|0,W=W+Math.imul(o,k)|0;a=(ee+(T+Math.imul(a,N)|0)|0)+((8191&(J=(J+Math.imul(a,L)|0)+Math.imul(s,N)|0))<<13)|0;ee=((W+Math.imul(s,L)|0)+(J>>>13)|0)+(a>>>26)|0,a&=67108863,T=Math.imul(E,C),J=Math.imul(E,Z)+Math.imul(B,C)|0,W=Math.imul(B,Z),T=T+Math.imul(P,H)|0,J=(J+Math.imul(P,D)|0)+Math.imul(j,H)|0,W=W+Math.imul(j,D)|0,T=T+Math.imul(q,K)|0,J=(J+Math.imul(q,V)|0)+Math.imul(R,K)|0,W=W+Math.imul(R,V)|0,T=T+Math.imul(x,U)|0,J=(J+Math.imul(x,Y)|0)+Math.imul(I,U)|0,W=W+Math.imul(I,Y)|0,T=T+Math.imul(S,Q)|0,J=(J+Math.imul(S,$)|0)+Math.imul(_,Q)|0,W=W+Math.imul(_,$)|0,T=T+Math.imul(y,c)|0,J=(J+Math.imul(y,u)|0)+Math.imul(M,c)|0,W=W+Math.imul(M,u)|0,T=T+Math.imul(m,g)|0,J=(J+Math.imul(m,w)|0)+Math.imul(v,g)|0,W=W+Math.imul(v,w)|0,T=T+Math.imul(b,z)|0,J=(J+Math.imul(b,k)|0)+Math.imul(l,z)|0,W=W+Math.imul(l,k)|0;h=(ee+(T+Math.imul(h,N)|0)|0)+((8191&(J=(J+Math.imul(h,L)|0)+Math.imul(o,N)|0))<<13)|0;ee=((W+Math.imul(o,L)|0)+(J>>>13)|0)+(h>>>26)|0,h&=67108863,T=Math.imul(E,H),J=Math.imul(E,D)+Math.imul(B,H)|0,W=Math.imul(B,D),T=T+Math.imul(P,K)|0,J=(J+Math.imul(P,V)|0)+Math.imul(j,K)|0,W=W+Math.imul(j,V)|0,T=T+Math.imul(q,U)|0,J=(J+Math.imul(q,Y)|0)+Math.imul(R,U)|0,W=W+Math.imul(R,Y)|0,T=T+Math.imul(x,Q)|0,J=(J+Math.imul(x,$)|0)+Math.imul(I,Q)|0,W=W+Math.imul(I,$)|0,T=T+Math.imul(S,c)|0,J=(J+Math.imul(S,u)|0)+Math.imul(_,c)|0,W=W+Math.imul(_,u)|0,T=T+Math.imul(y,g)|0,J=(J+Math.imul(y,w)|0)+Math.imul(M,g)|0,W=W+Math.imul(M,w)|0,T=T+Math.imul(m,z)|0,J=(J+Math.imul(m,k)|0)+Math.imul(v,z)|0,W=W+Math.imul(v,k)|0;b=(ee+(T+Math.imul(b,N)|0)|0)+((8191&(J=(J+Math.imul(b,L)|0)+Math.imul(l,N)|0))<<13)|0;ee=((W+Math.imul(l,L)|0)+(J>>>13)|0)+(b>>>26)|0,b&=67108863,T=Math.imul(E,K),J=Math.imul(E,V)+Math.imul(B,K)|0,W=Math.imul(B,V),T=T+Math.imul(P,U)|0,J=(J+Math.imul(P,Y)|0)+Math.imul(j,U)|0,W=W+Math.imul(j,Y)|0,T=T+Math.imul(q,Q)|0,J=(J+Math.imul(q,$)|0)+Math.imul(R,Q)|0,W=W+Math.imul(R,$)|0,T=T+Math.imul(x,c)|0,J=(J+Math.imul(x,u)|0)+Math.imul(I,c)|0,W=W+Math.imul(I,u)|0,T=T+Math.imul(S,g)|0,J=(J+Math.imul(S,w)|0)+Math.imul(_,g)|0,W=W+Math.imul(_,w)|0,T=T+Math.imul(y,z)|0,J=(J+Math.imul(y,k)|0)+Math.imul(M,z)|0,W=W+Math.imul(M,k)|0;m=(ee+(T+Math.imul(m,N)|0)|0)+((8191&(J=(J+Math.imul(m,L)|0)+Math.imul(v,N)|0))<<13)|0;ee=((W+Math.imul(v,L)|0)+(J>>>13)|0)+(m>>>26)|0,m&=67108863,T=Math.imul(E,U),J=Math.imul(E,Y)+Math.imul(B,U)|0,W=Math.imul(B,Y),T=T+Math.imul(P,Q)|0,J=(J+Math.imul(P,$)|0)+Math.imul(j,Q)|0,W=W+Math.imul(j,$)|0,T=T+Math.imul(q,c)|0,J=(J+Math.imul(q,u)|0)+Math.imul(R,c)|0,W=W+Math.imul(R,u)|0,T=T+Math.imul(x,g)|0,J=(J+Math.imul(x,w)|0)+Math.imul(I,g)|0,W=W+Math.imul(I,w)|0,T=T+Math.imul(S,z)|0,J=(J+Math.imul(S,k)|0)+Math.imul(_,z)|0,W=W+Math.imul(_,k)|0;y=(ee+(T+Math.imul(y,N)|0)|0)+((8191&(J=(J+Math.imul(y,L)|0)+Math.imul(M,N)|0))<<13)|0;ee=((W+Math.imul(M,L)|0)+(J>>>13)|0)+(y>>>26)|0,y&=67108863,T=Math.imul(E,Q),J=Math.imul(E,$)+Math.imul(B,Q)|0,W=Math.imul(B,$),T=T+Math.imul(P,c)|0,J=(J+Math.imul(P,u)|0)+Math.imul(j,c)|0,W=W+Math.imul(j,u)|0,T=T+Math.imul(q,g)|0,J=(J+Math.imul(q,w)|0)+Math.imul(R,g)|0,W=W+Math.imul(R,w)|0,T=T+Math.imul(x,z)|0,J=(J+Math.imul(x,k)|0)+Math.imul(I,z)|0,W=W+Math.imul(I,k)|0;S=(ee+(T+Math.imul(S,N)|0)|0)+((8191&(J=(J+Math.imul(S,L)|0)+Math.imul(_,N)|0))<<13)|0;ee=((W+Math.imul(_,L)|0)+(J>>>13)|0)+(S>>>26)|0,S&=67108863,T=Math.imul(E,c),J=Math.imul(E,u)+Math.imul(B,c)|0,W=Math.imul(B,u),T=T+Math.imul(P,g)|0,J=(J+Math.imul(P,w)|0)+Math.imul(j,g)|0,W=W+Math.imul(j,w)|0,T=T+Math.imul(q,z)|0,J=(J+Math.imul(q,k)|0)+Math.imul(R,z)|0,W=W+Math.imul(R,k)|0;x=(ee+(T+Math.imul(x,N)|0)|0)+((8191&(J=(J+Math.imul(x,L)|0)+Math.imul(I,N)|0))<<13)|0;ee=((W+Math.imul(I,L)|0)+(J>>>13)|0)+(x>>>26)|0,x&=67108863,T=Math.imul(E,g),J=Math.imul(E,w)+Math.imul(B,g)|0,W=Math.imul(B,w),T=T+Math.imul(P,z)|0,J=(J+Math.imul(P,k)|0)+Math.imul(j,z)|0,W=W+Math.imul(j,k)|0;q=(ee+(T+Math.imul(q,N)|0)|0)+((8191&(J=(J+Math.imul(q,L)|0)+Math.imul(R,N)|0))<<13)|0;ee=((W+Math.imul(R,L)|0)+(J>>>13)|0)+(q>>>26)|0,q&=67108863,T=Math.imul(E,z),J=Math.imul(E,k)+Math.imul(B,z)|0,W=Math.imul(B,k);P=(ee+(T+Math.imul(P,N)|0)|0)+((8191&(J=(J+Math.imul(P,L)|0)+Math.imul(j,N)|0))<<13)|0;ee=((W+Math.imul(j,L)|0)+(J>>>13)|0)+(P>>>26)|0,P&=67108863;N=(ee+Math.imul(E,N)|0)+((8191&(J=Math.imul(E,L)+Math.imul(B,N)|0))<<13)|0;return ee=(Math.imul(B,L)+(J>>>13)|0)+(N>>>26)|0,N&=67108863,d[0]=X,d[1]=G,d[2]=n,d[3]=p,d[4]=A,d[5]=f,d[6]=r,d[7]=e,d[8]=t,d[9]=a,d[10]=h,d[11]=b,d[12]=m,d[13]=y,d[14]=S,d[15]=x,d[16]=q,d[17]=P,d[18]=N,0!=ee&&(d[19]=ee,i.length++),i};function a(e,t,i){return(new s).mulp(e,t,i)}function s(e,t){this.x=e,this.y=t}Math.imul||(d=f),m.prototype.mulTo=function(e,t){var i=this.length+e.length,t=(10===this.length&&10===e.length?d:i<63?f:i<1024?function(e,t,i){i.negative=t.negative^e.negative,i.length=e.length+t.length;for(var r=0,f=0,d=0;d<i.length-1;d++){for(var n=f,f=0,a=67108863&r,s=Math.min(d,t.length-1),c=Math.max(0,d-e.length+1);c<=s;c++){var h=d-c,o=(0|e.words[h])*(0|t.words[c]),h=67108863&o,a=67108863&(h=h+a|0);f+=(n=(n=n+(o/67108864|0)|0)+(h>>>26)|0)>>>26,n&=67108863}i.words[d]=a,r=n,n=f}return 0!==r?i.words[d]=r:i.length--,i.strip()}:a)(this,e,t);return t},s.prototype.makeRBT=function(e){for(var t=new Array(e),i=m.prototype._countBits(e)-1,r=0;r<e;r++)t[r]=this.revBin(r,i,e);return t},s.prototype.revBin=function(e,t,i){if(0===e||e===i-1)return e;for(var r=0,f=0;f<t;f++)r|=(1&e)<<t-f-1,e>>=1;return r},s.prototype.permute=function(e,t,i,r,f,d){for(var n=0;n<d;n++)r[n]=t[e[n]],f[n]=i[e[n]]},s.prototype.transform=function(e,t,i,r,f,d){this.permute(d,e,t,i,r,f);for(var n=1;n<f;n<<=1)for(var a=n<<1,s=Math.cos(2*Math.PI/a),c=Math.sin(2*Math.PI/a),h=0;h<f;h+=a)for(var o=s,u=c,b=0;b<n;b++){var l=i[h+b],p=r[h+b],m=o*(g=i[h+b+n])-u*(v=r[h+b+n]),v=o*v+u*g,g=m;i[h+b]=l+g,r[h+b]=p+v,i[h+b+n]=l-g,r[h+b+n]=p-v,b!==a&&(m=s*o-c*u,u=s*u+c*o,o=m)}},s.prototype.guessLen13b=function(e,t){for(var e=1&(r=1|Math.max(t,e)),i=0,r=r/2|0;r;r>>>=1)i++;return 1<<i+1+e},s.prototype.conjugate=function(e,t,i){if(!(i<=1))for(var r=0;r<i/2;r++){var f=e[r];e[r]=e[i-r-1],e[i-r-1]=f,f=t[r],t[r]=-t[i-r-1],t[i-r-1]=-f}},s.prototype.normalize13b=function(e,t){for(var i=0,r=0;r<t/2;r++){var f=8192*Math.round(e[2*r+1]/t)+Math.round(e[2*r]/t)+i;e[r]=67108863&f,i=f<67108864?0:f/67108864|0}return e},s.prototype.convert13b=function(e,t,i,r){for(var f=0,d=0;d<t;d++)f+=0|e[d],i[2*d]=8191&f,f>>>=13,i[2*d+1]=8191&f,f>>>=13;for(d=2*t;d<r;++d)i[d]=0;p(0===f),p(0==(-8192&f))},s.prototype.stub=function(e){for(var t=new Array(e),i=0;i<e;i++)t[i]=0;return t},s.prototype.mulp=function(e,t,i){var r=2*this.guessLen13b(e.length,t.length),f=this.makeRBT(r),d=this.stub(r),n=new Array(r),a=new Array(r),s=new Array(r),c=new Array(r),h=new Array(r),o=new Array(r),u=i.words;u.length=r,this.convert13b(e.words,e.length,n,r),this.convert13b(t.words,t.length,c,r),this.transform(n,d,a,s,r,f),this.transform(c,d,h,o,r,f);for(var b=0;b<r;b++){var l=a[b]*h[b]-s[b]*o[b];s[b]=a[b]*o[b]+s[b]*h[b],a[b]=l}return this.conjugate(a,s,r),this.transform(a,s,u,d,r,f),this.conjugate(u,d,r),this.normalize13b(u,r),i.negative=e.negative^t.negative,i.length=e.length+t.length,i.strip()},m.prototype.mul=function(e){var t=new m(null);return t.words=new Array(this.length+e.length),this.mulTo(e,t)},m.prototype.mulf=function(e){var t=new m(null);return t.words=new Array(this.length+e.length),a(this,e,t)},m.prototype.imul=function(e){return this.clone().mulTo(e,this)},m.prototype.imuln=function(e){p("number"==typeof e),p(e<67108864);for(var t=0,i=0;i<this.length;i++){var r=(0|this.words[i])*e,f=(67108863&r)+(67108863&t);t>>=26,t+=r/67108864|0,t+=f>>>26,this.words[i]=67108863&f}return 0!==t&&(this.words[i]=t,this.length++),this},m.prototype.muln=function(e){return this.clone().imuln(e)},m.prototype.sqr=function(){return this.mul(this)},m.prototype.isqr=function(){return this.imul(this.clone())},m.prototype.pow=function(e){var t=function(e){for(var t=new Array(e.bitLength()),i=0;i<t.length;i++){var r=i/26|0,f=i%26;t[i]=(e.words[r]&1<<f)>>>f}return t}(e);if(0===t.length)return new m(1);for(var i=this,r=0;r<t.length&&0===t[r];r++,i=i.sqr());if(++r<t.length)for(var f=i.sqr();r<t.length;r++,f=f.sqr())0!==t[r]&&(i=i.mul(f));return i},m.prototype.iushln=function(e){p("number"==typeof e&&0<=e);var t=e%26,i=(e-t)/26,r=67108863>>>26-t<<26-t;if(0!=t){for(var f=0,d=0;d<this.length;d++){var n=this.words[d]&r,a=(0|this.words[d])-n<<t;this.words[d]=a|f,f=n>>>26-t}f&&(this.words[d]=f,this.length++)}if(0!=i){for(d=this.length-1;0<=d;d--)this.words[d+i]=this.words[d];for(d=0;d<i;d++)this.words[d]=0;this.length+=i}return this.strip()},m.prototype.ishln=function(e){return p(0===this.negative),this.iushln(e)},m.prototype.iushrn=function(e,t,i){var r;p("number"==typeof e&&0<=e),r=t?(t-t%26)/26:0;var f=e%26,d=Math.min((e-f)/26,this.length),n=67108863^67108863>>>f<<f,a=i;if(r-=d,r=Math.max(0,r),a){for(var s=0;s<d;s++)a.words[s]=this.words[s];a.length=d}if(0!==d)if(this.length>d)for(this.length-=d,s=0;s<this.length;s++)this.words[s]=this.words[s+d];else this.words[0]=0,this.length=1;for(var c=0,s=this.length-1;0<=s&&(0!==c||r<=s);s--){var h=0|this.words[s];this.words[s]=c<<26-f|h>>>f,c=h&n}return a&&0!==c&&(a.words[a.length++]=c),0===this.length&&(this.words[0]=0,this.length=1),this.strip()},m.prototype.ishrn=function(e,t,i){return p(0===this.negative),this.iushrn(e,t,i)},m.prototype.shln=function(e){return this.clone().ishln(e)},m.prototype.ushln=function(e){return this.clone().iushln(e)},m.prototype.shrn=function(e){return this.clone().ishrn(e)},m.prototype.ushrn=function(e){return this.clone().iushrn(e)},m.prototype.testn=function(e){p("number"==typeof e&&0<=e);var t=e%26,e=(e-t)/26,t=1<<t;return!(this.length<=e)&&!!(this.words[e]&t)},m.prototype.imaskn=function(e){p("number"==typeof e&&0<=e);var t=e%26,e=(e-t)/26;return p(0===this.negative,"imaskn works only with positive numbers"),this.length<=e?this:(0!=t&&e++,this.length=Math.min(e,this.length),0!=t&&(t=67108863^67108863>>>t<<t,this.words[this.length-1]&=t),this.strip())},m.prototype.maskn=function(e){return this.clone().imaskn(e)},m.prototype.iaddn=function(e){return p("number"==typeof e),p(e<67108864),e<0?this.isubn(-e):0!==this.negative?(1===this.length&&(0|this.words[0])<e?(this.words[0]=e-(0|this.words[0]),this.negative=0):(this.negative=0,this.isubn(e),this.negative=1),this):this._iaddn(e)},m.prototype._iaddn=function(e){this.words[0]+=e;for(var t=0;t<this.length&&67108864<=this.words[t];t++)this.words[t]-=67108864,t===this.length-1?this.words[t+1]=1:this.words[t+1]++;return this.length=Math.max(this.length,t+1),this},m.prototype.isubn=function(e){if(p("number"==typeof e),p(e<67108864),e<0)return this.iaddn(-e);if(0!==this.negative)return this.negative=0,this.iaddn(e),this.negative=1,this;if(this.words[0]-=e,1===this.length&&this.words[0]<0)this.words[0]=-this.words[0],this.negative=1;else for(var t=0;t<this.length&&this.words[t]<0;t++)this.words[t]+=67108864,--this.words[t+1];return this.strip()},m.prototype.addn=function(e){return this.clone().iaddn(e)},m.prototype.subn=function(e){return this.clone().isubn(e)},m.prototype.iabs=function(){return this.negative=0,this},m.prototype.abs=function(){return this.clone().iabs()},m.prototype._ishlnsubmul=function(e,t,i){var r,f=e.length+i;this._expand(f);for(var d=0,n=0;n<e.length;n++){r=(0|this.words[n+i])+d;var a=(0|e.words[n])*t,d=((r-=67108863&a)>>26)-(a/67108864|0);this.words[n+i]=67108863&r}for(;n<this.length-i;n++)d=(r=(0|this.words[n+i])+d)>>26,this.words[n+i]=67108863&r;if(0===d)return this.strip();for(p(-1===d),n=d=0;n<this.length;n++)d=(r=-(0|this.words[n])+d)>>26,this.words[n]=67108863&r;return this.negative=1,this.strip()},m.prototype._wordDiv=function(e,t){var i=this.length-e.length,r=this.clone(),f=e,d=0|f.words[f.length-1];0!=(i=26-this._countBits(d))&&(f=f.ushln(i),r.iushln(i),d=0|f.words[f.length-1]);var n,a=r.length-f.length;if("mod"!==t){(n=new m(null)).length=1+a,n.words=new Array(n.length);for(var s=0;s<n.length;s++)n.words[s]=0}e=r.clone()._ishlnsubmul(f,1,a);0===e.negative&&(r=e,n&&(n.words[a]=1));for(var c=a-1;0<=c;c--){var h=67108864*(0|r.words[f.length+c])+(0|r.words[f.length+c-1]),h=Math.min(h/d|0,67108863);for(r._ishlnsubmul(f,h,c);0!==r.negative;)h--,r.negative=0,r._ishlnsubmul(f,1,c),r.isZero()||(r.negative^=1);n&&(n.words[c]=h)}return n&&n.strip(),r.strip(),"div"!==t&&0!=i&&r.iushrn(i),{div:n||null,mod:r}},m.prototype.divmod=function(e,t,i){return p(!e.isZero()),this.isZero()?{div:new m(0),mod:new m(0)}:0!==this.negative&&0===e.negative?(d=this.neg().divmod(e,t),"mod"!==t&&(r=d.div.neg()),"div"!==t&&(f=d.mod.neg(),i&&0!==f.negative&&f.iadd(e)),{div:r,mod:f}):0===this.negative&&0!==e.negative?(d=this.divmod(e.neg(),t),{div:r="mod"!==t?d.div.neg():r,mod:d.mod}):0!=(this.negative&e.negative)?(d=this.neg().divmod(e.neg(),t),"div"!==t&&(f=d.mod.neg(),i&&0!==f.negative&&f.isub(e)),{div:d.div,mod:f}):e.length>this.length||this.cmp(e)<0?{div:new m(0),mod:this}:1===e.length?"div"===t?{div:this.divn(e.words[0]),mod:null}:"mod"===t?{div:null,mod:new m(this.modn(e.words[0]))}:{div:this.divn(e.words[0]),mod:new m(this.modn(e.words[0]))}:this._wordDiv(e,t);var r,f,d},m.prototype.div=function(e){return this.divmod(e,"div",!1).div},m.prototype.mod=function(e){return this.divmod(e,"mod",!1).mod},m.prototype.umod=function(e){return this.divmod(e,"mod",!0).mod},m.prototype.divRound=function(e){var t=this.divmod(e);if(t.mod.isZero())return t.div;var i=0!==t.div.negative?t.mod.isub(e):t.mod,r=e.ushrn(1),e=e.andln(1),r=i.cmp(r);return r<0||1===e&&0===r?t.div:0!==t.div.negative?t.div.isubn(1):t.div.iaddn(1)},m.prototype.modn=function(e){p(e<=67108863);for(var t=(1<<26)%e,i=0,r=this.length-1;0<=r;r--)i=(t*i+(0|this.words[r]))%e;return i},m.prototype.idivn=function(e){p(e<=67108863);for(var t=0,i=this.length-1;0<=i;i--){var r=(0|this.words[i])+67108864*t;this.words[i]=r/e|0,t=r%e}return this.strip()},m.prototype.divn=function(e){return this.clone().idivn(e)},m.prototype.egcd=function(e){p(0===e.negative),p(!e.isZero());for(var t=this,i=e.clone(),t=0!==t.negative?t.umod(e):t.clone(),r=new m(1),f=new m(0),d=new m(0),n=new m(1),a=0;t.isEven()&&i.isEven();)t.iushrn(1),i.iushrn(1),++a;for(var s=i.clone(),c=t.clone();!t.isZero();){for(var h=0,o=1;0==(t.words[0]&o)&&h<26;++h,o<<=1);if(0<h)for(t.iushrn(h);0<h--;)(r.isOdd()||f.isOdd())&&(r.iadd(s),f.isub(c)),r.iushrn(1),f.iushrn(1);for(var u=0,b=1;0==(i.words[0]&b)&&u<26;++u,b<<=1);if(0<u)for(i.iushrn(u);0<u--;)(d.isOdd()||n.isOdd())&&(d.iadd(s),n.isub(c)),d.iushrn(1),n.iushrn(1);0<=t.cmp(i)?(t.isub(i),r.isub(d),f.isub(n)):(i.isub(t),d.isub(r),n.isub(f))}return{a:d,b:n,gcd:i.iushln(a)}},m.prototype._invmp=function(e){p(0===e.negative),p(!e.isZero());for(var t,i=this,r=e.clone(),i=0!==i.negative?i.umod(e):i.clone(),f=new m(1),d=new m(0),n=r.clone();0<i.cmpn(1)&&0<r.cmpn(1);){for(var a=0,s=1;0==(i.words[0]&s)&&a<26;++a,s<<=1);if(0<a)for(i.iushrn(a);0<a--;)f.isOdd()&&f.iadd(n),f.iushrn(1);for(var c=0,h=1;0==(r.words[0]&h)&&c<26;++c,h<<=1);if(0<c)for(r.iushrn(c);0<c--;)d.isOdd()&&d.iadd(n),d.iushrn(1);0<=i.cmp(r)?(i.isub(r),f.isub(d)):(r.isub(i),d.isub(f))}return(t=0===i.cmpn(1)?f:d).cmpn(0)<0&&t.iadd(e),t},m.prototype.gcd=function(e){if(this.isZero())return e.abs();if(e.isZero())return this.abs();var t=this.clone(),i=e.clone();t.negative=0;for(var r=i.negative=0;t.isEven()&&i.isEven();r++)t.iushrn(1),i.iushrn(1);for(;;){for(;t.isEven();)t.iushrn(1);for(;i.isEven();)i.iushrn(1);var f=t.cmp(i);if(f<0)var d=t,t=i,i=d;else if(0===f||0===i.cmpn(1))break;t.isub(i)}return i.iushln(r)},m.prototype.invm=function(e){return this.egcd(e).a.umod(e)},m.prototype.isEven=function(){return 0==(1&this.words[0])},m.prototype.isOdd=function(){return 1==(1&this.words[0])},m.prototype.andln=function(e){return this.words[0]&e},m.prototype.bincn=function(e){p("number"==typeof e);var t=e%26,e=(e-t)/26,t=1<<t;if(this.length<=e)return this._expand(1+e),this.words[e]|=t,this;for(var i=t,r=e;0!==i&&r<this.length;r++){var f=0|this.words[r],i=(f+=i)>>>26;f&=67108863,this.words[r]=f}return 0!==i&&(this.words[r]=i,this.length++),this},m.prototype.isZero=function(){return 1===this.length&&0===this.words[0]},m.prototype.cmpn=function(e){var t=e<0;return 0===this.negative||t?0===this.negative&&t?1:(this.strip(),e=1<this.length?1:(p((e=t?-e:e)<=67108863,"Number is too big"),(t=0|this.words[0])===e?0:t<e?-1:1),0!==this.negative?0|-e:e):-1},m.prototype.cmp=function(e){if(0!==this.negative&&0===e.negative)return-1;if(0===this.negative&&0!==e.negative)return 1;e=this.ucmp(e);return 0!==this.negative?0|-e:e},m.prototype.ucmp=function(e){if(this.length>e.length)return 1;if(this.length<e.length)return-1;for(var t=0,i=this.length-1;0<=i;i--){var r=0|this.words[i],f=0|e.words[i];if(r!=f){r<f?t=-1:f<r&&(t=1);break}}return t},m.prototype.gtn=function(e){return 1===this.cmpn(e)},m.prototype.gt=function(e){return 1===this.cmp(e)},m.prototype.gten=function(e){return 0<=this.cmpn(e)},m.prototype.gte=function(e){return 0<=this.cmp(e)},m.prototype.ltn=function(e){return-1===this.cmpn(e)},m.prototype.lt=function(e){return-1===this.cmp(e)},m.prototype.lten=function(e){return this.cmpn(e)<=0},m.prototype.lte=function(e){return this.cmp(e)<=0},m.prototype.eqn=function(e){return 0===this.cmpn(e)},m.prototype.eq=function(e){return 0===this.cmp(e)},m.red=function(e){return new w(e)},m.prototype.toRed=function(e){return p(!this.red,"Already a number in reduction context"),p(0===this.negative,"red works only with positives"),e.convertTo(this)._forceRed(e)},m.prototype.fromRed=function(){return p(this.red,"fromRed works only with numbers in reduction context"),this.red.convertFrom(this)},m.prototype._forceRed=function(e){return this.red=e,this},m.prototype.forceRed=function(e){return p(!this.red,"Already a number in reduction context"),this._forceRed(e)},m.prototype.redAdd=function(e){return p(this.red,"redAdd works only with red numbers"),this.red.add(this,e)},m.prototype.redIAdd=function(e){return p(this.red,"redIAdd works only with red numbers"),this.red.iadd(this,e)},m.prototype.redSub=function(e){return p(this.red,"redSub works only with red numbers"),this.red.sub(this,e)},m.prototype.redISub=function(e){return p(this.red,"redISub works only with red numbers"),this.red.isub(this,e)},m.prototype.redShl=function(e){return p(this.red,"redShl works only with red numbers"),this.red.shl(this,e)},m.prototype.redMul=function(e){return p(this.red,"redMul works only with red numbers"),this.red._verify2(this,e),this.red.mul(this,e)},m.prototype.redIMul=function(e){return p(this.red,"redMul works only with red numbers"),this.red._verify2(this,e),this.red.imul(this,e)},m.prototype.redSqr=function(){return p(this.red,"redSqr works only with red numbers"),this.red._verify1(this),this.red.sqr(this)},m.prototype.redISqr=function(){return p(this.red,"redISqr works only with red numbers"),this.red._verify1(this),this.red.isqr(this)},m.prototype.redSqrt=function(){return p(this.red,"redSqrt works only with red numbers"),this.red._verify1(this),this.red.sqrt(this)},m.prototype.redInvm=function(){return p(this.red,"redInvm works only with red numbers"),this.red._verify1(this),this.red.invm(this)},m.prototype.redNeg=function(){return p(this.red,"redNeg works only with red numbers"),this.red._verify1(this),this.red.neg(this)},m.prototype.redPow=function(e){return p(this.red&&!e.red,"redPow(normalNum)"),this.red._verify1(this),this.red.pow(this,e)};var c={k256:null,p224:null,p192:null,p25519:null};function h(e,t){this.name=e,this.p=new m(t,16),this.n=this.p.bitLength(),this.k=new m(1).iushln(this.n).isub(this.p),this.tmp=this._tmp()}function v(){h.call(this,"k256","ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe fffffc2f")}function g(){h.call(this,"p224","ffffffff ffffffff ffffffff ffffffff 00000000 00000000 00000001")}function y(){h.call(this,"p192","ffffffff ffffffff ffffffff fffffffe ffffffff ffffffff")}function M(){h.call(this,"25519","7fffffffffffffff ffffffffffffffff ffffffffffffffff ffffffffffffffed")}function w(e){var t;"string"==typeof e?(t=m._prime(e),this.m=t.p,this.prime=t):(p(e.gtn(1),"modulus must be greater than 1"),this.m=e,this.prime=null)}function S(e){w.call(this,e),this.shift=this.m.bitLength(),this.shift%26!=0&&(this.shift+=26-this.shift%26),this.r=new m(1).iushln(this.shift),this.r2=this.imod(this.r.sqr()),this.rinv=this.r._invmp(this.m),this.minv=this.rinv.mul(this.r).isubn(1).div(this.m),this.minv=this.minv.umod(this.r),this.minv=this.r.sub(this.minv)}h.prototype._tmp=function(){var e=new m(null);return e.words=new Array(Math.ceil(this.n/13)),e},h.prototype.ireduce=function(e){for(var t,i=e;this.split(i,this.tmp),t=(i=(i=this.imulK(i)).iadd(this.tmp)).bitLength(),t>this.n;);e=t<this.n?-1:i.ucmp(this.p);return 0===e?(i.words[0]=0,i.length=1):0<e?i.isub(this.p):void 0!==i.strip?i.strip():i._strip(),i},h.prototype.split=function(e,t){e.iushrn(this.n,0,t)},h.prototype.imulK=function(e){return e.imul(this.k)},i(v,h),v.prototype.split=function(e,t){for(var i=Math.min(e.length,9),r=0;r<i;r++)t.words[r]=e.words[r];if(t.length=i,e.length<=9)return e.words[0]=0,void(e.length=1);var f=e.words[9];for(t.words[t.length++]=4194303&f,r=10;r<e.length;r++){var d=0|e.words[r];e.words[r-10]=(4194303&d)<<4|f>>>22,f=d}f>>>=22,0===(e.words[r-10]=f)&&10<e.length?e.length-=10:e.length-=9},v.prototype.imulK=function(e){e.words[e.length]=0,e.words[e.length+1]=0,e.length+=2;for(var t=0,i=0;i<e.length;i++){var r=0|e.words[i];t+=977*r,e.words[i]=67108863&t,t=64*r+(t/67108864|0)}return 0===e.words[e.length-1]&&(e.length--,0===e.words[e.length-1]&&e.length--),e},i(g,h),i(y,h),i(M,h),M.prototype.imulK=function(e){for(var t=0,i=0;i<e.length;i++){var r=19*(0|e.words[i])+t,f=67108863&r;r>>>=26,e.words[i]=f,t=r}return 0!==t&&(e.words[e.length++]=t),e},m._prime=function(e){if(c[e])return c[e];var t;if("k256"===e)t=new v;else if("p224"===e)t=new g;else if("p192"===e)t=new y;else{if("p25519"!==e)throw new Error("Unknown prime "+e);t=new M}return c[e]=t},w.prototype._verify1=function(e){p(0===e.negative,"red works only with positives"),p(e.red,"red works only with red numbers")},w.prototype._verify2=function(e,t){p(0==(e.negative|t.negative),"red works only with positives"),p(e.red&&e.red===t.red,"red works only with red numbers")},w.prototype.imod=function(e){return(this.prime?this.prime.ireduce(e):e.umod(this.m))._forceRed(this)},w.prototype.neg=function(e){return e.isZero()?e.clone():this.m.sub(e)._forceRed(this)},w.prototype.add=function(e,t){this._verify2(e,t);t=e.add(t);return 0<=t.cmp(this.m)&&t.isub(this.m),t._forceRed(this)},w.prototype.iadd=function(e,t){this._verify2(e,t);t=e.iadd(t);return 0<=t.cmp(this.m)&&t.isub(this.m),t},w.prototype.sub=function(e,t){this._verify2(e,t);t=e.sub(t);return t.cmpn(0)<0&&t.iadd(this.m),t._forceRed(this)},w.prototype.isub=function(e,t){this._verify2(e,t);t=e.isub(t);return t.cmpn(0)<0&&t.iadd(this.m),t},w.prototype.shl=function(e,t){return this._verify1(e),this.imod(e.ushln(t))},w.prototype.imul=function(e,t){return this._verify2(e,t),this.imod(e.imul(t))},w.prototype.mul=function(e,t){return this._verify2(e,t),this.imod(e.mul(t))},w.prototype.isqr=function(e){return this.imul(e,e.clone())},w.prototype.sqr=function(e){return this.mul(e,e)},w.prototype.sqrt=function(e){if(e.isZero())return e.clone();var t=this.m.andln(3);if(p(t%2==1),3===t){t=this.m.add(new m(1)).iushrn(2);return this.pow(e,t)}for(var i=this.m.subn(1),r=0;!i.isZero()&&0===i.andln(1);)r++,i.iushrn(1);p(!i.isZero());for(var f=new m(1).toRed(this),d=f.redNeg(),n=this.m.subn(1).iushrn(1),a=new m(2*(a=this.m.bitLength())*a).toRed(this);0!==this.pow(a,n).cmp(d);)a.redIAdd(d);for(var s=this.pow(a,i),c=this.pow(e,i.addn(1).iushrn(1)),h=this.pow(e,i),o=r;0!==h.cmp(f);){for(var u=h,b=0;0!==u.cmp(f);b++)u=u.redSqr();p(b<o);var l=this.pow(s,new m(1).iushln(o-b-1)),c=c.redMul(l),s=l.redSqr(),h=h.redMul(s),o=b}return c},w.prototype.invm=function(e){e=e._invmp(this.m);return 0!==e.negative?(e.negative=0,this.imod(e).redNeg()):this.imod(e)},w.prototype.pow=function(e,t){if(t.isZero())return new m(1).toRed(this);if(0===t.cmpn(1))return e.clone();var i=new Array(16);i[0]=new m(1).toRed(this),i[1]=e;for(var r=2;r<i.length;r++)i[r]=this.mul(i[r-1],e);var f=i[0],d=0,n=0,a=t.bitLength()%26;for(0===a&&(a=26),r=t.length-1;0<=r;r--){for(var s=t.words[r],c=a-1;0<=c;c--){var h=s>>c&1;f!==i[0]&&(f=this.sqr(f)),0!=h||0!==d?(d<<=1,d|=h,(4===++n||0===r&&0===c)&&(f=this.mul(f,i[d]),d=n=0)):n=0}a=26}return f},w.prototype.convertTo=function(e){var t=e.umod(this.m);return t===e?t.clone():t},w.prototype.convertFrom=function(e){e=e.clone();return e.red=null,e},m.mont=function(e){return new S(e)},i(S,w),S.prototype.convertTo=function(e){return this.imod(e.ushln(this.shift))},S.prototype.convertFrom=function(e){e=this.imod(e.mul(this.rinv));return e.red=null,e},S.prototype.imul=function(e,t){if(e.isZero()||t.isZero())return e.words[0]=0,e.length=1,e;e=e.imul(t),t=e.maskn(this.shift).mul(this.minv).imaskn(this.shift).mul(this.m),e=e.isub(t).iushrn(this.shift),t=e;return 0<=e.cmp(this.m)?t=e.isub(this.m):e.cmpn(0)<0&&(t=e.iadd(this.m)),t._forceRed(this)},S.prototype.mul=function(e,t){if(e.isZero()||t.isZero())return new m(0)._forceRed(this);e=e.mul(t),t=e.maskn(this.shift).mul(this.minv).imaskn(this.shift).mul(this.m),e=e.isub(t).iushrn(this.shift),t=e;return 0<=e.cmp(this.m)?t=e.isub(this.m):e.cmpn(0)<0&&(t=e.iadd(this.m)),t._forceRed(this)},S.prototype.invm=function(e){return this.imod(e._invmp(this.m).mul(this.r2))._forceRed(this)}}(void 0===e||e,this)},{buffer:18}],17:[function(e,t,i){var r;function f(e){this.rand=e}if(t.exports=function(e){return(r=r||new f(null)).generate(e)},(t.exports.Rand=f).prototype.generate=function(e){return this._rand(e)},f.prototype._rand=function(e){if(this.rand.getBytes)return this.rand.getBytes(e);for(var t=new Uint8Array(e),i=0;i<t.length;i++)t[i]=this.rand.getByte();return t},"object"==typeof self)self.crypto&&self.crypto.getRandomValues?f.prototype._rand=function(e){e=new Uint8Array(e);return self.crypto.getRandomValues(e),e}:self.msCrypto&&self.msCrypto.getRandomValues?f.prototype._rand=function(e){e=new Uint8Array(e);return self.msCrypto.getRandomValues(e),e}:"object"==typeof window&&(f.prototype._rand=function(){throw new Error("Not implemented yet")});else try{var d=e("crypto");if("function"!=typeof d.randomBytes)throw new Error("Not supported");f.prototype._rand=function(e){return d.randomBytes(e)}}catch(e){}},{crypto:18}],18:[function(e,t,i){},{}],19:[function(e,t,i){i.utils=e("./hash/utils"),i.common=e("./hash/common"),i.sha=e("./hash/sha"),i.ripemd=e("./hash/ripemd"),i.hmac=e("./hash/hmac"),i.sha1=i.sha.sha1,i.sha256=i.sha.sha256,i.sha224=i.sha.sha224,i.sha384=i.sha.sha384,i.sha512=i.sha.sha512,i.ripemd160=i.ripemd.ripemd160},{"./hash/common":20,"./hash/hmac":21,"./hash/ripemd":22,"./hash/sha":23,"./hash/utils":30}],20:[function(e,t,i){"use strict";var r=e("./utils"),f=e("minimalistic-assert");function d(){this.pending=null,this.pendingTotal=0,this.blockSize=this.constructor.blockSize,this.outSize=this.constructor.outSize,this.hmacStrength=this.constructor.hmacStrength,this.padLength=this.constructor.padLength/8,this.endian="big",this._delta8=this.blockSize/8,this._delta32=this.blockSize/32}(i.BlockHash=d).prototype.update=function(e,t){if(e=r.toArray(e,t),this.pending?this.pending=this.pending.concat(e):this.pending=e,this.pendingTotal+=e.length,this.pending.length>=this._delta8){t=(e=this.pending).length%this._delta8;this.pending=e.slice(e.length-t,e.length),0===this.pending.length&&(this.pending=null),e=r.join32(e,0,e.length-t,this.endian);for(var i=0;i<e.length;i+=this._delta32)this._update(e,i,i+this._delta32)}return this},d.prototype.digest=function(e){return this.update(this._pad()),f(null===this.pending),this._digest(e)},d.prototype._pad=function(){var e=this.pendingTotal,t=this._delta8,i=t-(e+this.padLength)%t,r=new Array(i+this.padLength);r[0]=128;for(var f=1;f<i;f++)r[f]=0;if(e<<=3,"big"===this.endian){for(var d=8;d<this.padLength;d++)r[f++]=0;r[f++]=0,r[f++]=0,r[f++]=0,r[f++]=0,r[f++]=e>>>24&255,r[f++]=e>>>16&255,r[f++]=e>>>8&255,r[f++]=255&e}else for(r[f++]=255&e,r[f++]=e>>>8&255,r[f++]=e>>>16&255,r[f++]=e>>>24&255,r[f++]=0,r[f++]=0,r[f++]=0,r[f++]=0,d=8;d<this.padLength;d++)r[f++]=0;return r}},{"./utils":30,"minimalistic-assert":33}],21:[function(e,t,i){"use strict";var r=e("./utils"),f=e("minimalistic-assert");function d(e,t,i){if(!(this instanceof d))return new d(e,t,i);this.Hash=e,this.blockSize=e.blockSize/8,this.outSize=e.outSize/8,this.inner=null,this.outer=null,this._init(r.toArray(t,i))}(t.exports=d).prototype._init=function(e){e.length>this.blockSize&&(e=(new this.Hash).update(e).digest()),f(e.length<=this.blockSize);for(var t=e.length;t<this.blockSize;t++)e.push(0);for(t=0;t<e.length;t++)e[t]^=54;for(this.inner=(new this.Hash).update(e),t=0;t<e.length;t++)e[t]^=106;this.outer=(new this.Hash).update(e)},d.prototype.update=function(e,t){return this.inner.update(e,t),this},d.prototype.digest=function(e){return this.outer.update(this.inner.digest()),this.outer.digest(e)}},{"./utils":30,"minimalistic-assert":33}],22:[function(e,t,i){"use strict";var r=e("./utils"),e=e("./common"),p=r.rotl32,m=r.sum32,v=r.sum32_3,g=r.sum32_4,f=e.BlockHash;function d(){if(!(this instanceof d))return new d;f.call(this),this.h=[1732584193,4023233417,2562383102,271733878,3285377520],this.endian="little"}function y(e,t,i,r){return e<=15?t^i^r:e<=31?t&i|~t&r:e<=47?(t|~i)^r:e<=63?t&r|i&~r:t^(i|~r)}r.inherits(d,f),(i.ripemd160=d).blockSize=512,d.outSize=160,d.hmacStrength=192,d.padLength=64,d.prototype._update=function(e,t){for(var i,r=h=this.h[0],f=l=this.h[1],d=b=this.h[2],n=u=this.h[3],a=o=this.h[4],s=0;s<80;s++)var c=m(p(g(h,y(s,l,b,u),e[M[s]+t],(i=s)<=15?0:i<=31?1518500249:i<=47?1859775393:i<=63?2400959708:2840853838),S[s]),o),h=o,o=u,u=p(b,10),b=l,l=c,c=m(p(g(r,y(79-s,f,d,n),e[w[s]+t],(i=s)<=15?1352829926:i<=31?1548603684:i<=47?1836072691:i<=63?2053994217:0),_[s]),a),r=a,a=n,n=p(d,10),d=f,f=c;c=v(this.h[1],b,n),this.h[1]=v(this.h[2],u,a),this.h[2]=v(this.h[3],o,r),this.h[3]=v(this.h[4],h,f),this.h[4]=v(this.h[0],l,d),this.h[0]=c},d.prototype._digest=function(e){return"hex"===e?r.toHex32(this.h,"little"):r.split32(this.h,"little")};var M=[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,7,4,13,1,10,6,15,3,12,0,9,5,2,14,11,8,3,10,14,4,9,15,8,1,2,7,0,6,13,11,5,12,1,9,11,10,0,8,12,4,13,3,7,15,14,5,6,2,4,0,5,9,7,12,2,10,14,1,3,8,11,6,15,13],w=[5,14,7,0,9,2,11,4,13,6,15,8,1,10,3,12,6,11,3,7,0,13,5,10,14,15,8,12,4,9,1,2,15,5,1,3,7,14,6,9,11,8,12,2,10,0,4,13,8,6,4,1,3,11,15,0,5,12,2,13,9,7,10,14,12,15,10,4,1,5,8,7,6,2,13,14,0,3,9,11],S=[11,14,15,12,5,8,7,9,11,13,14,15,6,7,9,8,7,6,8,13,11,9,7,15,7,12,15,9,11,7,13,12,11,13,6,7,14,9,13,15,14,8,13,6,5,12,7,5,11,12,14,15,14,15,9,8,9,14,5,6,8,6,5,12,9,15,5,11,6,8,13,12,5,12,13,14,11,8,5,6],_=[8,9,9,11,13,15,15,5,7,7,8,11,14,14,12,6,9,13,15,7,12,8,9,11,7,7,12,7,6,15,13,11,9,7,15,11,8,6,6,14,12,13,5,14,13,13,7,5,15,5,8,11,14,14,6,14,6,9,12,9,12,5,15,8,8,5,12,9,12,5,14,6,8,13,6,5,15,13,11,11]},{"./common":20,"./utils":30}],23:[function(e,t,i){"use strict";i.sha1=e("./sha/1"),i.sha224=e("./sha/224"),i.sha256=e("./sha/256"),i.sha384=e("./sha/384"),i.sha512=e("./sha/512")},{"./sha/1":24,"./sha/224":25,"./sha/256":26,"./sha/384":27,"./sha/512":28}],24:[function(e,t,i){"use strict";var r=e("../utils"),f=e("../common"),e=e("./common"),h=r.rotl32,o=r.sum32,u=r.sum32_5,b=e.ft_1,d=f.BlockHash,l=[1518500249,1859775393,2400959708,3395469782];function n(){if(!(this instanceof n))return new n;d.call(this),this.h=[1732584193,4023233417,2562383102,271733878,3285377520],this.W=new Array(80)}r.inherits(n,d),(t.exports=n).blockSize=512,n.outSize=160,n.hmacStrength=80,n.padLength=64,n.prototype._update=function(e,t){for(var i=this.W,r=0;r<16;r++)i[r]=e[t+r];for(;r<i.length;r++)i[r]=h(i[r-3]^i[r-8]^i[r-14]^i[r-16],1);for(var f=this.h[0],d=this.h[1],n=this.h[2],a=this.h[3],s=this.h[4],r=0;r<i.length;r++)var c=~~(r/20),c=u(h(f,5),b(c,d,n,a),s,i[r],l[c]),s=a,a=n,n=h(d,30),d=f,f=c;this.h[0]=o(this.h[0],f),this.h[1]=o(this.h[1],d),this.h[2]=o(this.h[2],n),this.h[3]=o(this.h[3],a),this.h[4]=o(this.h[4],s)},n.prototype._digest=function(e){return"hex"===e?r.toHex32(this.h,"big"):r.split32(this.h,"big")}},{"../common":20,"../utils":30,"./common":29}],25:[function(e,t,i){"use strict";var r=e("../utils"),f=e("./256");function d(){if(!(this instanceof d))return new d;f.call(this),this.h=[3238371032,914150663,812702999,4144912697,4290775857,1750603025,1694076839,3204075428]}r.inherits(d,f),(t.exports=d).blockSize=512,d.outSize=224,d.hmacStrength=192,d.padLength=64,d.prototype._digest=function(e){return"hex"===e?r.toHex32(this.h.slice(0,7),"big"):r.split32(this.h.slice(0,7),"big")}},{"../utils":30,"./256":26}],26:[function(e,t,i){"use strict";var r=e("../utils"),f=e("../common"),d=e("./common"),l=e("minimalistic-assert"),p=r.sum32,m=r.sum32_4,v=r.sum32_5,g=d.ch32,y=d.maj32,M=d.s0_256,w=d.s1_256,S=d.g0_256,_=d.g1_256,n=f.BlockHash,a=[1116352408,1899447441,3049323471,3921009573,961987163,1508970993,2453635748,2870763221,3624381080,310598401,607225278,1426881987,1925078388,2162078206,2614888103,3248222580,3835390401,4022224774,264347078,604807628,770255983,1249150122,1555081692,1996064986,2554220882,2821834349,2952996808,3210313671,3336571891,3584528711,113926993,338241895,666307205,773529912,1294757372,1396182291,1695183700,1986661051,2177026350,2456956037,2730485921,2820302411,3259730800,3345764771,3516065817,3600352804,4094571909,275423344,430227734,506948616,659060556,883997877,958139571,1322822218,1537002063,1747873779,1955562222,2024104815,2227730452,2361852424,2428436474,2756734187,3204031479,3329325298];function s(){if(!(this instanceof s))return new s;n.call(this),this.h=[1779033703,3144134277,1013904242,2773480762,1359893119,2600822924,528734635,1541459225],this.k=a,this.W=new Array(64)}r.inherits(s,n),(t.exports=s).blockSize=512,s.outSize=256,s.hmacStrength=192,s.padLength=64,s.prototype._update=function(e,t){for(var i=this.W,r=0;r<16;r++)i[r]=e[t+r];for(;r<i.length;r++)i[r]=m(_(i[r-2]),i[r-7],S(i[r-15]),i[r-16]);var f=this.h[0],d=this.h[1],n=this.h[2],a=this.h[3],s=this.h[4],c=this.h[5],h=this.h[6],o=this.h[7];for(l(this.k.length===i.length),r=0;r<i.length;r++)var u=v(o,w(s),g(s,c,h),this.k[r],i[r]),b=p(M(f),y(f,d,n)),o=h,h=c,c=s,s=p(a,u),a=n,n=d,d=f,f=p(u,b);this.h[0]=p(this.h[0],f),this.h[1]=p(this.h[1],d),this.h[2]=p(this.h[2],n),this.h[3]=p(this.h[3],a),this.h[4]=p(this.h[4],s),this.h[5]=p(this.h[5],c),this.h[6]=p(this.h[6],h),this.h[7]=p(this.h[7],o)},s.prototype._digest=function(e){return"hex"===e?r.toHex32(this.h,"big"):r.split32(this.h,"big")}},{"../common":20,"../utils":30,"./common":29,"minimalistic-assert":33}],27:[function(e,t,i){"use strict";var r=e("../utils"),f=e("./512");function d(){if(!(this instanceof d))return new d;f.call(this),this.h=[3418070365,3238371032,1654270250,914150663,2438529370,812702999,355462360,4144912697,1731405415,4290775857,2394180231,1750603025,3675008525,1694076839,1203062813,3204075428]}r.inherits(d,f),(t.exports=d).blockSize=1024,d.outSize=384,d.hmacStrength=192,d.padLength=128,d.prototype._digest=function(e){return"hex"===e?r.toHex32(this.h.slice(0,12),"big"):r.split32(this.h.slice(0,12),"big")}},{"../utils":30,"./512":28}],28:[function(e,t,i){"use strict";var r=e("../utils"),f=e("../common"),P=e("minimalistic-assert"),j=r.rotr64_hi,N=r.rotr64_lo,u=r.shr64_hi,b=r.shr64_lo,E=r.sum64,B=r.sum64_hi,L=r.sum64_lo,l=r.sum64_4_hi,p=r.sum64_4_lo,O=r.sum64_5_hi,F=r.sum64_5_lo,d=f.BlockHash,n=[1116352408,3609767458,1899447441,602891725,3049323471,3964484399,3921009573,2173295548,961987163,4081628472,1508970993,3053834265,2453635748,2937671579,2870763221,3664609560,3624381080,2734883394,310598401,1164996542,607225278,1323610764,1426881987,3590304994,1925078388,4068182383,2162078206,991336113,2614888103,633803317,3248222580,3479774868,3835390401,2666613458,4022224774,944711139,264347078,2341262773,604807628,2007800933,770255983,1495990901,1249150122,1856431235,1555081692,3175218132,1996064986,2198950837,2554220882,3999719339,2821834349,766784016,2952996808,2566594879,3210313671,3203337956,3336571891,1034457026,3584528711,2466948901,113926993,3758326383,338241895,168717936,666307205,1188179964,773529912,1546045734,1294757372,1522805485,1396182291,2643833823,1695183700,2343527390,1986661051,1014477480,2177026350,1206759142,2456956037,344077627,2730485921,1290863460,2820302411,3158454273,3259730800,3505952657,3345764771,106217008,3516065817,3606008344,3600352804,1432725776,4094571909,1467031594,275423344,851169720,430227734,3100823752,506948616,1363258195,659060556,3750685593,883997877,3785050280,958139571,3318307427,1322822218,3812723403,1537002063,2003034995,1747873779,3602036899,1955562222,1575990012,2024104815,1125592928,2227730452,2716904306,2361852424,442776044,2428436474,593698344,2756734187,3733110249,3204031479,2999351573,3329325298,3815920427,3391569614,3928383900,3515267271,566280711,3940187606,3454069534,4118630271,4000239992,116418474,1914138554,174292421,2731055270,289380356,3203993006,460393269,320620315,685471733,587496836,852142971,1086792851,1017036298,365543100,1126000580,2618297676,1288033470,3409855158,1501505948,4234509866,1607167915,987167468,1816402316,1246189591];function a(){if(!(this instanceof a))return new a;d.call(this),this.h=[1779033703,4089235720,3144134277,2227873595,1013904242,4271175723,2773480762,1595750129,1359893119,2917565137,2600822924,725511199,528734635,4215389547,1541459225,327033209],this.k=n,this.W=new Array(160)}r.inherits(a,d),(t.exports=a).blockSize=1024,a.outSize=512,a.hmacStrength=192,a.padLength=128,a.prototype._prepareBlock=function(e,t){for(var i=this.W,r=0;r<32;r++)i[r]=e[t+r];for(;r<i.length;r+=2){var f=function(e,t){var i=j(e,t,19),r=j(t,e,29),t=u(e,t,6),t=i^r^t;t<0&&(t+=4294967296);return t}(i[r-4],i[r-3]),d=function(e,t){var i=N(e,t,19),r=N(t,e,29),t=b(e,t,6),t=i^r^t;t<0&&(t+=4294967296);return t}(i[r-4],i[r-3]),n=i[r-14],a=i[r-13],s=function(e,t){var i=j(e,t,1),r=j(e,t,8),t=u(e,t,7),t=i^r^t;t<0&&(t+=4294967296);return t}(i[r-30],i[r-29]),c=function(e,t){var i=N(e,t,1),r=N(e,t,8),t=b(e,t,7),t=i^r^t;t<0&&(t+=4294967296);return t}(i[r-30],i[r-29]),h=i[r-32],o=i[r-31];i[r]=l(f,d,n,a,s,c,h,o),i[r+1]=p(f,d,n,a,s,c,h,o)}},a.prototype._update=function(e,t){this._prepareBlock(e,t);var i=this.W,r=this.h[0],f=this.h[1],d=this.h[2],n=this.h[3],a=this.h[4],s=this.h[5],c=this.h[6],h=this.h[7],o=this.h[8],u=this.h[9],b=this.h[10],l=this.h[11],p=this.h[12],m=this.h[13],v=this.h[14],g=this.h[15];P(this.k.length===i.length);for(var y=0;y<i.length;y+=2)var M=v,w=g,S=function(e,t){var i=j(e,t,14),r=j(e,t,18),e=j(t,e,9),e=i^r^e;e<0&&(e+=4294967296);return e}(o,u),_=function(e,t){var i=N(e,t,14),r=N(e,t,18),e=N(t,e,9),e=i^r^e;e<0&&(e+=4294967296);return e}(o,u),A=function(e,t,i){i=e&t^~e&i;i<0&&(i+=4294967296);return i}(o,b,p),x=function(e,t,i){i=e&t^~e&i;i<0&&(i+=4294967296);return i}(u,l,m),I=this.k[y],z=this.k[y+1],q=i[y],R=i[y+1],k=O(M,w,S,_,A,x,I,z,q,R),q=F(M,w,S,_,A,x,I,z,q,R),M=function(e,t){var i=j(e,t,28),r=j(t,e,2),e=j(t,e,7),e=i^r^e;e<0&&(e+=4294967296);return e}(r,f),w=function(e,t){var i=N(e,t,28),r=N(t,e,2),e=N(t,e,7),e=i^r^e;e<0&&(e+=4294967296);return e}(r,f),S=function(e,t,i){i=e&t^e&i^t&i;i<0&&(i+=4294967296);return i}(r,d,a),_=function(e,t,i){i=e&t^e&i^t&i;i<0&&(i+=4294967296);return i}(f,n,s),R=B(M,w,S,_),_=L(M,w,S,_),v=p,g=m,p=b,m=l,b=o,l=u,o=B(c,h,k,q),u=L(h,h,k,q),c=a,h=s,a=d,s=n,d=r,n=f,r=B(k,q,R,_),f=L(k,q,R,_);E(this.h,0,r,f),E(this.h,2,d,n),E(this.h,4,a,s),E(this.h,6,c,h),E(this.h,8,o,u),E(this.h,10,b,l),E(this.h,12,p,m),E(this.h,14,v,g)},a.prototype._digest=function(e){return"hex"===e?r.toHex32(this.h,"big"):r.split32(this.h,"big")}},{"../common":20,"../utils":30,"minimalistic-assert":33}],29:[function(e,t,i){"use strict";var r=e("../utils").rotr32;function f(e,t,i){return e&t^~e&i}function d(e,t,i){return e&t^e&i^t&i}function n(e,t,i){return e^t^i}i.ft_1=function(e,t,i,r){return 0===e?f(t,i,r):1===e||3===e?t^i^r:2===e?d(t,i,r):void 0},i.ch32=f,i.maj32=d,i.p32=n,i.s0_256=function(e){return r(e,2)^r(e,13)^r(e,22)},i.s1_256=function(e){return r(e,6)^r(e,11)^r(e,25)},i.g0_256=function(e){return r(e,7)^r(e,18)^e>>>3},i.g1_256=function(e){return r(e,17)^r(e,19)^e>>>10}},{"../utils":30}],30:[function(e,t,i){"use strict";var s=e("minimalistic-assert"),e=e("inherits");function d(e){return(e>>>24|e>>>8&65280|e<<8&16711680|(255&e)<<24)>>>0}function r(e){return 1===e.length?"0"+e:e}function n(e){return 7===e.length?"0"+e:6===e.length?"00"+e:5===e.length?"000"+e:4===e.length?"0000"+e:3===e.length?"00000"+e:2===e.length?"000000"+e:1===e.length?"0000000"+e:e}i.inherits=e,i.toArray=function(e,t){if(Array.isArray(e))return e.slice();if(!e)return[];var i,r,f=[];if("string"==typeof e)if(t){if("hex"===t)for((e=e.replace(/[^a-z0-9]+/gi,"")).length%2!=0&&(e="0"+e),n=0;n<e.length;n+=2)f.push(parseInt(e[n]+e[n+1],16))}else for(var d=0,n=0;n<e.length;n++){var a=e.charCodeAt(n);a<128?f[d++]=a:a<2048?(f[d++]=a>>6|192,f[d++]=63&a|128):(r=n,55296!=(64512&(i=e).charCodeAt(r))||r<0||r+1>=i.length||56320!=(64512&i.charCodeAt(r+1))?f[d++]=a>>12|224:(a=65536+((1023&a)<<10)+(1023&e.charCodeAt(++n)),f[d++]=a>>18|240,f[d++]=a>>12&63|128),f[d++]=a>>6&63|128,f[d++]=63&a|128)}else for(n=0;n<e.length;n++)f[n]=0|e[n];return f},i.toHex=function(e){for(var t="",i=0;i<e.length;i++)t+=r(e[i].toString(16));return t},i.htonl=d,i.toHex32=function(e,t){for(var i="",r=0;r<e.length;r++){var f=e[r];i+=n((f="little"===t?d(f):f).toString(16))}return i},i.zero2=r,i.zero8=n,i.join32=function(e,t,i,r){s((i-=t)%4==0);for(var f=new Array(i/4),d=0,n=t;d<f.length;d++,n+=4){var a="big"===r?e[n]<<24|e[n+1]<<16|e[n+2]<<8|e[n+3]:e[n+3]<<24|e[n+2]<<16|e[n+1]<<8|e[n];f[d]=a>>>0}return f},i.split32=function(e,t){for(var i=new Array(4*e.length),r=0,f=0;r<e.length;r++,f+=4){var d=e[r];"big"===t?(i[f]=d>>>24,i[f+1]=d>>>16&255,i[f+2]=d>>>8&255,i[f+3]=255&d):(i[f+3]=d>>>24,i[f+2]=d>>>16&255,i[f+1]=d>>>8&255,i[f]=255&d)}return i},i.rotr32=function(e,t){return e>>>t|e<<32-t},i.rotl32=function(e,t){return e<<t|e>>>32-t},i.sum32=function(e,t){return e+t>>>0},i.sum32_3=function(e,t,i){return e+t+i>>>0},i.sum32_4=function(e,t,i,r){return e+t+i+r>>>0},i.sum32_5=function(e,t,i,r,f){return e+t+i+r+f>>>0},i.sum64=function(e,t,i,r){var f=e[t],d=r+e[t+1]>>>0,f=(d<r?1:0)+i+f;e[t]=f>>>0,e[t+1]=d},i.sum64_hi=function(e,t,i,r){return(t+r>>>0<t?1:0)+e+i>>>0},i.sum64_lo=function(e,t,i,r){return t+r>>>0},i.sum64_4_hi=function(e,t,i,r,f,d,n,a){var s=0,c=t;return s+=(c=c+r>>>0)<t?1:0,s+=(c=c+d>>>0)<d?1:0,e+i+f+n+(s+=(c=c+a>>>0)<a?1:0)>>>0},i.sum64_4_lo=function(e,t,i,r,f,d,n,a){return t+r+d+a>>>0},i.sum64_5_hi=function(e,t,i,r,f,d,n,a,s,c){var h=0,o=t;return h+=(o=o+r>>>0)<t?1:0,h+=(o=o+d>>>0)<d?1:0,h+=(o=o+a>>>0)<a?1:0,e+i+f+n+s+(h+=(o=o+c>>>0)<c?1:0)>>>0},i.sum64_5_lo=function(e,t,i,r,f,d,n,a,s,c){return t+r+d+a+c>>>0},i.rotr64_hi=function(e,t,i){return(t<<32-i|e>>>i)>>>0},i.rotr64_lo=function(e,t,i){return(e<<32-i|t>>>i)>>>0},i.shr64_hi=function(e,t,i){return e>>>i},i.shr64_lo=function(e,t,i){return(e<<32-i|t>>>i)>>>0}},{inherits:32,"minimalistic-assert":33}],31:[function(e,t,i){"use strict";var r=e("hash.js"),d=e("minimalistic-crypto-utils"),f=e("minimalistic-assert");function n(e){if(!(this instanceof n))return new n(e);this.hash=e.hash,this.predResist=!!e.predResist,this.outLen=this.hash.outSize,this.minEntropy=e.minEntropy||this.hash.hmacStrength,this._reseed=null,this.reseedInterval=null,this.K=null,this.V=null;var t=d.toArray(e.entropy,e.entropyEnc||"hex"),i=d.toArray(e.nonce,e.nonceEnc||"hex"),e=d.toArray(e.pers,e.persEnc||"hex");f(t.length>=this.minEntropy/8,"Not enough entropy. Minimum is: "+this.minEntropy+" bits"),this._init(t,i,e)}(t.exports=n).prototype._init=function(e,t,i){i=e.concat(t).concat(i);this.K=new Array(this.outLen/8),this.V=new Array(this.outLen/8);for(var r=0;r<this.V.length;r++)this.K[r]=0,this.V[r]=1;this._update(i),this._reseed=1,this.reseedInterval=281474976710656},n.prototype._hmac=function(){return new r.hmac(this.hash,this.K)},n.prototype._update=function(e){var t=this._hmac().update(this.V).update([0]);e&&(t=t.update(e)),this.K=t.digest(),this.V=this._hmac().update(this.V).digest(),e&&(this.K=this._hmac().update(this.V).update([1]).update(e).digest(),this.V=this._hmac().update(this.V).digest())},n.prototype.reseed=function(e,t,i,r){"string"!=typeof t&&(r=i,i=t,t=null),e=d.toArray(e,t),i=d.toArray(i,r),f(e.length>=this.minEntropy/8,"Not enough entropy. Minimum is: "+this.minEntropy+" bits"),this._update(e.concat(i||[])),this._reseed=1},n.prototype.generate=function(e,t,i,r){if(this._reseed>this.reseedInterval)throw new Error("Reseed is required");"string"!=typeof t&&(r=i,i=t,t=null),i&&(i=d.toArray(i,r||"hex"),this._update(i));for(var f=[];f.length<e;)this.V=this._hmac().update(this.V).digest(),f=f.concat(this.V);r=f.slice(0,e);return this._update(i),this._reseed++,d.encode(r,t)}},{"hash.js":19,"minimalistic-assert":33,"minimalistic-crypto-utils":34}],32:[function(e,t,i){"function"==typeof Object.create?t.exports=function(e,t){t&&(e.super_=t,e.prototype=Object.create(t.prototype,{constructor:{value:e,enumerable:!1,writable:!0,configurable:!0}}))}:t.exports=function(e,t){var i;t&&(e.super_=t,(i=function(){}).prototype=t.prototype,e.prototype=new i,e.prototype.constructor=e)}},{}],33:[function(e,t,i){function r(e,t){if(!e)throw new Error(t||"Assertion failed")}(t.exports=r).equal=function(e,t,i){if(e!=t)throw new Error(i||"Assertion failed: "+e+" != "+t)}},{}],34:[function(e,t,i){"use strict";function r(e){return 1===e.length?"0"+e:e}function f(e){for(var t="",i=0;i<e.length;i++)t+=r(e[i].toString(16));return t}i.toArray=function(e,t){if(Array.isArray(e))return e.slice();if(!e)return[];var i=[];if("string"!=typeof e){for(var r=0;r<e.length;r++)i[r]=0|e[r];return i}if("hex"===t){(e=e.replace(/[^a-z0-9]+/gi,"")).length%2!=0&&(e="0"+e);for(r=0;r<e.length;r+=2)i.push(parseInt(e[r]+e[r+1],16))}else for(r=0;r<e.length;r++){var f=e.charCodeAt(r),d=f>>8,f=255&f;d?i.push(d,f):i.push(f)}return i},i.zero2=r,i.toHex=f,i.encode=function(e,t){return"hex"===t?f(e):e}},{}],35:[function(e,t,i){t.exports={name:"elliptic",version:"6.5.4",description:"EC cryptography",main:"lib/elliptic.js",files:["lib"],scripts:{lint:"eslint lib test","lint:fix":"npm run lint -- --fix",unit:"istanbul test _mocha --reporter=spec test/index.js",test:"npm run lint && npm run unit",version:"grunt dist && git add dist/"},repository:{type:"git",url:"git@github.com:indutny/elliptic"},keywords:["EC","Elliptic","curve","Cryptography"],author:"Fedor Indutny <fedor@indutny.com>",license:"MIT",bugs:{url:"https://github.com/indutny/elliptic/issues"},homepage:"https://github.com/indutny/elliptic",devDependencies:{brfs:"^2.0.2",coveralls:"^3.1.0",eslint:"^7.6.0",grunt:"^1.2.1","grunt-browserify":"^5.3.0","grunt-cli":"^1.3.2","grunt-contrib-connect":"^3.0.0","grunt-contrib-copy":"^1.0.0","grunt-contrib-uglify":"^5.0.0","grunt-mocha-istanbul":"^5.0.2","grunt-saucelabs":"^9.0.1",istanbul:"^0.4.5",mocha:"^8.0.1"},dependencies:{"bn.js":"^4.11.9",brorand:"^1.1.0","hash.js":"^1.0.0","hmac-drbg":"^1.0.1",inherits:"^2.0.4","minimalistic-assert":"^1.0.1","minimalistic-crypto-utils":"^1.0.1"}}},{}]},{},[1])(1)});
var elliptic = module.exports;
</script>
<script>
/* sjcl 1.0.8 - Stanford Javascript Crypto Library - Full build with HMAC & SHA256 */
"use strict";var sjcl={cipher:{},hash:{},keyexchange:{},mode:{},misc:{},codec:{},exception:{corrupt:function(a){this.toString=function(){return"CORRUPT: "+this.message};this.message=a},invalid:function(a){this.toString=function(){return"INVALID: "+this.message};this.message=a},bug:function(a){this.toString=function(){return"BUG: "+this.message};this.message=a},notReady:function(a){this.toString=function(){return"NOT READY: "+this.message};this.message=a}}};
sjcl.cipher.aes=function(a){this.s[0][0][0]||this.O();var b,c,d,e,f=this.s[0][4],g=this.s[1];b=a.length;var h=1;if(4!==b&&6!==b&&8!==b)throw new sjcl.exception.invalid("invalid aes key size");this.b=[d=a.slice(0),e=[]];for(a=b;a<4*b+28;a++){c=d[a-1];if(0===a%b||8===b&&4===a%b)c=f[c>>>24]<<24^f[c>>16&255]<<16^f[c>>8&255]<<8^f[c&255],0===a%b&&(c=c<<8^c>>>24^h<<24,h=h<<1^283*(h>>7));d[a]=d[a-b]^c}for(b=0;a;b++,a--)c=d[b&3?a:a-4],e[b]=4>=a||4>b?c:g[0][f[c>>>24]]^g[1][f[c>>16&255]]^g[2][f[c>>8&255]]^g[3][f[c&
255]]};
sjcl.cipher.aes.prototype={encrypt:function(a){return t(this,a,0)},decrypt:function(a){return t(this,a,1)},s:[[[],[],[],[],[]],[[],[],[],[],[]]],O:function(){var a=this.s[0],b=this.s[1],c=a[4],d=b[4],e,f,g,h=[],k=[],l,n,m,p;for(e=0;0x100>e;e++)k[(h[e]=e<<1^283*(e>>7))^e]=e;for(f=g=0;!c[f];f^=l||1,g=k[g]||1)for(m=g^g<<1^g<<2^g<<3^g<<4,m=m>>8^m&255^99,c[f]=m,d[m]=f,n=h[e=h[l=h[f]]],p=0x1010101*n^0x10001*e^0x101*l^0x1010100*f,n=0x101*h[m]^0x1010100*m,e=0;4>e;e++)a[e][f]=n=n<<24^n>>>8,b[e][m]=p=p<<24^p>>>8;for(e=
0;5>e;e++)a[e]=a[e].slice(0),b[e]=b[e].slice(0)}};
function t(a,b,c){if(4!==b.length)throw new sjcl.exception.invalid("invalid aes block size");var d=a.b[c],e=b[0]^d[0],f=b[c?3:1]^d[1],g=b[2]^d[2];b=b[c?1:3]^d[3];var h,k,l,n=d.length/4-2,m,p=4,r=[0,0,0,0];h=a.s[c];a=h[0];var q=h[1],v=h[2],w=h[3],x=h[4];for(m=0;m<n;m++)h=a[e>>>24]^q[f>>16&255]^v[g>>8&255]^w[b&255]^d[p],k=a[f>>>24]^q[g>>16&255]^v[b>>8&255]^w[e&255]^d[p+1],l=a[g>>>24]^q[b>>16&255]^v[e>>8&255]^w[f&255]^d[p+2],b=a[b>>>24]^q[e>>16&255]^v[f>>8&255]^w[g&255]^d[p+3],p+=4,e=h,f=k,g=l;for(m=
0;4>m;m++)r[c?3&-m:m]=x[e>>>24]<<24^x[f>>16&255]<<16^x[g>>8&255]<<8^x[b&255]^d[p++],h=e,e=f,f=g,g=b,b=h;return r}
sjcl.bitArray={bitSlice:function(a,b,c){a=sjcl.bitArray.$(a.slice(b/32),32-(b&31)).slice(1);return void 0===c?a:sjcl.bitArray.clamp(a,c-b)},extract:function(a,b,c){var d=Math.floor(-b-c&31);return((b+c-1^b)&-32?a[b/32|0]<<32-d^a[b/32+1|0]>>>d:a[b/32|0]>>>d)&(1<<c)-1},concat:function(a,b){if(0===a.length||0===b.length)return a.concat(b);var c=a[a.length-1],d=sjcl.bitArray.getPartial(c);return 32===d?a.concat(b):sjcl.bitArray.$(b,d,c|0,a.slice(0,a.length-1))},bitLength:function(a){var b=a.length;return 0===
b?0:32*(b-1)+sjcl.bitArray.getPartial(a[b-1])},clamp:function(a,b){if(32*a.length<b)return a;a=a.slice(0,Math.ceil(b/32));var c=a.length;b=b&31;0<c&&b&&(a[c-1]=sjcl.bitArray.partial(b,a[c-1]&2147483648>>b-1,1));return a},partial:function(a,b,c){return 32===a?b:(c?b|0:b<<32-a)+0x10000000000*a},getPartial:function(a){return Math.round(a/0x10000000000)||32},equal:function(a,b){if(sjcl.bitArray.bitLength(a)!==sjcl.bitArray.bitLength(b))return!1;var c=0,d;for(d=0;d<a.length;d++)c|=a[d]^b[d];return 0===
c},$:function(a,b,c,d){var e;e=0;for(void 0===d&&(d=[]);32<=b;b-=32)d.push(c),c=0;if(0===b)return d.concat(a);for(e=0;e<a.length;e++)d.push(c|a[e]>>>b),c=a[e]<<32-b;e=a.length?a[a.length-1]:0;a=sjcl.bitArray.getPartial(e);d.push(sjcl.bitArray.partial(b+a&31,32<b+a?c:d.pop(),1));return d},i:function(a,b){return[a[0]^b[0],a[1]^b[1],a[2]^b[2],a[3]^b[3]]},byteswapM:function(a){var b,c;for(b=0;b<a.length;++b)c=a[b],a[b]=c>>>24|c>>>8&0xff00|(c&0xff00)<<8|c<<24;return a}};
sjcl.codec.utf8String={fromBits:function(a){var b="",c=sjcl.bitArray.bitLength(a),d,e;for(d=0;d<c/8;d++)0===(d&3)&&(e=a[d/4]),b+=String.fromCharCode(e>>>8>>>8>>>8),e<<=8;return decodeURIComponent(escape(b))},toBits:function(a){a=unescape(encodeURIComponent(a));var b=[],c,d=0;for(c=0;c<a.length;c++)d=d<<8|a.charCodeAt(c),3===(c&3)&&(b.push(d),d=0);c&3&&b.push(sjcl.bitArray.partial(8*(c&3),d));return b}};
sjcl.codec.hex={fromBits:function(a){var b="",c;for(c=0;c<a.length;c++)b+=((a[c]|0)+0xf00000000000).toString(16).substr(4);return b.substr(0,sjcl.bitArray.bitLength(a)/4)},toBits:function(a){var b,c=[],d;a=a.replace(/\s|0x/g,"");d=a.length;a=a+"00000000";for(b=0;b<a.length;b+=8)c.push(parseInt(a.substr(b,8),16)^0);return sjcl.bitArray.clamp(c,4*d)}};
sjcl.codec.base32={B:"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567",X:"0123456789ABCDEFGHIJKLMNOPQRSTUV",BITS:32,BASE:5,REMAINING:27,fromBits:function(a,b,c){var d=sjcl.codec.base32.BASE,e=sjcl.codec.base32.REMAINING,f="",g=0,h=sjcl.codec.base32.B,k=0,l=sjcl.bitArray.bitLength(a);c&&(h=sjcl.codec.base32.X);for(c=0;f.length*d<l;)f+=h.charAt((k^a[c]>>>g)>>>e),g<d?(k=a[c]<<d-g,g+=e,c++):(k<<=d,g-=d);for(;f.length&7&&!b;)f+="=";return f},toBits:function(a,b){a=a.replace(/\s|=/g,"").toUpperCase();var c=sjcl.codec.base32.BITS,
d=sjcl.codec.base32.BASE,e=sjcl.codec.base32.REMAINING,f=[],g,h=0,k=sjcl.codec.base32.B,l=0,n,m="base32";b&&(k=sjcl.codec.base32.X,m="base32hex");for(g=0;g<a.length;g++){n=k.indexOf(a.charAt(g));if(0>n){if(!b)try{return sjcl.codec.base32hex.toBits(a)}catch(p){}throw new sjcl.exception.invalid("this isn't "+m+"!");}h>e?(h-=e,f.push(l^n>>>h),l=n<<c-h):(h+=d,l^=n<<c-h)}h&56&&f.push(sjcl.bitArray.partial(h&56,l,1));return f}};
sjcl.codec.base32hex={fromBits:function(a,b){return sjcl.codec.base32.fromBits(a,b,1)},toBits:function(a){return sjcl.codec.base32.toBits(a,1)}};
sjcl.codec.base64={B:"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",fromBits:function(a,b,c){var d="",e=0,f=sjcl.codec.base64.B,g=0,h=sjcl.bitArray.bitLength(a);c&&(f=f.substr(0,62)+"-_");for(c=0;6*d.length<h;)d+=f.charAt((g^a[c]>>>e)>>>26),6>e?(g=a[c]<<6-e,e+=26,c++):(g<<=6,e-=6);for(;d.length&3&&!b;)d+="=";return d},toBits:function(a,b){a=a.replace(/\s|=/g,"");var c=[],d,e=0,f=sjcl.codec.base64.B,g=0,h;b&&(f=f.substr(0,62)+"-_");for(d=0;d<a.length;d++){h=f.indexOf(a.charAt(d));
if(0>h)throw new sjcl.exception.invalid("this isn't base64!");26<e?(e-=26,c.push(g^h>>>e),g=h<<32-e):(e+=6,g^=h<<32-e)}e&56&&c.push(sjcl.bitArray.partial(e&56,g,1));return c}};sjcl.codec.base64url={fromBits:function(a){return sjcl.codec.base64.fromBits(a,1,1)},toBits:function(a){return sjcl.codec.base64.toBits(a,1)}};sjcl.hash.sha256=function(a){this.b[0]||this.O();a?(this.F=a.F.slice(0),this.A=a.A.slice(0),this.l=a.l):this.reset()};sjcl.hash.sha256.hash=function(a){return(new sjcl.hash.sha256).update(a).finalize()};
sjcl.hash.sha256.prototype={blockSize:512,reset:function(){this.F=this.Y.slice(0);this.A=[];this.l=0;return this},update:function(a){"string"===typeof a&&(a=sjcl.codec.utf8String.toBits(a));var b,c=this.A=sjcl.bitArray.concat(this.A,a);b=this.l;a=this.l=b+sjcl.bitArray.bitLength(a);if(0x1fffffffffffff<a)throw new sjcl.exception.invalid("Cannot hash more than 2^53 - 1 bits");if("undefined"!==typeof Uint32Array){var d=new Uint32Array(c),e=0;for(b=512+b-(512+b&0x1ff);b<=a;b+=512)u(this,d.subarray(16*e,
16*(e+1))),e+=1;c.splice(0,16*e)}else for(b=512+b-(512+b&0x1ff);b<=a;b+=512)u(this,c.splice(0,16));return this},finalize:function(){var a,b=this.A,c=this.F,b=sjcl.bitArray.concat(b,[sjcl.bitArray.partial(1,1)]);for(a=b.length+2;a&15;a++)b.push(0);b.push(Math.floor(this.l/0x100000000));for(b.push(this.l|0);b.length;)u(this,b.splice(0,16));this.reset();return c},Y:[],b:[],O:function(){function a(a){return 0x100000000*(a-Math.floor(a))|0}for(var b=0,c=2,d,e;64>b;c++){e=!0;for(d=2;d*d<=c;d++)if(0===c%d){e=
!1;break}e&&(8>b&&(this.Y[b]=a(Math.pow(c,.5))),this.b[b]=a(Math.pow(c,1/3)),b++)}}};
function u(a,b){var c,d,e,f=a.F,g=a.b,h=f[0],k=f[1],l=f[2],n=f[3],m=f[4],p=f[5],r=f[6],q=f[7];for(c=0;64>c;c++)16>c?d=b[c]:(d=b[c+1&15],e=b[c+14&15],d=b[c&15]=(d>>>7^d>>>18^d>>>3^d<<25^d<<14)+(e>>>17^e>>>19^e>>>10^e<<15^e<<13)+b[c&15]+b[c+9&15]|0),d=d+q+(m>>>6^m>>>11^m>>>25^m<<26^m<<21^m<<7)+(r^m&(p^r))+g[c],q=r,r=p,p=m,m=n+d|0,n=l,l=k,k=h,h=d+(k&l^n&(k^l))+(k>>>2^k>>>13^k>>>22^k<<30^k<<19^k<<10)|0;f[0]=f[0]+h|0;f[1]=f[1]+k|0;f[2]=f[2]+l|0;f[3]=f[3]+n|0;f[4]=f[4]+m|0;f[5]=f[5]+p|0;f[6]=f[6]+r|0;f[7]=
f[7]+q|0}
sjcl.mode.ccm={name:"ccm",G:[],listenProgress:function(a){sjcl.mode.ccm.G.push(a)},unListenProgress:function(a){a=sjcl.mode.ccm.G.indexOf(a);-1<a&&sjcl.mode.ccm.G.splice(a,1)},fa:function(a){var b=sjcl.mode.ccm.G.slice(),c;for(c=0;c<b.length;c+=1)b[c](a)},encrypt:function(a,b,c,d,e){var f,g=b.slice(0),h=sjcl.bitArray,k=h.bitLength(c)/8,l=h.bitLength(g)/8;e=e||64;d=d||[];if(7>k)throw new sjcl.exception.invalid("ccm: iv must be at least 7 bytes");for(f=2;4>f&&l>>>8*f;f++);f<15-k&&(f=15-k);c=h.clamp(c,
8*(15-f));b=sjcl.mode.ccm.V(a,b,c,d,e,f);g=sjcl.mode.ccm.C(a,g,c,b,e,f);return h.concat(g.data,g.tag)},decrypt:function(a,b,c,d,e){e=e||64;d=d||[];var f=sjcl.bitArray,g=f.bitLength(c)/8,h=f.bitLength(b),k=f.clamp(b,h-e),l=f.bitSlice(b,h-e),h=(h-e)/8;if(7>g)throw new sjcl.exception.invalid("ccm: iv must be at least 7 bytes");for(b=2;4>b&&h>>>8*b;b++);b<15-g&&(b=15-g);c=f.clamp(c,8*(15-b));k=sjcl.mode.ccm.C(a,k,c,l,e,b);a=sjcl.mode.ccm.V(a,k.data,c,d,e,b);if(!f.equal(k.tag,a))throw new sjcl.exception.corrupt("ccm: tag doesn't match");
return k.data},na:function(a,b,c,d,e,f){var g=[],h=sjcl.bitArray,k=h.i;d=[h.partial(8,(b.length?64:0)|d-2<<2|f-1)];d=h.concat(d,c);d[3]|=e;d=a.encrypt(d);if(b.length)for(c=h.bitLength(b)/8,65279>=c?g=[h.partial(16,c)]:0xffffffff>=c&&(g=h.concat([h.partial(16,65534)],[c])),g=h.concat(g,b),b=0;b<g.length;b+=4)d=a.encrypt(k(d,g.slice(b,b+4).concat([0,0,0])));return d},V:function(a,b,c,d,e,f){var g=sjcl.bitArray,h=g.i;e/=8;if(e%2||4>e||16<e)throw new sjcl.exception.invalid("ccm: invalid tag length");
if(0xffffffff<d.length||0xffffffff<b.length)throw new sjcl.exception.bug("ccm: can't deal with 4GiB or more data");c=sjcl.mode.ccm.na(a,d,c,e,g.bitLength(b)/8,f);for(d=0;d<b.length;d+=4)c=a.encrypt(h(c,b.slice(d,d+4).concat([0,0,0])));return g.clamp(c,8*e)},C:function(a,b,c,d,e,f){var g,h=sjcl.bitArray;g=h.i;var k=b.length,l=h.bitLength(b),n=k/50,m=n;c=h.concat([h.partial(8,f-1)],c).concat([0,0,0]).slice(0,4);d=h.bitSlice(g(d,a.encrypt(c)),0,e);if(!k)return{tag:d,data:[]};for(g=0;g<k;g+=4)g>n&&(sjcl.mode.ccm.fa(g/
k),n+=m),c[3]++,e=a.encrypt(c),b[g]^=e[0],b[g+1]^=e[1],b[g+2]^=e[2],b[g+3]^=e[3];return{tag:d,data:h.clamp(b,l)}}};
sjcl.mode.ocb2={name:"ocb2",encrypt:function(a,b,c,d,e,f){if(128!==sjcl.bitArray.bitLength(c))throw new sjcl.exception.invalid("ocb iv must be 128 bits");var g,h=sjcl.mode.ocb2.S,k=sjcl.bitArray,l=k.i,n=[0,0,0,0];c=h(a.encrypt(c));var m,p=[];d=d||[];e=e||64;for(g=0;g+4<b.length;g+=4)m=b.slice(g,g+4),n=l(n,m),p=p.concat(l(c,a.encrypt(l(c,m)))),c=h(c);m=b.slice(g);b=k.bitLength(m);g=a.encrypt(l(c,[0,0,0,b]));m=k.clamp(l(m.concat([0,0,0]),g),b);n=l(n,l(m.concat([0,0,0]),g));n=a.encrypt(l(n,l(c,h(c))));
d.length&&(n=l(n,f?d:sjcl.mode.ocb2.pmac(a,d)));return p.concat(k.concat(m,k.clamp(n,e)))},decrypt:function(a,b,c,d,e,f){if(128!==sjcl.bitArray.bitLength(c))throw new sjcl.exception.invalid("ocb iv must be 128 bits");e=e||64;var g=sjcl.mode.ocb2.S,h=sjcl.bitArray,k=h.i,l=[0,0,0,0],n=g(a.encrypt(c)),m,p,r=sjcl.bitArray.bitLength(b)-e,q=[];d=d||[];for(c=0;c+4<r/32;c+=4)m=k(n,a.decrypt(k(n,b.slice(c,c+4)))),l=k(l,m),q=q.concat(m),n=g(n);p=r-32*c;m=a.encrypt(k(n,[0,0,0,p]));m=k(m,h.clamp(b.slice(c),p).concat([0,
0,0]));l=k(l,m);l=a.encrypt(k(l,k(n,g(n))));d.length&&(l=k(l,f?d:sjcl.mode.ocb2.pmac(a,d)));if(!h.equal(h.clamp(l,e),h.bitSlice(b,r)))throw new sjcl.exception.corrupt("ocb: tag doesn't match");return q.concat(h.clamp(m,p))},pmac:function(a,b){var c,d=sjcl.mode.ocb2.S,e=sjcl.bitArray,f=e.i,g=[0,0,0,0],h=a.encrypt([0,0,0,0]),h=f(h,d(d(h)));for(c=0;c+4<b.length;c+=4)h=d(h),g=f(g,a.encrypt(f(h,b.slice(c,c+4))));c=b.slice(c);128>e.bitLength(c)&&(h=f(h,d(h)),c=e.concat(c,[-2147483648,0,0,0]));g=f(g,c);
return a.encrypt(f(d(f(h,d(h))),g))},S:function(a){return[a[0]<<1^a[1]>>>31,a[1]<<1^a[2]>>>31,a[2]<<1^a[3]>>>31,a[3]<<1^135*(a[0]>>>31)]}};
sjcl.mode.gcm={name:"gcm",encrypt:function(a,b,c,d,e){var f=b.slice(0);b=sjcl.bitArray;d=d||[];a=sjcl.mode.gcm.C(!0,a,f,d,c,e||128);return b.concat(a.data,a.tag)},decrypt:function(a,b,c,d,e){var f=b.slice(0),g=sjcl.bitArray,h=g.bitLength(f);e=e||128;d=d||[];e<=h?(b=g.bitSlice(f,h-e),f=g.bitSlice(f,0,h-e)):(b=f,f=[]);a=sjcl.mode.gcm.C(!1,a,f,d,c,e);if(!g.equal(a.tag,b))throw new sjcl.exception.corrupt("gcm: tag doesn't match");return a.data},ka:function(a,b){var c,d,e,f,g,h=sjcl.bitArray.i;e=[0,0,
0,0];f=b.slice(0);for(c=0;128>c;c++){(d=0!==(a[Math.floor(c/32)]&1<<31-c%32))&&(e=h(e,f));g=0!==(f[3]&1);for(d=3;0<d;d--)f[d]=f[d]>>>1|(f[d-1]&1)<<31;f[0]>>>=1;g&&(f[0]^=-0x1f000000)}return e},j:function(a,b,c){var d,e=c.length;b=b.slice(0);for(d=0;d<e;d+=4)b[0]^=0xffffffff&c[d],b[1]^=0xffffffff&c[d+1],b[2]^=0xffffffff&c[d+2],b[3]^=0xffffffff&c[d+3],b=sjcl.mode.gcm.ka(b,a);return b},C:function(a,b,c,d,e,f){var g,h,k,l,n,m,p,r,q=sjcl.bitArray;m=c.length;p=q.bitLength(c);r=q.bitLength(d);h=q.bitLength(e);
g=b.encrypt([0,0,0,0]);96===h?(e=e.slice(0),e=q.concat(e,[1])):(e=sjcl.mode.gcm.j(g,[0,0,0,0],e),e=sjcl.mode.gcm.j(g,e,[0,0,Math.floor(h/0x100000000),h&0xffffffff]));h=sjcl.mode.gcm.j(g,[0,0,0,0],d);n=e.slice(0);d=h.slice(0);a||(d=sjcl.mode.gcm.j(g,h,c));for(l=0;l<m;l+=4)n[3]++,k=b.encrypt(n),c[l]^=k[0],c[l+1]^=k[1],c[l+2]^=k[2],c[l+3]^=k[3];c=q.clamp(c,p);a&&(d=sjcl.mode.gcm.j(g,h,c));a=[Math.floor(r/0x100000000),r&0xffffffff,Math.floor(p/0x100000000),p&0xffffffff];d=sjcl.mode.gcm.j(g,d,a);k=b.encrypt(e);
d[0]^=k[0];d[1]^=k[1];d[2]^=k[2];d[3]^=k[3];return{tag:q.bitSlice(d,0,f),data:c}}};sjcl.misc.hmac=function(a,b){this.W=b=b||sjcl.hash.sha256;var c=[[],[]],d,e=b.prototype.blockSize/32;this.w=[new b,new b];a.length>e&&(a=b.hash(a));for(d=0;d<e;d++)c[0][d]=a[d]^909522486,c[1][d]=a[d]^1549556828;this.w[0].update(c[0]);this.w[1].update(c[1]);this.R=new b(this.w[0])};
sjcl.misc.hmac.prototype.encrypt=sjcl.misc.hmac.prototype.mac=function(a){if(this.aa)throw new sjcl.exception.invalid("encrypt on already updated hmac called!");this.update(a);return this.digest(a)};sjcl.misc.hmac.prototype.reset=function(){this.R=new this.W(this.w[0]);this.aa=!1};sjcl.misc.hmac.prototype.update=function(a){this.aa=!0;this.R.update(a)};sjcl.misc.hmac.prototype.digest=function(){var a=this.R.finalize(),a=(new this.W(this.w[1])).update(a).finalize();this.reset();return a};
sjcl.misc.pbkdf2=function(a,b,c,d,e){c=c||1E4;if(0>d||0>c)throw new sjcl.exception.invalid("invalid params to pbkdf2");"string"===typeof a&&(a=sjcl.codec.utf8String.toBits(a));"string"===typeof b&&(b=sjcl.codec.utf8String.toBits(b));e=e||sjcl.misc.hmac;a=new e(a);var f,g,h,k,l=[],n=sjcl.bitArray;for(k=1;32*l.length<(d||1);k++){e=f=a.encrypt(n.concat(b,[k]));for(g=1;g<c;g++)for(f=a.encrypt(f),h=0;h<f.length;h++)e[h]^=f[h];l=l.concat(e)}d&&(l=n.clamp(l,d));return l};
sjcl.prng=function(a){this.c=[new sjcl.hash.sha256];this.m=[0];this.P=0;this.H={};this.N=0;this.U={};this.Z=this.f=this.o=this.ha=0;this.b=[0,0,0,0,0,0,0,0];this.h=[0,0,0,0];this.L=void 0;this.M=a;this.D=!1;this.K={progress:{},seeded:{}};this.u=this.ga=0;this.I=1;this.J=2;this.ca=0x10000;this.T=[0,48,64,96,128,192,0x100,384,512,768,1024];this.da=3E4;this.ba=80};
sjcl.prng.prototype={randomWords:function(a,b){var c=[],d;d=this.isReady(b);var e;if(d===this.u)throw new sjcl.exception.notReady("generator isn't seeded");if(d&this.J){d=!(d&this.I);e=[];var f=0,g;this.Z=e[0]=(new Date).valueOf()+this.da;for(g=0;16>g;g++)e.push(0x100000000*Math.random()|0);for(g=0;g<this.c.length&&(e=e.concat(this.c[g].finalize()),f+=this.m[g],this.m[g]=0,d||!(this.P&1<<g));g++);this.P>=1<<this.c.length&&(this.c.push(new sjcl.hash.sha256),this.m.push(0));this.f-=f;f>this.o&&(this.o=
f);this.P++;this.b=sjcl.hash.sha256.hash(this.b.concat(e));this.L=new sjcl.cipher.aes(this.b);for(d=0;4>d&&(this.h[d]=this.h[d]+1|0,!this.h[d]);d++);}for(d=0;d<a;d+=4)0===(d+1)%this.ca&&y(this),e=z(this),c.push(e[0],e[1],e[2],e[3]);y(this);return c.slice(0,a)},setDefaultParanoia:function(a,b){if(0===a&&"Setting paranoia=0 will ruin your security; use it only for testing"!==b)throw new sjcl.exception.invalid("Setting paranoia=0 will ruin your security; use it only for testing");this.M=a},addEntropy:function(a,
b,c){c=c||"user";var d,e,f=(new Date).valueOf(),g=this.H[c],h=this.isReady(),k=0;d=this.U[c];void 0===d&&(d=this.U[c]=this.ha++);void 0===g&&(g=this.H[c]=0);this.H[c]=(this.H[c]+1)%this.c.length;switch(typeof a){case "number":void 0===b&&(b=1);this.c[g].update([d,this.N++,1,b,f,1,a|0]);break;case "object":c=Object.prototype.toString.call(a);if("[object Uint32Array]"===c){e=[];for(c=0;c<a.length;c++)e.push(a[c]);a=e}else for("[object Array]"!==c&&(k=1),c=0;c<a.length&&!k;c++)"number"!==typeof a[c]&&
(k=1);if(!k){if(void 0===b)for(c=b=0;c<a.length;c++)for(e=a[c];0<e;)b++,e=e>>>1;this.c[g].update([d,this.N++,2,b,f,a.length].concat(a))}break;case "string":void 0===b&&(b=a.length);this.c[g].update([d,this.N++,3,b,f,a.length]);this.c[g].update(a);break;default:k=1}if(k)throw new sjcl.exception.bug("random: addEntropy only supports number, array of numbers or string");this.m[g]+=b;this.f+=b;h===this.u&&(this.isReady()!==this.u&&A("seeded",Math.max(this.o,this.f)),A("progress",this.getProgress()))},
isReady:function(a){a=this.T[void 0!==a?a:this.M];return this.o&&this.o>=a?this.m[0]>this.ba&&(new Date).valueOf()>this.Z?this.J|this.I:this.I:this.f>=a?this.J|this.u:this.u},getProgress:function(a){a=this.T[a?a:this.M];return this.o>=a?1:this.f>a?1:this.f/a},startCollectors:function(){if(!this.D){this.a={loadTimeCollector:B(this,this.ma),mouseCollector:B(this,this.oa),keyboardCollector:B(this,this.la),accelerometerCollector:B(this,this.ea),touchCollector:B(this,this.qa)};if(window.addEventListener)window.addEventListener("load",
this.a.loadTimeCollector,!1),window.addEventListener("mousemove",this.a.mouseCollector,!1),window.addEventListener("keypress",this.a.keyboardCollector,!1),window.addEventListener("devicemotion",this.a.accelerometerCollector,!1),window.addEventListener("touchmove",this.a.touchCollector,!1);else if(document.attachEvent)document.attachEvent("onload",this.a.loadTimeCollector),document.attachEvent("onmousemove",this.a.mouseCollector),document.attachEvent("keypress",this.a.keyboardCollector);else throw new sjcl.exception.bug("can't attach event");
this.D=!0}},stopCollectors:function(){this.D&&(window.removeEventListener?(window.removeEventListener("load",this.a.loadTimeCollector,!1),window.removeEventListener("mousemove",this.a.mouseCollector,!1),window.removeEventListener("keypress",this.a.keyboardCollector,!1),window.removeEventListener("devicemotion",this.a.accelerometerCollector,!1),window.removeEventListener("touchmove",this.a.touchCollector,!1)):document.detachEvent&&(document.detachEvent("onload",this.a.loadTimeCollector),document.detachEvent("onmousemove",
this.a.mouseCollector),document.detachEvent("keypress",this.a.keyboardCollector)),this.D=!1)},addEventListener:function(a,b){this.K[a][this.ga++]=b},removeEventListener:function(a,b){var c,d,e=this.K[a],f=[];for(d in e)e.hasOwnProperty(d)&&e[d]===b&&f.push(d);for(c=0;c<f.length;c++)d=f[c],delete e[d]},la:function(){C(this,1)},oa:function(a){var b,c;try{b=a.x||a.clientX||a.offsetX||0,c=a.y||a.clientY||a.offsetY||0}catch(d){c=b=0}0!=b&&0!=c&&this.addEntropy([b,c],2,"mouse");C(this,0)},qa:function(a){a=
a.touches[0]||a.changedTouches[0];this.addEntropy([a.pageX||a.clientX,a.pageY||a.clientY],1,"touch");C(this,0)},ma:function(){C(this,2)},ea:function(a){a=a.accelerationIncludingGravity.x||a.accelerationIncludingGravity.y||a.accelerationIncludingGravity.z;if(window.orientation){var b=window.orientation;"number"===typeof b&&this.addEntropy(b,1,"accelerometer")}a&&this.addEntropy(a,2,"accelerometer");C(this,0)}};
function A(a,b){var c,d=sjcl.random.K[a],e=[];for(c in d)d.hasOwnProperty(c)&&e.push(d[c]);for(c=0;c<e.length;c++)e[c](b)}function C(a,b){"undefined"!==typeof window&&window.performance&&"function"===typeof window.performance.now?a.addEntropy(window.performance.now(),b,"loadtime"):a.addEntropy((new Date).valueOf(),b,"loadtime")}function y(a){a.b=z(a).concat(z(a));a.L=new sjcl.cipher.aes(a.b)}function z(a){for(var b=0;4>b&&(a.h[b]=a.h[b]+1|0,!a.h[b]);b++);return a.L.encrypt(a.h)}
function B(a,b){return function(){b.apply(a,arguments)}}sjcl.random=new sjcl.prng(6);
a:try{var D,E,F,G;if(G="undefined"!==typeof module&&module.exports){var H;try{H=require("crypto")}catch(a){H=null}G=E=H}if(G&&E.randomBytes)D=E.randomBytes(128),D=new Uint32Array((new Uint8Array(D)).buffer),sjcl.random.addEntropy(D,1024,"crypto['randomBytes']");else if("undefined"!==typeof window&&"undefined"!==typeof Uint32Array){F=new Uint32Array(32);if(window.crypto&&window.crypto.getRandomValues)window.crypto.getRandomValues(F);else if(window.msCrypto&&window.msCrypto.getRandomValues)window.msCrypto.getRandomValues(F);
else break a;sjcl.random.addEntropy(F,1024,"crypto['getRandomValues']")}}catch(a){"undefined"!==typeof window&&window.console&&(console.log("There was an error collecting entropy from the browser:"),console.log(a))}
sjcl.json={defaults:{v:1,iter:1E4,ks:128,ts:64,mode:"ccm",adata:"",cipher:"aes"},ja:function(a,b,c,d){c=c||{};d=d||{};var e=sjcl.json,f=e.g({iv:sjcl.random.randomWords(4,0)},e.defaults),g;e.g(f,c);c=f.adata;"string"===typeof f.salt&&(f.salt=sjcl.codec.base64.toBits(f.salt));"string"===typeof f.iv&&(f.iv=sjcl.codec.base64.toBits(f.iv));if(!sjcl.mode[f.mode]||!sjcl.cipher[f.cipher]||"string"===typeof a&&100>=f.iter||64!==f.ts&&96!==f.ts&&128!==f.ts||128!==f.ks&&192!==f.ks&&0x100!==f.ks||2>f.iv.length||
4<f.iv.length)throw new sjcl.exception.invalid("json encrypt: invalid parameters");"string"===typeof a?(g=sjcl.misc.cachedPbkdf2(a,f),a=g.key.slice(0,f.ks/32),f.salt=g.salt):sjcl.ecc&&a instanceof sjcl.ecc.elGamal.publicKey&&(g=a.kem(),f.kemtag=g.tag,a=g.key.slice(0,f.ks/32));"string"===typeof b&&(b=sjcl.codec.utf8String.toBits(b));"string"===typeof c&&(f.adata=c=sjcl.codec.utf8String.toBits(c));g=new sjcl.cipher[f.cipher](a);e.g(d,f);d.key=a;f.ct="ccm"===f.mode&&sjcl.arrayBuffer&&sjcl.arrayBuffer.ccm&&
b instanceof ArrayBuffer?sjcl.arrayBuffer.ccm.encrypt(g,b,f.iv,c,f.ts):sjcl.mode[f.mode].encrypt(g,b,f.iv,c,f.ts);return f},encrypt:function(a,b,c,d){var e=sjcl.json,f=e.ja.apply(e,arguments);return e.encode(f)},ia:function(a,b,c,d){c=c||{};d=d||{};var e=sjcl.json;b=e.g(e.g(e.g({},e.defaults),b),c,!0);var f,g;f=b.adata;"string"===typeof b.salt&&(b.salt=sjcl.codec.base64.toBits(b.salt));"string"===typeof b.iv&&(b.iv=sjcl.codec.base64.toBits(b.iv));if(!sjcl.mode[b.mode]||!sjcl.cipher[b.cipher]||"string"===
typeof a&&100>=b.iter||64!==b.ts&&96!==b.ts&&128!==b.ts||128!==b.ks&&192!==b.ks&&0x100!==b.ks||!b.iv||2>b.iv.length||4<b.iv.length)throw new sjcl.exception.invalid("json decrypt: invalid parameters");"string"===typeof a?(g=sjcl.misc.cachedPbkdf2(a,b),a=g.key.slice(0,b.ks/32),b.salt=g.salt):sjcl.ecc&&a instanceof sjcl.ecc.elGamal.secretKey&&(a=a.unkem(sjcl.codec.base64.toBits(b.kemtag)).slice(0,b.ks/32));"string"===typeof f&&(f=sjcl.codec.utf8String.toBits(f));g=new sjcl.cipher[b.cipher](a);f="ccm"===
b.mode&&sjcl.arrayBuffer&&sjcl.arrayBuffer.ccm&&b.ct instanceof ArrayBuffer?sjcl.arrayBuffer.ccm.decrypt(g,b.ct,b.iv,b.tag,f,b.ts):sjcl.mode[b.mode].decrypt(g,b.ct,b.iv,f,b.ts);e.g(d,b);d.key=a;return 1===c.raw?f:sjcl.codec.utf8String.fromBits(f)},decrypt:function(a,b,c,d){var e=sjcl.json;return e.ia(a,e.decode(b),c,d)},encode:function(a){var b,c="{",d="";for(b in a)if(a.hasOwnProperty(b)){if(!b.match(/^[a-z0-9]+$/i))throw new sjcl.exception.invalid("json encode: invalid property name");c+=d+'"'+
b+'":';d=",";switch(typeof a[b]){case "number":case "boolean":c+=a[b];break;case "string":c+='"'+escape(a[b])+'"';break;case "object":c+='"'+sjcl.codec.base64.fromBits(a[b],0)+'"';break;default:throw new sjcl.exception.bug("json encode: unsupported type");}}return c+"}"},decode:function(a){a=a.replace(/\s/g,"");if(!a.match(/^\{.*\}$/))throw new sjcl.exception.invalid("json decode: this isn't json!");a=a.replace(/^\{|\}$/g,"").split(/,/);var b={},c,d;for(c=0;c<a.length;c++){if(!(d=a[c].match(/^\s*(?:(["']?)([a-z][a-z0-9]*)\1)\s*:\s*(?:(-?\d+)|"([a-z0-9+\/%*_.@=\-]*)"|(true|false))$/i)))throw new sjcl.exception.invalid("json decode: this isn't json!");
null!=d[3]?b[d[2]]=parseInt(d[3],10):null!=d[4]?b[d[2]]=d[2].match(/^(ct|adata|salt|iv)$/)?sjcl.codec.base64.toBits(d[4]):unescape(d[4]):null!=d[5]&&(b[d[2]]="true"===d[5])}return b},g:function(a,b,c){void 0===a&&(a={});if(void 0===b)return a;for(var d in b)if(b.hasOwnProperty(d)){if(c&&void 0!==a[d]&&a[d]!==b[d])throw new sjcl.exception.invalid("required parameter overridden");a[d]=b[d]}return a},sa:function(a,b){var c={},d;for(d in a)a.hasOwnProperty(d)&&a[d]!==b[d]&&(c[d]=a[d]);return c},ra:function(a,
b){var c={},d;for(d=0;d<b.length;d++)void 0!==a[b[d]]&&(c[b[d]]=a[b[d]]);return c}};sjcl.encrypt=sjcl.json.encrypt;sjcl.decrypt=sjcl.json.decrypt;sjcl.misc.pa={};sjcl.misc.cachedPbkdf2=function(a,b){var c=sjcl.misc.pa,d;b=b||{};d=b.iter||1E3;c=c[a]=c[a]||{};d=c[d]=c[d]||{firstSalt:b.salt&&b.salt.length?b.salt.slice(0):sjcl.random.randomWords(2,0)};c=void 0===b.salt?d.firstSalt:b.salt;d[c]=d[c]||sjcl.misc.pbkdf2(a,c,b.iter);return{key:d[c].slice(0),salt:c.slice(0)}};
"undefined"!==typeof module&&module.exports&&(module.exports=sjcl);"function"===typeof define&&define([],function(){return sjcl});
</script>
</head><body><h2 data-i18n="app.title">Authenticator Control Panel</h2><div id="status" class="status-message" style="display:none;"></div>
<div class="tabs">
    <button class="tab-link active user-activity" onclick="openTab(event, 'Keys')" data-i18n="tab.keys">Keys</button>
    <button class="tab-link user-activity" onclick="openTab(event, 'Passwords')" data-i18n="tab.passwords">Passwords</button>
    <button class="tab-link user-activity" onclick="openTab(event, 'Display')" data-i18n="tab.display">Display</button>
    <button class="tab-link user-activity" onclick="openTab(event, 'Pin')" data-i18n="tab.pin">PIN</button>
    <button class="tab-link user-activity" onclick="openTab(event, 'Settings')" data-i18n="tab.settings">Settings</button>
</div>
<div id="widget-bar"><div id="lang-switcher"><span id="lang-flag">🇬🇧</span><div id="lang-dropdown" style="display:none;"><div class="lang-option active" data-lang="en">🇬🇧 English</div><div class="lang-option" data-lang="ru">🇷🇺 Русский</div><div class="lang-option" data-lang="de">🇩🇪 Deutsch</div><div class="lang-option" data-lang="zh">🇨🇳 中文</div><div class="lang-option" data-lang="es">🇪🇸 Español</div></div></div><div id="battery-widget"></div></div>
<div id="Keys" class="tab-content" style="display:block;">
    <h3 data-i18n="keys.title">Manage Keys</h3>
    <div class="form-container">
        <h4 data-i18n="keys.add.title">Add New Key</h4>
        <form id="add-key-form">
            <label for="key-name" data-i18n="keys.field.name">Name:</label>
            <input type="text" id="key-name" name="name" class="user-activity" required style="width: 100%; box-sizing: border-box;">
            <label for="key-secret" data-i18n="keys.field.secret">Secret (Base32):</label>
            <div style="position: relative; margin-bottom: 15px;">
                <input type="text" id="key-secret" name="secret" class="user-activity" required style="width: 100%; padding-right: 42px; margin-bottom: 0; box-sizing: border-box;">
                <button type="button" id="scan-qr-file" title="Upload QR Image" style="position: absolute; right: 5px; top: 50%; transform: translateY(-50%); background: rgba(90, 158, 238, 0.2); border: 1px solid rgba(90, 158, 238, 0.5); border-radius: 5px; width: 32px; height: 32px; cursor: pointer; font-size: 16px; display: flex; align-items: center; justify-content: center; transition: all 0.3s ease;">🖼️</button>
                <input type="file" id="qr-file-input" accept="image/*" style="display: none;">
            </div>
            <div id="qr-status" style="display: none; margin: 10px 0; padding: 10px; border-radius: 5px; text-align: center; font-weight: 500;"></div>
            
            <!-- Hidden element for QR scanner library -->
            <div id="qr-reader" style="display: none;"></div>
            
            <details>
                <summary style="cursor:pointer;color:#5a9eee;margin:10px 0;">⚙️ <span data-i18n="keys.advanced">Advanced Settings</span></summary>
                
                <div style="margin: 20px 0;">
                    <label style="display: block; margin-bottom: 10px;">Type:</label>
                    <div style="display: flex; gap: 15px; align-items: center;">
                        <label style="display: flex; align-items: center; cursor: pointer;">
                            <input type="radio" name="key-type" value="T" id="type-totp" checked class="user-activity" style="width: auto; margin: 0 8px 0 0;">
                            <span data-i18n="keys.type.totp">TOTP (Time-based)</span>
                        </label>
                        <label style="display: flex; align-items: center; cursor: pointer;">
                            <input type="radio" name="key-type" value="H" id="type-hotp" class="user-activity" style="width: auto; margin: 0 8px 0 0;">
                            <span data-i18n="keys.type.hotp">HOTP (Counter-based)</span>
                        </label>
                    </div>
                </div>
                <label for="key-algorithm" data-i18n="keys.field.algorithm">Algorithm:</label>
                <select id="key-algorithm" name="algorithm" class="user-activity" style="width: 100%; box-sizing: border-box;">
                    <option value="1" selected data-i18n="keys.algo.sha1">SHA1 (Standard)</option>
                    <option value="256">SHA256</option>
                    <option value="512">SHA512</option>
                </select>
                <div style="margin: 20px 0;">
                    <label style="display: block; margin-bottom: 10px;" data-i18n="keys.field.digits">Digits:</label>
                    <div style="display: flex; gap: 15px; align-items: center;">
                        <label style="display: flex; align-items: center; cursor: pointer;">
                            <input type="radio" name="key-digits" value="6" id="digits-6" checked class="user-activity" style="width: auto; margin: 0 8px 0 0;">
                            <span data-i18n="keys.digits.6">6 digits</span>
                        </label>
                        <label style="display: flex; align-items: center; cursor: pointer;">
                            <input type="radio" name="key-digits" value="8" id="digits-8" class="user-activity" style="width: auto; margin: 0 8px 0 0;">
                            <span data-i18n="keys.digits.8">8 digits</span>
                        </label>
                    </div>
                </div>
                <div id="period-container" style="margin: 20px 0;">
                    <label style="display: block; margin-bottom: 10px;" data-i18n="keys.field.period">Period:</label>
                    <div style="display: flex; gap: 15px; align-items: center;">
                        <label style="display: flex; align-items: center; cursor: pointer;">
                            <input type="radio" name="key-period" value="30" id="period-30" checked class="user-activity" style="width: auto; margin: 0 8px 0 0;">
                            <span data-i18n="keys.period.30">30 seconds</span>
                        </label>
                        <label style="display: flex; align-items: center; cursor: pointer;">
                            <input type="radio" name="key-period" value="60" id="period-60" class="user-activity" style="width: auto; margin: 0 8px 0 0;">
                            <span data-i18n="keys.period.60">60 seconds</span>
                        </label>
                    </div>
                </div>
                <div id="counter-container" style="display: none;">
                    <label for="key-counter">Initial Counter:</label>
                    <input type="number" id="key-counter" name="counter" value="0" min="0" class="user-activity" style="width: 100%; box-sizing: border-box;">
                    <small style="color: #b0b0b0; display: block; margin-top: 5px;">Counter increments after each code generation</small>
                </div>
                
            </details>
            
            <button type="submit" class="button user-activity" data-i18n="keys.add.btn">Add Key</button>
        </form>
    </div>
    <div class="content-box">
        <h4 data-i18n="keys.current.title">Current Keys</h4>
        <table id="keys-table">
            <thead><tr><th>::</th><th data-i18n="keys.col.name">Name</th><th data-i18n="keys.col.code">Code</th><th data-i18n="keys.col.timer">Timer</th><th data-i18n="keys.col.progress">Progress</th><th data-i18n="keys.col.actions">Actions</th></tr></thead>
            <tbody></tbody>
        </table>
        <p style="color:#888;font-size:0.8em;margin-top:6px;text-align:center;">ℹ️ <span data-i18n="keys.hotp.sync">HOTP counters are synced together with TOTP keys</span></p>
    </div>
    <div class="form-container">
        <h4 data-i18n="keys.importexport.title">Import/Export Keys</h4>
        <div class="api-access-container">
            <p><strong data-i18n="keys.api.status">API Status:</strong> <span class="api-status" style="font-weight:bold; color:#ffc107;" data-i18n="keys.api.inactive">Inactive</span></p>
            <button class="enable-api-btn button user-activity" data-i18n="keys.api.enable">Enable API Access (5 min)</button>
        </div>
        <div id="import-export-buttons" style="margin-top: 15px;">
            <button id="export-keys-btn" class="button-action user-activity" disabled data-i18n="keys.btn.export">Export Keys</button>
            <button id="import-keys-btn" class="button-action user-activity" disabled data-i18n="keys.btn.import">Import Keys</button>
            <input type="file" id="import-file" style="display: none;" accept=".json" class="user-activity">
        </div>
    </div>
</div>

<div id="Passwords" class="tab-content">
    <h3 data-i18n="passwords.title">Manage Passwords</h3>
    <div class="form-container">
        <h4 data-i18n="passwords.add.title">Add New Password</h4>
        <form id="add-password-form">
            <label for="password-name" data-i18n="passwords.field.name">Name:</label>
            <input type="text" id="password-name" name="name" class="user-activity" required>
            <label for="password-value" data-i18n="passwords.field.password">Password:</label>
            <div class="password-input-container">
                <input type="text" id="password-value" name="password" class="user-activity" required>
                <span class="password-generate" onclick="openPasswordGeneratorModal()" title="Generate Password">#</span>
            </div>
            <button type="submit" class="button user-activity" data-i18n="passwords.add.btn">Add Password</button>
        </form>
    </div>
    <div class="content-box">
        <h4 data-i18n="passwords.current.title">Current Passwords</h4>
        <table id="passwords-table">
            <thead><tr><th>::</th><th data-i18n="passwords.col.name">Name</th><th data-i18n="passwords.col.actions">Actions</th></tr></thead>
            <tbody></tbody>
        </table>
    </div>
    <div class="form-container">
        <h4 data-i18n="passwords.importexport.title">Import/Export Passwords</h4>
        <div class="api-access-container">
            <p><strong data-i18n="keys.api.status">API Status:</strong> <span class="api-status" style="font-weight:bold; color:#ffc107;" data-i18n="keys.api.inactive">Inactive</span></p>
            <button class="enable-api-btn button user-activity" data-i18n="keys.api.enable">Enable API Access (5 min)</button>
        </div>
        <div id="import-export-buttons-passwords" style="margin-top: 15px;">
            <button id="export-passwords-btn" class="button-action user-activity" disabled data-i18n="passwords.btn.export">Export Passwords</button>
            <button id="import-passwords-btn" class="button-action user-activity" disabled data-i18n="passwords.btn.import">Import Passwords</button>
            <input type="file" id="import-passwords-file" style="display: none;" accept=".json" class="user-activity">
        </div>
    </div>
</div>

<div id="Display" class="tab-content">
    <h3 data-i18n="display.title">Display Settings</h3>
    <div class="form-container">
        <h4 data-i18n="display.theme.title">Theme Selection</h4>
        <form id="theme-selection-form">
            <label><input type="radio" name="theme" value="light" id="theme-light" class="user-activity"> <span data-i18n="display.theme.light">Light Theme</span></label><br>
            <label><input type="radio" name="theme" value="dark" id="theme-dark" class="user-activity"> <span data-i18n="display.theme.dark">Dark Theme</span></label><br>
            <button type="submit" data-i18n="display.theme.btn" class="button user-activity">Apply Theme</button>
        </form>
    </div>
    <div class="form-container">
        <h4 data-i18n="display.splash.title">Splash Screen</h4>
        
        <div style="margin-bottom: 20px;">
            <label for="splash-mode-select" data-i18n="display.splash.label" style="font-weight: bold; display: block; margin-bottom: 10px;">Embedded Splash Mode:</label>
            <select id="splash-mode-select" class="user-activity" style="width: 100%; padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; margin-bottom: 10px;">
                <option value="disabled" data-i18n="display.splash.disabled">Disabled (No splash screen)</option>
                <option value="securegen">SecureGen</option>
                <option value="bladerunner">BladeRunner</option>
                <option value="combs">Combs</option>
            </select>
            <button id="save-splash-mode-btn" data-i18n="display.splash.btn" class="button user-activity">Save Mode</button>
        </div>
        <!-- Custom splash upload removed for security - only embedded splash screens available -->
    </div>

    <div class="form-container">
        <h4 data-i18n="display.clock.title">Clock Display</h4>
        <div style="margin-bottom: 20px;">
            <label for="timezone-select" data-i18n="display.clock.timezone" style="font-weight: bold; display: block; margin-bottom: 10px;">Timezone:</label>
            <select id="timezone-select" class="user-activity" style="width: 100%; padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; margin-bottom: 10px;">
                <option value="UTC0">UTC</option>
                <option value="MSK-3">Moscow (UTC+3)</option>
                <option value="TRT-3">Istanbul (UTC+3)</option>
                <option value="GST-4">Dubai (UTC+4)</option>
                <option value="PKT-5">Karachi (UTC+5)</option>
                <option value="IST-5:30">India (UTC+5:30)</option>
                <option value="CST-8">China/Beijing (UTC+8)</option>
                <option value="JST-9">Tokyo (UTC+9)</option>
                <option value="AEST-10">Sydney (UTC+10)</option>
                <option value="GMT0BST,M3.5.0/1,M10.5.0">London (UTC+0/+1)</option>
                <option value="CET-1CEST,M3.5.0,M10.5.0/3">Berlin/Paris (UTC+1/+2)</option>
                <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Helsinki/Kyiv (UTC+2/+3)</option>
                <option value="EST5EDT,M3.2.0,M11.1.0">New York (UTC-5/-4)</option>
                <option value="CST6CDT,M3.2.0,M11.1.0">Chicago (UTC-6/-5)</option>
                <option value="MST7MDT,M3.2.0,M11.1.0">Denver (UTC-7/-6)</option>
                <option value="PST8PDT,M3.2.0,M11.1.0">Los Angeles (UTC-8/-7)</option>
            </select>
            <button id="save-timezone-btn" data-i18n="display.clock.save.tz" class="button user-activity">Save Timezone</button>
        </div>
        <div style="margin-top:20px;padding-top:15px;border-top:1px solid rgba(255,255,255,0.1);">
            <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:12px;">
                <div>
                    <label for="rtc-enabled" data-i18n="display.rtc.title" style="font-weight:bold;cursor:pointer;">DS3231 RTC Module</label>
                    <p data-i18n="display.rtc.desc" style="margin:4px 0 0 0;font-size:12px;opacity:0.6;">Hardware clock for offline/AP time</p>
                </div>
                <label class="switch">
                    <input type="checkbox" id="rtc-enabled" class="user-activity">
                    <span class="slider"></span>
                </label>
            </div>
            <div id="rtc-pin-config" style="display:none;margin-bottom:12px;">
                <div style="display:flex;gap:12px;">
                    <div style="flex:1;">
                        <label data-i18n="display.rtc.sda" style="font-size:12px;opacity:0.7;display:block;margin-bottom:4px;">SDA Pin</label>
                        <input type="number" id="rtc-sda" value="21" min="0" max="39" class="user-activity"
                               style="width:100%;padding:6px;border-radius:4px;border:1px solid rgba(255,255,255,0.2);background:rgba(255,255,255,0.05);color:inherit;">
                    </div>
                    <div style="flex:1;">
                        <label data-i18n="display.rtc.scl" style="font-size:12px;opacity:0.7;display:block;margin-bottom:4px;">SCL Pin</label>
                        <input type="number" id="rtc-scl" value="22" min="0" max="39" class="user-activity"
                               style="width:100%;padding:6px;border-radius:4px;border:1px solid rgba(255,255,255,0.2);background:rgba(255,255,255,0.05);color:inherit;">
                    </div>
                </div>
            </div>
            <div id="rtc-status-row" style="display:none;margin-bottom:12px;font-size:13px;align-items:center;">
                <span id="rtc-status-badge" style="padding:3px 10px;border-radius:12px;font-size:11px;font-weight:bold;"></span>
                <span id="rtc-time-display" style="margin-left:10px;opacity:0.7;"></span>
            </div>
            <button id="rtc-sync-btn" data-i18n="display.rtc.btn" class="button user-activity" style="width:100%;">Sync &amp; Save</button>
            <p id="rtc-sync-hint" style="font-size:12px;opacity:0.6;margin-top:8px;"></p>
        </div>
    </div>

    <div class="form-container">
        <h4 data-i18n="display.lock.title">Screen &amp; Lock Settings</h4>
        <form id="display-timeout-form">
            <label for="display-timeout" data-i18n="display.lock.screen.label">Screen timeout (turn off display after):</label>
            <select id="display-timeout" name="display_timeout" required class="user-activity" style="width: 100%; padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; margin-bottom: 16px;">
                <option value="15" data-i18n="display.lock.opt.15s">15 seconds</option>
                <option value="30" data-i18n="display.lock.opt.30s">30 seconds</option>
                <option value="60" data-i18n="display.lock.opt.1m">1 minute</option>
                <option value="300" data-i18n="display.lock.opt.5m">5 minutes</option>
                <option value="1800" data-i18n="display.lock.opt.30m">30 minutes</option>
                <option value="0" data-i18n="display.lock.never">Never</option>
            </select>
            
            <label for="auto-lock-timeout" data-i18n="display.lock.auto.label">Auto lock (deep sleep + PIN required after):</label>
            <select id="auto-lock-timeout" name="auto_lock_timeout" required class="user-activity" style="width: 100%; padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; margin-bottom: 6px;">
                <option value="300" data-i18n="display.lock.opt.5m">5 minutes</option>
                <option value="900" data-i18n="display.lock.opt.15m">15 minutes</option>
                <option value="1800" data-i18n="display.lock.opt.30m">30 minutes</option>
                <option value="3600" data-i18n="display.lock.opt.1h">1 hour</option>
                <option value="14400" data-i18n="display.lock.opt.4h">4 hours</option>
                <option value="0" data-i18n="display.lock.never">Never</option>
            </select>
            <p data-i18n="display.lock.warning" style="font-size:12px;opacity:0.6;margin-top:0;margin-bottom:12px;">Auto lock must be longer than screen timeout when both are enabled.</p>
            
            <button type="submit" data-i18n="display.lock.btn" class="button user-activity">Save Settings</button>
        </form>
    </div>
</div>

<div id="Pin" class="tab-content">
    <h3 data-i18n="pin.title">PIN Code Settings</h3>
    <div class="form-container">
        <form id="pincode-settings-form">
            <!-- Startup PIN Protection -->
            <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 20px; padding: 15px; background: rgba(255,255,255,0.05); border-radius: 8px;">
                <div>
                    <label for="pin-enabled-device" data-i18n="pin.startup.label" style="font-size: 16px; font-weight: 500; margin: 0;">Startup PIN</label>
                    <p data-i18n="pin.startup.desc" style="margin: 5px 0 0 0; font-size: 13px; color: rgba(255,255,255,0.6);">Encrypt device key with PIN on startup</p>
                </div>
                <label class="switch">
                    <input type="checkbox" id="pin-enabled-device" name="enabledForDevice" class="user-activity">
                    <span class="slider"></span>
                </label>
            </div>
            
            <button type="submit" data-i18n="pin.startup.btn" class="button user-activity">Save Startup PIN Settings</button>
        </form>
    </div>

    <!-- BLE Bonding PIN Section -->
    <div class="form-container" style="margin-top: 30px; border-top: 1px solid rgba(255,255,255,0.1); padding-top: 25px;">
        <h4 data-i18n="pin.ble.title" style="color: #4a90e2; margin-bottom: 15px;">BLE Bonding PIN</h4>
        <p style="color: #888; font-size: 0.9em; margin-bottom: 20px;"><strong>Security Notice:</strong> <span data-i18n="pin.ble.notice">This PIN is displayed on device screen during BLE pairing for client authentication.</span></p>
        
        <form id="ble-bonding-pin-form">
            <label for="ble-bonding-pin" data-i18n="pin.ble.label.new">New BLE Bonding PIN (6 digits):</label>
            <div class="password-input-container">
                <input type="password" id="ble-bonding-pin" name="ble_bonding_pin" pattern="\d{6}" maxlength="6" placeholder="Enter 6-digit PIN" data-i18n="pin.placeholder.enter" data-i18n-attr="placeholder" class="user-activity">
                <span class="password-toggle" onclick="togglePasswordVisibility('ble-bonding-pin', this)">O</span>
            </div>
            
            <label for="ble-bonding-pin-confirm" data-i18n="pin.ble.label.confirm">Confirm BLE Bonding PIN:</label>
            <div class="password-input-container">
                <input type="password" id="ble-bonding-pin-confirm" name="ble_bonding_pin_confirm" pattern="\d{6}" maxlength="6" placeholder="Confirm 6-digit PIN" data-i18n="pin.placeholder.confirm" data-i18n-attr="placeholder" class="user-activity">
                <span class="password-toggle" onclick="togglePasswordVisibility('ble-bonding-pin-confirm', this)">O</span>
            </div>
            
            <div style="margin: 15px 0; padding: 12px; background: rgba(255,193,7,0.1); border: 1px solid rgba(255,193,7,0.3); border-radius: 6px;">
                <small style="color: #ffc107; font-size: 0.85rem;">
                    <strong>Important:</strong> <span data-i18n="pin.ble.important">This PIN will be displayed on the ESP32 screen during BLE pairing for clients to enter.</span>
                </small>
            </div>

            <button type="submit" data-i18n="pin.ble.btn" class="button user-activity" style="background-color: #28a745;">Update BLE Bonding PIN</button>
        </form>
    </div>

    <!-- Device BLE PIN Section -->
    <div class="form-container" style="margin-top: 30px; border-top: 1px solid rgba(255,255,255,0.1); padding-top: 25px;">
        <h4 data-i18n="pin.device.title" style="color: #4a90e2; margin-bottom: 15px;">Device BLE PIN</h4>
        <p style="color: #888; font-size: 0.9em; margin-bottom: 20px;"><strong>Security Notice:</strong> <span data-i18n="pin.device.notice">This PIN is required on the hardware device when transmitting passwords via BLE.</span></p>
        
        <!-- Switch для включения/выключения Device BLE PIN -->
        <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 20px; padding: 15px; background: rgba(255,255,255,0.05); border-radius: 8px;">
            <div>
                <label for="device-ble-pin-enabled" data-i18n="pin.device.label.enable" style="font-size: 16px; font-weight: 500; margin: 0;">Enable Device BLE PIN</label>
                <p data-i18n="pin.device.desc" style="margin: 5px 0 0 0; font-size: 13px; color: rgba(255,255,255,0.6);">Require PIN on device for BLE password transmission</p>
            </div>
            <label class="switch">
                <input type="checkbox" id="device-ble-pin-enabled" class="user-activity" onchange="toggleDeviceBlePinFields()">
                <span class="slider"></span>
            </label>
        </div>

        <!-- Поля ввода PIN (показываются при включении switch) -->
        <div id="device-ble-pin-fields" style="display: none;">
            <form id="device-ble-pin-form">
                <label for="device-ble-pin" data-i18n="pin.device.label.new">New Device BLE PIN (6 digits):</label>
                <div class="password-input-container">
                    <input type="password" id="device-ble-pin" name="device_ble_pin" pattern="\d{6}" maxlength="6" placeholder="Enter 6-digit PIN" data-i18n="pin.placeholder.enter" data-i18n-attr="placeholder" class="user-activity">
                    <span class="password-toggle" onclick="togglePasswordVisibility('device-ble-pin', this)">O</span>
                </div>
                
                <label for="device-ble-pin-confirm" data-i18n="pin.device.label.confirm">Confirm Device BLE PIN:</label>
                <div class="password-input-container">
                    <input type="password" id="device-ble-pin-confirm" name="device_ble_pin_confirm" pattern="\d{6}" maxlength="6" placeholder="Confirm 6-digit PIN" data-i18n="pin.placeholder.confirm" data-i18n-attr="placeholder" class="user-activity">
                    <span class="password-toggle" onclick="togglePasswordVisibility('device-ble-pin-confirm', this)">O</span>
                </div>
                
                <div style="margin: 15px 0; padding: 12px; background: rgba(255,193,7,0.1); border: 1px solid rgba(255,193,7,0.3); border-radius: 6px;">
                    <small style="color: #ffc107; font-size: 0.85rem;">
                        <strong>Important:</strong> <span data-i18n="pin.device.important">This PIN will be required on the device screen when transmitting passwords via BLE.</span>
                    </small>
                </div>

                <button type="submit" data-i18n="pin.device.btn" class="button user-activity" style="background-color: #28a745;">Save Device BLE PIN</button>
            </form>
        </div>
    </div>
</div>

<div id="Settings" class="tab-content">
    <h3 data-i18n="settings.title">Device Settings</h3>
    <div class="form-container">
        <h4 data-i18n="settings.pwd.title">Password Management</h4>
        
        <!-- Password Type Selector -->
        <div class="password-type-selector">
            <div class="toggle-container">
                <div class="toggle-option active web-active" id="web-password-toggle">
                    <span class="toggle-icon">🔒</span>
                    <span data-i18n="settings.pwd.toggle.web">Web Cabinet</span>
                </div>
                <div class="toggle-option" id="wifi-password-toggle">
                    <span class="toggle-icon">📶</span>
                    <span data-i18n="settings.pwd.toggle.wifi">WiFi Access Point</span>
                </div>
            </div>
        </div>
        
        <!-- Dynamic Form Title -->
        <div class="password-form-title" id="password-form-title">
            <span class="title-icon">🔒</span>
            <span id="password-form-title-text" data-i18n="settings.pwd.web.form.title">Change Web Cabinet Password</span>
        </div>
        
        <!-- Dynamic Description -->
        <div class="password-type-description" id="password-type-description">
            <span data-i18n="settings.pwd.web.desc">Change the password for accessing this web interface.</span>
        </div>
        
        <div class="login-display-container">
            <p><span data-i18n="settings.pwd.current.login">Current Login:</span> <strong id="current-admin-login">Loading...</strong></p>
        </div>
        <hr class="modern-hr">
        <form id="change-password-form">
            <label for="new-password" id="new-password-label" data-i18n="settings.pwd.web.new.label">New Web Password</label>
            <div class="password-input-container">
                <input type="password" id="new-password" name="new-password" required class="user-activity">
                <span class="password-toggle" onclick="togglePasswordVisibility('new-password', this)">O</span>
            </div>
            <ul class="password-criteria">
                <li id="pwd-length" data-i18n="settings.pwd.req.length">At least 8 characters</li>
                <li id="pwd-uppercase" data-i18n="settings.pwd.req.upper">An uppercase letter</li>
                <li id="pwd-lowercase" data-i18n="settings.pwd.req.lower">A lowercase letter</li>
                <li id="pwd-number" data-i18n="settings.pwd.req.number">A number</li>
                <li id="pwd-special" data-i18n="settings.pwd.req.special">A special character (!@#$%)</li>
            </ul>
            <label for="confirm-password" id="confirm-password-label" data-i18n="settings.pwd.web.confirm.label">Confirm New Web Password</label>
            <div class="password-input-container">
                <input type="password" id="confirm-password" name="confirm-password" required class="user-activity">
                <span class="password-toggle" onclick="togglePasswordVisibility('confirm-password', this)">O</span>
            </div>
            <div id="password-confirm-message"></div>
            <button type="submit" id="change-password-btn" data-i18n="settings.pwd.web.btn" class="button user-activity" disabled>Change Password</button>
        </form>
    </div>
    <div class="form-container">
        <h4 data-i18n="settings.ble.title">Bluetooth Settings</h4>
        <form id="ble-settings-form">
            <label for="ble-device-name" data-i18n="settings.ble.label">BLE Device Name (max 15 chars):</label>
            <input type="text" id="ble-device-name" name="device_name" maxlength="15" required class="user-activity">
            <button type="submit" data-i18n="settings.ble.btn" class="button user-activity">Save BLE Name</button>
        </form>
    </div>
    <div class="form-container">
        <h4 data-i18n="settings.mdns.title">mDNS Settings</h4>
        <form id="mdns-settings-form">
            <label for="mdns-hostname" data-i18n="settings.mdns.label">mDNS Hostname (e.g., 't-disp-totp'):</label>
            <input type="text" id="mdns-hostname" name="hostname" maxlength="63" required class="user-activity">
            <button type="submit" data-i18n="settings.mdns.btn" class="button user-activity">Save mDNS Hostname</button>
        </form>
    </div>
    <div class="form-container">
        <h4 data-i18n="settings.wifi.title">📶 WiFi Network</h4>
        <div class="content-box">
            <div class="form-group">
                <label for="wifi-ssid" data-i18n="settings.wifi.ssid.label">Network SSID</label>
                <div style="display:flex;gap:8px;align-items:flex-start;">
                    <div style="position:relative;flex:1;">
                        <input type="text" id="wifi-ssid" placeholder="WiFi network name" data-i18n="settings.wifi.ssid.placeholder" data-i18n-attr="placeholder" autocomplete="off" maxlength="64" style="width:100%;box-sizing:border-box;">
                        <div id="wifi-scan-dropdown" style="display:none;position:absolute;top:100%;left:0;right:0;z-index:100;background:var(--bg-secondary, #1e1e2e);border:1px solid rgba(255,255,255,0.15);border-radius:6px;max-height:180px;overflow-y:auto;margin-top:2px;"></div>
                    </div>
                    <button type="button" id="wifi-scan-btn" data-i18n="settings.scan.btn" class="btn-secondary user-activity" style="white-space:nowrap;flex-shrink:0;">📡 Scan</button>
                </div>
            </div>
            <div class="form-group">
                <label for="wifi-password" data-i18n="settings.wifi.pwd.label">Password</label>
                <div class="password-input-container">
                    <input type="password" id="wifi-password" placeholder="WiFi password (min 8 chars)" data-i18n="settings.wifi.pwd.placeholder" data-i18n-attr="placeholder" autocomplete="new-password" maxlength="64">
                    <span class="password-toggle" onclick="togglePasswordVisibility('wifi-password', this)">O</span>
                </div>
            </div>
            <div class="form-group">
                <label for="wifi-password-confirm" data-i18n="settings.wifi.confirm.label">Confirm Password</label>
                <div class="password-input-container">
                    <input type="password" id="wifi-password-confirm" placeholder="Repeat password" data-i18n="settings.wifi.confirm.placeholder" data-i18n-attr="placeholder" autocomplete="new-password" maxlength="64">
                    <span class="password-toggle" onclick="togglePasswordVisibility('wifi-password-confirm', this)">O</span>
                </div>
            </div>
            <button id="save-wifi-credentials-btn" data-i18n="settings.wifi.btn" class="button user-activity">Save WiFi Credentials</button>
            <div id="wifi-credentials-status" class="status-message" style="display:none;"></div>
            <p class="settings-note" data-i18n="settings.wifi.note">⚠️ Changes apply after reboot.</p>
        </div>
    </div>
    <div class="form-container">
        <h4 data-i18n="settings.startup.title">Startup Mode</h4>
        <form id="startup-mode-form">
            <label for="startup-mode" data-i18n="settings.startup.label">Default mode on startup:</label>
            <select id="startup-mode" name="startup_mode" required class="user-activity" style="width: 100%; padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; margin-bottom: 10px;">
                <option value="totp" data-i18n="settings.startup.opt.totp">TOTP Authenticator</option>
                <option value="password" data-i18n="settings.startup.opt.pwd">Password Manager</option>
            </select>
            <button type="submit" data-i18n="settings.startup.btn" class="button user-activity">Save Startup Mode</button>
        </form>
    </div>
    <div class="form-container">
        <h4 data-i18n="settings.boot.title">Boot Mode</h4>
        <form id="boot-mode-form">
            <label for="boot-mode-select" data-i18n="settings.boot.label">Default network mode on boot:</label>
            <select id="boot-mode-select" name="boot_mode" required class="user-activity" style="width: 100%; padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; margin-bottom: 10px;">
                <option value="wifi" data-i18n="settings.boot.opt.wifi">WiFi Mode</option>
                <option value="ap" data-i18n="settings.boot.opt.ap">AP Mode</option>
                <option value="offline" data-i18n="settings.boot.opt.offline">Offline Mode</option>
            </select>
            <p data-i18n="settings.boot.note" style="font-size:12px;opacity:0.6;margin:0 0 10px 0;">Override available via buttons during boot (2 sec window).</p>
            <button id="save-boot-mode-btn" data-i18n="settings.boot.btn" type="button" class="button user-activity">Save Boot Mode</button>
        </form>
    </div>
    <div class="form-container">
        <h4 data-i18n="settings.webserver.title">Web Server</h4>
        <form id="web-server-settings-form">
            <label for="web-server-timeout" data-i18n="settings.webserver.label">Auto-shutdown on inactivity:</label>
            <select id="web-server-timeout" name="web_server_timeout" required class="user-activity" style="width: 100%; padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; margin-bottom: 10px;">
                <option value="5" data-i18n="settings.webserver.opt.5m">5 minutes</option>
                <option value="10" data-i18n="settings.webserver.opt.10m">10 minutes</option>
                <option value="60" data-i18n="settings.webserver.opt.1h">1 hour</option>
                <option value="0" data-i18n="settings.webserver.opt.never">Never</option>
            </select>
            <button type="submit" data-i18n="settings.webserver.btn" class="button user-activity">Save Setting</button>
        </form>
    </div>
    <div class="form-container">
        <h4 data-i18n="settings.session.title">Auto-Logout Timer</h4>
        <form id="session-duration-form">
            <label for="session-duration" data-i18n="settings.session.label">How long to stay logged in:</label>
            <select id="session-duration" name="session_duration" required class="user-activity" style="width: 100%; padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; margin-bottom: 10px;">
                <option value="0" data-i18n="settings.session.opt.reboot">Until device reboot</option>
                <option value="1" data-i18n="settings.session.opt.1h">1 hour</option>
                <option value="6" data-i18n="settings.session.opt.6h">6 hours (default)</option>
                <option value="24" data-i18n="settings.session.opt.24h">24 hours</option>
                <option value="72" data-i18n="settings.session.opt.3d">3 days</option>
            </select>
            <div style="margin: 15px 0; padding: 12px; background: rgba(76,175,80,0.1); border: 1px solid rgba(76,175,80,0.3); border-radius: 6px;">
                <small style="color: #81c784; font-size: 0.85rem;">
                    <strong>Security Feature:</strong> <span data-i18n="settings.session.notice">Controls automatic logout timing for enhanced device security. Sessions survive device restarts except Until reboot mode which requires fresh login after power cycle. Longer durations reduce login frequency but may compromise security if device is lost or stolen.</span>
                </small>
            </div>
            <button type="submit" data-i18n="settings.session.btn" class="button user-activity">Save Auto-Logout Timer</button>
        </form>
    </div>
    <div class="form-container">
        <h4 data-i18n="settings.system.title">System</h4>
        <button id="reboot-btn" data-i18n="settings.system.reboot" class="button-action user-activity">Reboot Device</button>
        <button id="reboot-with-web-btn" data-i18n="settings.system.reboot.web" class="button user-activity">Reboot with Web Server</button>
        <button id="clear-ble-clients-btn" data-i18n="settings.system.ble.clear" class="button-action user-activity">Clear BLE Clients</button>
        <button onclick="logout()" data-i18n="settings.system.logout" class="button-delete user-activity">Logout</button>
    </div>
</div>

<!-- Password Modal for Import/Export -->
<div id="password-modal" class="modal">
    <div class="modal-content">
        <span class="close" onclick="closePasswordModal()">&times;</span>
        <h3 id="modal-title">Enter Admin Password</h3>
        <p id="modal-description"></p>
        <div class="form-group">
            <label for="modal-password" data-i18n="passwords.modal.password">Password:</label>
            <div style="position:relative;">
                <input type="password" id="modal-password" style="width:100%;padding-right:40px;box-sizing:border-box;" class="user-activity">
                <button type="button" onclick="toggleModalPassword()" style="position:absolute;right:6px;top:50%;transform:translateY(-50%);background:none;border:none;cursor:pointer;font-size:18px;color:#aaa;padding:0;" title="Show/hide password">👁️</button>
            </div>
        </div>
        <button id="modal-submit-btn" class="button user-activity" data-i18n="passwords.modal.confirm">Confirm</button>
    </div>
</div>

<!-- Password Generator Modal -->
<div id="password-generator-modal" class="modal">
    <div class="modal-content">
        <span class="close" onclick="closePasswordGeneratorModal()">&times;</span>
        <h3 data-i18n="passwords.gen.title">Password Generator</h3>
        <p data-i18n="passwords.gen.desc">Choose password length and generate a secure password</p>
        <div class="form-group">
            <label for="password-length-slider"><span data-i18n="passwords.gen.length">Password Length:</span> <span id="length-display">14</span></label>
            <input type="range" id="password-length-slider" min="4" max="64" value="14" style="width: 100%; margin: 15px 0;">
            <div style="display: flex; justify-content: space-between; font-size: 0.8rem; color: #b0b0b0; margin-bottom: 20px;">
                <span>4</span><span>64</span>
            </div>
        </div>
        <div class="form-group">
            <label for="generated-password" data-i18n="passwords.gen.generated">Generated Password:</label>
            <div class="password-input-container">
                <input type="text" id="generated-password" readonly style="width: calc(100% - 24px); font-family: monospace; background: rgba(90, 158, 238, 0.1);" class="user-activity">
                <span class="password-toggle" onclick="togglePasswordVisibility('generated-password', this)">O</span>
            </div>
            <div class="password-strength-container">
                <div class="password-strength-bar">
                    <div class="password-strength-fill" id="strength-fill"></div>
                </div>
                <div class="password-strength-text" id="strength-text">Encryption Key</div>
            </div>
        </div>
        <div style="display: flex; gap: 10px; margin-top: 20px;">
            <button id="generate-new-btn" class="button user-activity" onclick="generatePassword()" data-i18n="passwords.gen.btn.generate">Generate New</button>
            <button id="use-password-btn" class="button user-activity" onclick="useGeneratedPassword()" data-i18n="passwords.gen.btn.use">Use This Password</button>
        </div>
    </div>
</div>

<!-- Password Edit Modal -->
<div id="password-edit-modal" class="modal">
    <div class="modal-content">
        <span class="close" onclick="closePasswordEditModal()">&times;</span>
        <h3 data-i18n="passwords.edit.title">Edit Password</h3>
        <p data-i18n="passwords.edit.desc">Change the name and password for this entry</p>
        <div class="form-group">
            <label for="edit-password-name" data-i18n="passwords.edit.name">Name:</label>
            <input type="text" id="edit-password-name" style="width: calc(100% - 24px);" class="user-activity" required>
        </div>
        <div class="form-group">
            <label for="edit-password-value" data-i18n="passwords.edit.password">Password:</label>
            <div class="password-input-container">
                <input type="text" id="edit-password-value" style="width: calc(100% - 52px); font-family: monospace;" class="user-activity" required>
                <span class="password-generate" onclick="generatePasswordForEdit()" title="Generate Password">#</span>
                <span class="password-toggle" onclick="togglePasswordVisibility('edit-password-value', this)">O</span>
            </div>
            <div class="password-strength-container">
                <div class="password-strength-bar">
                    <div class="password-strength-fill" id="edit-strength-fill"></div>
                </div>
                <div class="password-strength-text" id="edit-strength-text">Weak Password</div>
            </div>
        </div>
        <div style="display: flex; gap: 10px; margin-top: 20px; justify-content: flex-end;">
            <button class="button-action user-activity" onclick="closePasswordEditModal()" data-i18n="passwords.edit.btn.cancel">Cancel</button>
            <button id="save-password-btn" class="button user-activity" onclick="savePasswordEdit()" data-i18n="passwords.edit.btn.save">Save</button>
        </div>
    </div>
</div>

<script>
// ⚡ CACHE MANAGER - localStorage кеширование для снижения нагрузки на ESP32
const CacheManager = {
    PREFIX: 'totp_cache_',
    TTL: 5 * 60 * 1000, // 5 минут в миллисекундах
    
    set(key, data) {
        try {
            const cacheEntry = {
                data: data,
                timestamp: Date.now(),
                ttl: this.TTL
            };
            localStorage.setItem(this.PREFIX + key, JSON.stringify(cacheEntry));
            // 📉 Убран DEBUG лог - повторяется очень часто
        } catch (e) {
            console.warn('⚠️ Cache storage failed:', e.message);
        }
    },
    
    get(key) {
        try {
            const cached = localStorage.getItem(this.PREFIX + key);
            if (!cached) return null;
            
            const cacheEntry = JSON.parse(cached);
            const age = Date.now() - cacheEntry.timestamp;
            
            if (age > cacheEntry.ttl) {
                console.log('🕒 Cache EXPIRED:', key, '(age:', Math.round(age/1000), 's)');
                this.remove(key);
                return null;
            }
            
            console.log('✅ Cache HIT:', key, '(age:', Math.round(age/1000), 's)');
            return cacheEntry.data;
        } catch (e) {
            console.warn('⚠️ Cache read failed:', e.message);
            return null;
        }
    },
    
    remove(key) {
        try {
            localStorage.removeItem(this.PREFIX + key);
            console.log('🗑️ Cache REMOVE:', key);
        } catch (e) {
            console.warn('⚠️ Cache remove failed:', e.message);
        }
    },
    
    clear() {
        try {
            const keys = Object.keys(localStorage);
            keys.forEach(key => {
                if (key.startsWith(this.PREFIX)) {
                    localStorage.removeItem(key);
                }
            });
            console.log('🧹 Cache CLEARED (all TOTP cache entries removed)');
        } catch (e) {
            console.warn('⚠️ Cache clear failed:', e.message);
        }
    },
    
    invalidate(key) {
        this.remove(key);
        console.log('♻️ Cache INVALIDATED:', key);
    },
    
    getStats() {
        try {
            const keys = Object.keys(localStorage).filter(k => k.startsWith(this.PREFIX));
            const stats = {
                totalEntries: keys.length,
                entries: {}
            };
            
            keys.forEach(key => {
                const cached = localStorage.getItem(key);
                if (cached) {
                    const entry = JSON.parse(cached);
                    const age = Date.now() - entry.timestamp;
                    const remaining = entry.ttl - age;
                    stats.entries[key.replace(this.PREFIX, '')] = {
                        age: Math.round(age / 1000),
                        remaining: Math.round(remaining / 1000),
                        size: new Blob([cached]).size
                    };
                }
            });
            
            return stats;
        } catch (e) {
            return { error: e.message };
        }
    }
};

// 🛡️ Global error handler to prevent white screen
window.addEventListener('error', function(event) {
    console.error('❌ Global error caught:', event.error);
    event.preventDefault();
    return true;
});

window.addEventListener('unhandledrejection', function(event) {
    console.error('❌ Unhandled promise rejection:', event.reason);
    event.preventDefault();
});

function getCookie(name){const value='; '+document.cookie;const parts=value.split('; '+name+'=');if(parts.length===2)return parts.pop().split(';').shift();return null}

// 🔗 Helper: redirect на login с обфускацией
function redirectToLogin() {
    let loginURL = '/login'; // Fallback
    
    if (window.urlObfuscationMap && window.urlObfuscationMap['/login']) {
        loginURL = window.urlObfuscationMap['/login'];
        console.log('🔗 Redirecting to obfuscated login:', loginURL);
    } else {
        console.log('🔗 Redirecting to standard login (no mapping)');
    }
    
    window.location.href = loginURL;
}

function logout(){CacheManager.clear();const formData=new FormData();makeEncryptedRequest('/logout',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{if(data.success){console.log('Logout successful, clearing cookies and redirecting...');document.cookie='session=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';setTimeout(()=>{window.location.replace(window.urlObfuscationMap&&window.urlObfuscationMap['/login']?window.urlObfuscationMap['/login']:'/login')},500)}else{showStatus('Logout failed',true)}}).catch(err=>{console.error('Logout error:',err);document.cookie='session=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';setTimeout(()=>{window.location.replace(window.urlObfuscationMap&&window.urlObfuscationMap['/login']?window.urlObfuscationMap['/login']:'/login')},500)})}
function showStatus(message,isError=false){const statusDiv=document.getElementById('status');statusDiv.textContent=message;statusDiv.className='status-message '+(isError?'status-err':'status-ok');statusDiv.style.display='block';setTimeout(()=>statusDiv.style.display='none',5000)}

let appInitialized = false;
function openTab(evt,tabName){
    if (!appInitialized) {
        showCopyNotification(tr('ui.loading') || 'Loading...');
        return;
    }
    var i,tabcontent,tablinks;
    
    if (typeof keysUpdateInterval !== 'undefined' && keysUpdateInterval) {
        clearInterval(keysUpdateInterval);
        keysUpdateInterval = null;
    }
    
    tabcontent=document.getElementsByClassName("tab-content");
    for(i=0;i<tabcontent.length;i++){tabcontent[i].style.display="none"}
    tablinks=document.getElementsByClassName("tab-link");
    for(i=0;i<tablinks.length;i++){tablinks[i].className=tablinks[i].className.replace(" active","")}
    document.getElementById(tabName).style.display="block";
    evt.currentTarget.className+=" active";
    if(tabName==='Display'){(async()=>{await fetchThemeSettings();await new Promise(r=>setTimeout(r,100));await fetchDisplaySettings()})()}
    else if(tabName==='Keys'){fetchKeys()}
    else if(tabName==='Passwords'){fetchPasswords()}
    else if(tabName==='Pin'){fetchPinSettings()}
    else if(tabName==='Settings'){
        // 🛡️ Загружаем настройки ПОСЛЕДОВАТЕЛЬНО с задержками
        async function loadAllSettings() {
            try {
                await fetchBleSettings();
                await new Promise(resolve => setTimeout(resolve, 150)); // 150ms задержка
                await fetchMdnsSettings();
                await new Promise(resolve => setTimeout(resolve, 150));
                await fetchStartupMode();
                await new Promise(resolve => setTimeout(resolve, 150));
                await loadBootMode();
                await new Promise(resolve => setTimeout(resolve, 150));
                await fetchDeviceSettings();
                await new Promise(resolve => setTimeout(resolve, 150));
                await fetchSessionDurationSettings();
                await new Promise(resolve => setTimeout(resolve, 150));
                await fetchThemeSettings();
                await new Promise(resolve => setTimeout(resolve, 150));
                await fetchDisplaySettings();
                await new Promise(resolve => setTimeout(resolve, 150));
                await fetchPinSettings();
                await new Promise(resolve => setTimeout(resolve, 150));
            } catch(err) {
                console.error('Error loading settings:', err);
            }
        }
        loadAllSettings();
    }
}


let keysUpdateInterval = null;

function fetchKeys(){
    // ⚡ CACHE: Проверяем кеш для списка ключей (только имена!)
    const cachedKeys = CacheManager.get('keys_list');
    if (cachedKeys) {
        console.log('⚡ Using cached keys list, updating TOTP codes from server...');
        keysData = cachedKeys;
        updateKeysTable(cachedKeys);
        if (keysUpdateInterval) clearInterval(keysUpdateInterval);
        keysUpdateInterval = setInterval(updateTOTPCodes, 1000);
        // Продолжаем запрос в фоне для обновления TOTP кодов
        // Return a resolved promise for the cache hit case
        return Promise.resolve();
    }
    
    return makeAuthenticatedRequest('/api/keys', {
        headers: {
            'X-User-Activity': 'true'  // Пользовательское действие
        }
    })
    .then(async response => {
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }
        
        const responseText = await response.text();
        
        // 🔐 Обработка зашифрованных TOTP данных
        let data;
        let originalData = JSON.parse(responseText);
        
        if (window.secureClient && window.secureClient.isReady) {
            data = await window.secureClient.decryptTOTPResponse(responseText);
            
            // Проверяем если расшифровка НЕ удалась
            if (originalData.type === "secure" && data && data[0] && data[0].name === "🔐 Encrypted Key 1") {
                showStatus('🔐 TOTP keys are encrypted - showing placeholders', false);
            } else if (originalData.type === "secure" && Array.isArray(data)) {
                showStatus('✅ TOTP keys decrypted successfully', false);
            }
        } else {
            data = originalData;
        }
        
        return data;
    })
    .then(data => {
        keysData = data;
        
        // ⚡ CACHE: Сохраняем только имена ключей (без TOTP кодов!)
        const keysForCache = data.map(key => ({
            name: key.name
            // НЕ кешируем code и timeLeft - они динамические!
        }));
        CacheManager.set('keys_list', keysForCache);
        
        updateKeysTable(data);
        if (keysUpdateInterval) clearInterval(keysUpdateInterval);
        keysUpdateInterval = setInterval(updateTOTPCodes, 1000);
    })
    .catch(err => {
        showStatus('Error fetching keys: ' + err.message, true);
    });
}

function updateKeysTable(data) {
    keysData = data;
    const tbody = document.querySelector('#keys-table tbody');
    tbody.innerHTML = '';
    
    if (!keysData || keysData.length === 0) {
        const row = tbody.insertRow();
        row.innerHTML = '<td colspan="6" style="text-align:center;color:#666;">No keys added yet</td>';
        return;
    }
    
    keysData.forEach((key, index) => {
        const row = tbody.insertRow();
        row.className = 'draggable-row';
        row.draggable = true;
        row.dataset.index = index;
        
        // Build timer and progress cells based on key type
        const isHOTP = key.t === 'H';
        const period = key.p || 30;
        const timerCell = isHOTP 
            ? `<td><span id="timer-${index}" style="color:#888;">🔄</span></td>`
            : `<td><span id="timer-${index}" style="font-weight:bold;color:#44ff44;">${key.timeLeft}s</span></td>`;
        const progressCell = isHOTP
            ? `<td><span id="progress-${index}"></span></td>`
            : `<td><progress id="progress-${index}" value="${key.timeLeft}" max="${period}"></progress></td>`;
        
        row.innerHTML = `
            <td><span class="drag-handle">::</span></td>
            <td>${key.name || key.n}${formatKeyMetadata(key)}</td>
            <td class="code" id="code-${index}" style="font-family:monospace;font-weight:bold;" onclick="copyTOTPCode(${index})" title="Click to copy TOTP code">${key.code}</td>
            ${timerCell}
            ${progressCell}
            <td>
                <button class="button-action button-qr user-activity" onclick="showQrOnDisplay(${index})" style="margin-right: 5px;" title="Show QR code on device">📱</button>
                ${isHOTP ? `<button class="button-action button-hotp user-activity" onclick="refreshHOTPCode(${index})" style="margin-right:5px;background:#4CAF50;" title="Generate next HOTP code">${tr('keys.btn.next')}</button>` : ''}
                <button class="button-delete user-activity" onclick="removeKey(${index})">${tr('keys.btn.remove')}</button>
            </td>
        `;
    });
    
    // Initialize drag and drop for keys table
    initializeDragAndDrop('keys-table', 'keys');
}

function updateTOTPCodes() {
    // 🔧 FIX: Если keysData пустой массив - НЕ делаем запросы
    if (keysData && keysData.length === 0) {
        return; // Нет ключей - не обновляем
    }
    
    // Проверяем нужно ли обновлять коды с сервера
    const currentTime = Math.floor(Date.now() / 1000);
    
    // Обновляем с сервера только если коды изменились или первый раз
    // Проверяем минимальный период среди всех TOTP ключей
    const totpKeys = keysData ? keysData.filter(k => k.t !== 'H') : [];
    const hasHOTPOnly = keysData && keysData.length > 0 && totpKeys.length === 0;
    const minPeriod = totpKeys.length > 0
        ? Math.min(...totpKeys.map(k => k.p || 30))
        : 0;
    const timeInPeriod = minPeriod > 0 ? currentTime % minPeriod : 999;
    
    const hasHOTPKeys = keysData && keysData.some(k => k.t === 'H');
    if (timeInPeriod <= 1 || !keysData || (hasHOTPOnly && currentTime % 5 === 0)) {
        // 🔐 ИСПРАВЛЕНИЕ: Используем ту же логику расшифровки что и в fetchKeys()
        makeAuthenticatedRequest('/api/keys')
        .then(async response => {
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }
            
            const responseText = await response.text();
            
            // 🔐 Обработка зашифрованных TOTP данных (как в fetchKeys)
            let data;
            let originalData = JSON.parse(responseText);
            
            if (window.secureClient && window.secureClient.isReady) {
                data = await window.secureClient.decryptTOTPResponse(responseText);
                
                // Проверяем если расшифровка НЕ удалась
                // Тихое обновление, логи не нужны
            } else {
                data = originalData;
            }
            
            return data;
        })
        .then(data => {
            // Preserve existing HOTP codes - don't update them from server
            if (keysData && keysData.length > 0) {
                data.forEach((key, index) => {
                    if (key.t === 'H' && keysData[index] && keysData[index].t === 'H') {
                        // If counter changed (device button pressed), use new code from server
                        // Otherwise keep existing code to avoid flicker
                        const serverCounter = key.c || 0;
                        const localCounter = keysData[index].c || 0;
                        if (serverCounter === localCounter) {
                            key.code = keysData[index].code;
                        }
                        // else: counter changed on device — use server's new code
                    }
                });
            }
            keysData = data;
            updateTOTPDisplay(data);
        })
        .catch(err => console.error('Error updating TOTP codes:', err));
    } else {
        // Локальное обновление без HTTP запроса - пересчитываем timeLeft
        if (keysData && keysData.length > 0) {
            const localData = keysData.map(key => {
                // Skip HOTP keys - they don't auto-update
                if (key.t === 'H') {
                    return key; // Return unchanged
                }
                
                // For TOTP keys, calculate timeLeft based on their period
                const period = key.p || 30;
                const timeInKeyPeriod = currentTime % period;
                return {
                    ...key,
                    timeLeft: period - timeInKeyPeriod
                };
            });
            updateTOTPDisplay(localData);
        }
    }
}

function updateTOTPDisplay(data) {
    data.forEach((key, index) => {
        const codeElement = document.getElementById(`code-${index}`);
        const progressElement = document.getElementById(`progress-${index}`);
        const timerElement = document.getElementById(`timer-${index}`);
        
        if (codeElement && progressElement && timerElement) {
            // Animate code changes with fade effect
            const newCode=key.code||'';
            if (codeElement.textContent !== newCode) {
                codeElement.style.transition = 'opacity 0.3s ease';
                codeElement.style.opacity = '0.3';
                setTimeout(() => {
                    codeElement.textContent = newCode;
                    codeElement.style.opacity = '1';
                }, 150);
            }
            
            // Handle HOTP differently - no timer, no progress bar
            if (key.t === 'H') {
                progressElement.style.display = 'none';
                timerElement.textContent = '🔄';
                timerElement.style.color = '#888';
                timerElement.style.fontWeight = 'normal';
                timerElement.style.animation = 'none';
            } else {
                // TOTP: Update progress bar and timer (use period from key data)
                progressElement.style.display = 'block';
                const period = key.p || 30;  // Default to 30 if not specified
                progressElement.value = key.timeLeft;
                progressElement.max = period;
                timerElement.textContent = key.timeLeft + 's';
                timerElement.title = '';
                
                // Color coding based on time remaining
                if (key.timeLeft <= 5) {
                    progressElement.style.filter = 'hue-rotate(0deg)';
                    timerElement.style.color = '#ff4444';
                    timerElement.style.fontWeight = 'bold';
                } else if (key.timeLeft <= 10) {
                    progressElement.style.filter = 'hue-rotate(40deg)';
                    timerElement.style.color = '#ff8800';
                    timerElement.style.fontWeight = 'bold';
                } else {
                    progressElement.style.filter = 'hue-rotate(120deg)';
                    timerElement.style.color = '#44ff44';
                    timerElement.style.fontWeight = 'bold';
                }
                
                // Add pulse animation when time is low
                if (key.timeLeft <= 5) {
                    timerElement.style.animation = 'pulse 1s infinite';
                } else {
                    timerElement.style.animation = 'none';
                }
            }
        }
    });
}
// === TOTP/HOTP Extended Support ===

// Toggle visibility of Period/Counter fields based on type
function updateKeyTypeFields() {
    const isTOTP = document.getElementById('type-totp').checked;
    document.getElementById('period-container').style.display = isTOTP ? 'block' : 'none';
    document.getElementById('counter-container').style.display = isTOTP ? 'none' : 'block';
}

// Base32 validation function
function validateBase32(secret) {
    const cleaned = secret.replace(/\s/g, '').toUpperCase();
    if (cleaned.length === 0) {
        return { valid: false, error: 'Secret cannot be empty' };
    }
    if (cleaned.length < 16) {
        return { valid: false, error: 'Secret too short (minimum 16 characters)' };
    }
    const base32Regex = /^[A-Z2-7]+=*$/;
    if (!base32Regex.test(cleaned)) {
        return { valid: false, error: 'Invalid Base32 format (use A-Z, 2-7 only)' };
    }
    const paddingIndex = cleaned.indexOf('=');
    if (paddingIndex !== -1 && paddingIndex !== cleaned.length - cleaned.split('=').length + 1) {
        return { valid: false, error: 'Invalid Base32 padding' };
    }
    return { valid: true, cleaned: cleaned };
}

// Format key metadata for display
function formatKeyMetadata(key) {
    const parts = [];
    if (key.t === 'H') {
        parts.push('HOTP');
    }
    if (key.a === '256') {
        parts.push('SHA256');
    } else if (key.a === '512') {
        parts.push('SHA512');
    }
    if (key.d === 8) {
        parts.push('8-digit');
    }
    if (key.t !== 'H' && key.p === 60) {
        parts.push('60s');
    }
    return parts.length > 0 ? ' <span style="font-size:0.75em; color:#b0b0b0;">(' + parts.join(' • ') + ')</span>' : '';
}

// Initialize type toggle listeners
document.addEventListener('DOMContentLoaded', function() {
    const typeTOTP = document.getElementById('type-totp');
    const typeHOTP = document.getElementById('type-hotp');
    const keySecret = document.getElementById('key-secret');
    
    if (typeTOTP) typeTOTP.addEventListener('change', updateKeyTypeFields);
    if (typeHOTP) typeHOTP.addEventListener('change', updateKeyTypeFields);
    
    // Real-time secret validation feedback
    if (keySecret) {
        keySecret.addEventListener('input', function(e) {
            const secret = e.target.value;
            const validation = validateBase32(secret);
            if (secret.length > 0 && !validation.valid) {
                e.target.style.borderColor = '#e74c3c';
                e.target.style.boxShadow = '0 0 0 3px rgba(231, 76, 60, 0.3)';
            } else if (validation.valid) {
                e.target.style.borderColor = '#27ae60';
                e.target.style.boxShadow = '0 0 0 3px rgba(39, 174, 96, 0.3)';
            } else {
                e.target.style.borderColor = '';
                e.target.style.boxShadow = '';
            }
        });
    }
});

document.getElementById('add-key-form').addEventListener('submit', function(e) {
    e.preventDefault();
    const name = document.getElementById('key-name').value.trim();
    const secret = document.getElementById('key-secret').value;
    
    // Validate Base32 secret
    const validation = validateBase32(secret);
    if (!validation.valid) {
        showStatus('❌ ' + validation.error, true);
        return;
    }
    
    // Collect form data
    const type = document.querySelector('input[name="key-type"]:checked').value;
    const algorithm = document.getElementById('key-algorithm').value;
    const digits = parseInt(document.querySelector('input[name="key-digits"]:checked').value);
    
    // Build compact JSON object
    const keyData = {
        n: name,
        s: validation.cleaned
    };
    
    // Always include metadata fields (server expects them)
    keyData.t = type;
    keyData.a = algorithm;
    keyData.d = digits;
    
    if (type === 'T') {
        const period = parseInt(document.querySelector('input[name="key-period"]:checked').value);
        keyData.p = period;
    } else {
        keyData.c = parseInt(document.getElementById('key-counter').value) || 0;
    }
    
    // Send as JSON
    makeAuthenticatedRequest('/api/add', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(keyData)
    })
    .then(response => response.json())
    .then(data => {
        if (data && data.status === 'error') {
            showStatus('❌ ' + (data.message || tr('keys.add.error')), true);
            return;
        }
        CacheManager.invalidate('keys_list');
        showStatus(tr('keys.add.success'));
        fetchKeys();
        this.reset();
        // Clear validation styling after form reset
        document.getElementById('key-secret').style.borderColor = '';
        document.getElementById('key-secret').style.boxShadow = '';
        updateKeyTypeFields();
    })
    .catch(err => showStatus('❌ Error: ' + err, true));
});

function removeKey(index){if(!confirm('Are you sure?'))return;const formData=new FormData();formData.append('index',index);makeAuthenticatedRequest('/api/remove',{method:'POST',body:new URLSearchParams(formData)}).then(data=>{CacheManager.invalidate('keys_list');showStatus(tr('keys.remove.success'));fetchKeys()}).catch(err=>showStatus('Error: '+err,true))};
function refreshHOTPCode(index) {
    const btn = event.target.closest('button');
    if (btn) {
        btn.disabled = true;
        btn.textContent = '⏳';
    }
    const formData = new FormData();
    formData.append('index', index);
    makeAuthenticatedRequest('/api/hotp/generate', {
        method: 'POST',
        body: new URLSearchParams(formData)
    }).then(async response => {
        if (!response.ok) throw new Error('HTTP ' + response.status);
        const responseText = await response.text();
        let data;
        if (window.secureClient && window.secureClient.isReady) {
            data = await window.secureClient.decryptTOTPResponse(responseText);
        } else {
            data = JSON.parse(responseText);
        }
        if (data && data.code) {
            if (keysData && keysData[index]) {
                keysData[index].code = data.code;
                keysData[index].c = data.counter;
                const codeEl = document.getElementById('code-' + index);
                if (codeEl) codeEl.textContent = data.code;
            }
            showStatus(tr('keys.hotp.generated'), false);
        }
        if (btn) {
            btn.disabled = false;
            btn.textContent = '🔄 Next';
        }
    }).catch(err => {
        showStatus('❌ Error: ' + err, true);
        if (btn) {
            btn.disabled = false;
            btn.textContent = '🔄 Next';
        }
    });
};
function showQrOnDisplay(keyIndex){if(!confirm('⚠️ Show QR code on ESP32 display?\\n\\nWARNING: The secret key will be visible on the device screen for 30 seconds.\\nAnyone near the device will be able to scan it!\\n\\nContinue?'))return;const formData=new FormData();formData.append('key_id',keyIndex);makeAuthenticatedRequest('/api/show_qr',{method:'POST',body:new URLSearchParams(formData)}).then(data=>{showStatus('✅ QR code displayed on device for 30 seconds',false)}).catch(err=>showStatus('❌ Failed to display QR code: '+err,true))};

// --- MODIFIED Import/Export Logic ---
let currentAction = null;
let selectedFile = null;

function showPasswordModal(action, file = null) {
    currentAction = action;
    selectedFile = file;
    const modal = document.getElementById('password-modal');
    const title = document.getElementById('modal-title');
    const description = document.getElementById('modal-description');
    
    if (action === 'export-passwords') {
        title.textContent = tr('passwords.export.title');
        description.textContent = tr('passwords.export.desc');
    } else if (action === 'import-passwords') {
        title.textContent = tr('passwords.import.title');
        description.textContent = tr('passwords.import.desc');
    } else if (action.startsWith('export')) {
        title.textContent = tr('keys.export.title');
        description.textContent = tr('keys.export.desc');
    } else {
        title.textContent = tr('keys.import.title');
        description.textContent = tr('keys.import.desc');
    }
    
    document.getElementById('modal-password').value = '';
    modal.style.display = 'block';
}

function closePasswordModal() {
    document.getElementById('password-modal').style.display = 'none';
    currentAction = null;
    selectedFile = null;
}

// Password Generator Functions
function openPasswordGeneratorModal() {
    const modal = document.getElementById('password-generator-modal');
    modal.style.display = 'block';
    // Reset slider to default value
    document.getElementById('password-length-slider').value = 14;
    document.getElementById('length-display').textContent = '14';
    generatePassword(); // Generate initial password
}

function closePasswordGeneratorModal() {
    document.getElementById('password-generator-modal').style.display = 'none';
}

function generatePassword() {
    const length = document.getElementById('password-length-slider').value;
    
    // Character sets for different types
    const lowercase = 'abcdefghijklmnopqrstuvwxyz';
    const uppercase = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
    const numbers = '0123456789';
    const specialChars = '!@#$%^&*()_+-=[]{}|;:,.<>?~`\'"/';
    
    // Ensure at least one character from each type for strong passwords
    let password = '';
    const allChars = lowercase + uppercase + numbers + specialChars;
    
    if (length >= 4) {
        // Add at least one character from each type for lengths >= 4
        password += getSecureRandomChar(lowercase);
        password += getSecureRandomChar(uppercase);
        password += getSecureRandomChar(numbers);
        password += getSecureRandomChar(specialChars);
        
        // Fill remaining length with random chars from all sets
        for (let i = 4; i < length; i++) {
            password += getSecureRandomChar(allChars);
        }
        
        // Shuffle the password to randomize positions
        password = shuffleString(password);
    } else {
        // For very short passwords, just use random chars
        for (let i = 0; i < length; i++) {
            password += getSecureRandomChar(allChars);
        }
    }
    
    document.getElementById('generated-password').value = password;
    updatePasswordStrength(password);
}

function getSecureRandomChar(charset) {
    const array = new Uint32Array(1);
    let randomValue;
    
    do {
        crypto.getRandomValues(array);
        randomValue = array[0];
    } while (randomValue >= (0x100000000 - (0x100000000 % charset.length)));
    
    return charset[randomValue % charset.length];
}

function shuffleString(str) {
    const arr = str.split('');
    // Multiple shuffle passes for better entropy
    for (let pass = 0; pass < 3; pass++) {
        for (let i = arr.length - 1; i > 0; i--) {
            const j = Math.floor(crypto.getRandomValues(new Uint32Array(1))[0] / (0x100000000 / (i + 1)));
            [arr[i], arr[j]] = [arr[j], arr[i]];
        }
    }
    return arr.join('');
}

function useGeneratedPassword() {
    const generatedPassword = document.getElementById('generated-password').value;
    document.getElementById('password-value').value = generatedPassword;
    closePasswordGeneratorModal();
}

// Password Edit Modal Functions
let currentEditIndex = -1;

function editPassword(index) {
    if (!passwordsData || !passwordsData[index]) {
        showStatus('Password not found!', true);
        return;
    }
    
    currentEditIndex = index;
    const formData = new FormData();
    formData.append('index', index);
    
    makeAuthenticatedRequest('/api/passwords/get', { method: 'POST', body: new URLSearchParams(formData) })
        .then(async response => {
            if (!response.ok) throw new Error('Failed to fetch password');
            
            const responseText = await response.text();
            
            // 🔐 Обработка зашифрованных паролей (аналогично fetchKeys)
            let data;
            let originalData = JSON.parse(responseText);
            
            if (window.secureClient && window.secureClient.isReady) {
                data = await window.secureClient.decryptTOTPResponse(responseText);
                
                // Проверяем если расшифровка НЕ удалась
                if (originalData.type === "secure" && (!data || !data.name)) {
                    console.warn('🔐 Password data is encrypted but decryption failed');
                    showStatus('🔐 Password is encrypted - decryption failed', true);
                    return;
                }
            } else {
                data = originalData;
            }
            
            document.getElementById('edit-password-name').value = data.name || '';
            document.getElementById('edit-password-value').value = data.password || '';
            updatePasswordStrengthForEdit(data.password || '');
            openPasswordEditModal();
        })
        .catch(err => {
            showStatus('Error loading password: ' + err.message, true);
        });
}

function openPasswordEditModal() {
    const modal = document.getElementById('password-edit-modal');
    modal.style.display = 'block';
}

function closePasswordEditModal() {
    const modal = document.getElementById('password-edit-modal');
    modal.style.display = 'none';
    currentEditIndex = -1;
    // Очищаем поля
    document.getElementById('edit-password-name').value = '';
    document.getElementById('edit-password-value').value = '';
    updatePasswordStrengthForEdit('');
}

function generatePasswordForEdit() {
    // Используем существующую логику генерации пароля
    const length = 14; // Стандартная длина
    
    const lowercase = 'abcdefghijklmnopqrstuvwxyz';
    const uppercase = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
    const numbers = '0123456789';
    const specialChars = '!@#$%^&*()_+-=[]{}|;:,.<>?~`\'"/';
    
    let password = '';
    const allChars = lowercase + uppercase + numbers + specialChars;
    
    // Добавляем по одному символу из каждого типа
    password += getSecureRandomChar(lowercase);
    password += getSecureRandomChar(uppercase);
    password += getSecureRandomChar(numbers);
    password += getSecureRandomChar(specialChars);
    
    // Заполняем оставшиеся символы
    for (let i = 4; i < length; i++) {
        password += getSecureRandomChar(allChars);
    }
    
    // Перемешиваем пароль
    password = shuffleString(password);
    
    document.getElementById('edit-password-value').value = password;
    updatePasswordStrengthForEdit(password);
}

function updatePasswordStrengthForEdit(password) {
    const container = document.querySelector('#password-edit-modal .password-strength-container');
    const text = document.getElementById('edit-strength-text');
    const fill = document.getElementById('edit-strength-fill');
    
    if (!container || !text || !fill) return;
    
    const { level, score } = calculatePasswordStrength(password);
    
    // Убираем все классы strength-*
    container.classList.remove('strength-weak', 'strength-medium', 'strength-strong', 'strength-encryption');
    
    // Добавляем новый класс
    container.classList.add(`strength-${level}`);
    
    // Устанавливаем ширину заливки
    fill.style.width = `${score}%`;
    
    // Update text in English
    const levelNames = {
        'weak': tr('passwords.strength.weak'),
        'medium': tr('passwords.strength.medium'),
        'strong': tr('passwords.strength.strong'),
        'encryption': tr('passwords.strength.encryption')
    };
    
    text.textContent = `${levelNames[level]} (${Math.round(score)}%)`;
}

function savePasswordEdit() {
    const name = document.getElementById('edit-password-name').value.trim();
    const password = document.getElementById('edit-password-value').value;
    
    if (!name || !password) {
        showStatus('Name and password cannot be empty!', true);
        return;
    }
    
    if (currentEditIndex < 0) {
        showStatus('Error: Invalid entry index!', true);
        return;
    }
    
    const formData = new FormData();
    formData.append('index', currentEditIndex);
    formData.append('name', name);
    formData.append('password', password);
    
    makeAuthenticatedRequest('/api/passwords/update', { method: 'POST', body: new URLSearchParams(formData) })
        .then(response => {
            if (response.ok) {
                CacheManager.invalidate('passwords_list'); // ♻️ Инвалидация кеша
                showStatus(tr('passwords.updated'));
                closePasswordEditModal();
                fetchPasswords(); // Refresh table
            } else {
                return response.text().then(text => {
                    throw new Error(text || 'Unknown error');
                });
            }
        })
        .catch(err => {
            showStatus('Error saving password: ' + err.message, true);
        });
}

function updatePasswordStrength(password) {
    const container = document.querySelector('.password-strength-container');
    const text = document.getElementById('strength-text');
    const fill = document.querySelector('.password-strength-fill');
    
    if (!container || !text || !fill) return;
    
    const { level, score } = calculatePasswordStrength(password);
    
    // Remove all existing strength classes
    container.classList.remove('strength-weak', 'strength-medium', 'strength-strong', 'strength-encryption');
    
    // Add new strength class for colors
    container.classList.add(`strength-${level}`);
    
    // Set the actual fill percentage dynamically
    fill.style.width = `${score}%`;
    
    // Update text with consistent names
    const levelNames = {
        'weak': tr('passwords.strength.weak'),
        'medium': tr('passwords.strength.medium'),
        'strong': tr('passwords.strength.strong'),
        'encryption': tr('passwords.strength.encryption')
    };
    
    text.textContent = `${levelNames[level]} (${Math.round(score)}%)`;
}

function calculatePasswordStrength(password) {
    if (!password || password.length === 0) {
        return { level: 'weak', score: 0 };
    }
    
    const len = password.length;
    
    // Count character types present
    const hasLower = /[a-z]/.test(password);
    const hasUpper = /[A-Z]/.test(password);
    const hasNumber = /[0-9]/.test(password);
    const hasSpecial = /[!@#$%^&*()_+\-=\[\]{}|;:,.<>?~`'"/]/.test(password);
    
    const typeCount = (hasLower ? 1 : 0) + (hasUpper ? 1 : 0) + (hasNumber ? 1 : 0) + (hasSpecial ? 1 : 0);
    
    // Calculate score with gentle progression starting from 1%
    let score = 0;
    
    // Base length score (starts very low, grows gradually)
    if (len === 1) {
        score = 1;
    } else if (len <= 4) {
        score = 1 + (len - 1) * 2; // 1, 3, 5, 7 for lengths 1-4
    } else if (len <= 8) {
        score = 7 + (len - 4) * 3; // 10, 13, 16, 19 for lengths 5-8
    } else if (len <= 12) {
        score = 19 + (len - 8) * 4; // 23, 27, 31, 35 for lengths 9-12
    } else if (len <= 20) {
        score = 35 + (len - 12) * 2; // 37, 39, 41, 43, 45, 47, 49, 51 for 13-20
    } else if (len <= 32) {
        score = 51 + (len - 20) * 1; // 52-63 for 21-32
    } else {
        score = 63 + Math.min(len - 32, 32) * 0.5; // 63-79 for 33+
    }
    
    // Character type bonus (smaller bonuses)
    if (typeCount === 1) {
        score += 0;
    } else if (typeCount === 2) {
        score += 5;
    } else if (typeCount === 3) {
        score += 10;
    } else if (typeCount === 4) {
        score += 15;
    }
    
    // Small bonus for good passwords only
    if (len >= 20 && typeCount === 4) {
        score += 5;
    }
    
    // Ensure score is in range 0-100
    score = Math.min(100, Math.max(0, score));
    
    // Determine level based on actual score ranges
    let level;
    if (score >= 75) {
        level = 'encryption';  // 75-100%: Excellent
    } else if (score >= 50) {
        level = 'strong';      // 50-74%: Good
    } else if (score >= 25) {
        level = 'medium';      // 25-49%: Fair
    } else {
        level = 'weak';        // 0-24%: Poor
    }
    
    return { level, score };
}

// Event listeners for password generator
document.addEventListener('DOMContentLoaded', function() {
    const lengthSlider = document.getElementById('password-length-slider');
    const lengthDisplay = document.getElementById('length-display');
    
    if (lengthSlider && lengthDisplay) {
        lengthSlider.addEventListener('input', function() {
            lengthDisplay.textContent = this.value;
            generatePassword();
        });
    }
    
    // Event listener for edit password field to update strength indicator
    const editPasswordField = document.getElementById('edit-password-value');
    if (editPasswordField) {
        editPasswordField.addEventListener('input', function() {
            updatePasswordStrengthForEdit(this.value);
        });
    }
});

document.getElementById('modal-submit-btn').addEventListener('click', () => {
    const password = document.getElementById('modal-password').value;
    if (!password) {
        showStatus('Password cannot be empty.', true);
        return;
    }

    if (currentAction === 'export-keys') handleExport('/api/export', password, 'encrypted_keys_backup.json');
    else if (currentAction === 'export-passwords') handleExport('/api/passwords/export', password, 'encrypted_passwords_backup.json');
    else if (currentAction === 'import-keys') handleImport('/api/import', password, selectedFile, fetchKeys);
    else if (currentAction === 'import-passwords') handleImport('/api/passwords/import', password, selectedFile, fetchPasswords);
    
    closePasswordModal();
});

function handleExport(url, password, filename) {
    const formData = new FormData();
    formData.append('password', password);

    makeAuthenticatedRequest(url, { method: 'POST', body: formData })
    .then(response => {

        if (!response.ok) {
            throw new Error(`Export failed with status ${response.status}`);
        }
        
        // ✅ FIX: Use .text() instead of .json() - export returns encrypted string, not JSON
        return response.text();
    })
    .then(rawContent => {

        // Extract fileContent from wrapper if present
        let fileContent = rawContent;
        try {
            // Try JSON parse first
            const parsed = JSON.parse(rawContent);
            if (parsed.fileContent) {
                fileContent = parsed.fileContent;
            }
        } catch(e) {
            // Fallback: extract fileContent via regex (handles malformed JSON)
            const match = rawContent.match(/"fileContent"\s*:\s*"([\s\S]*?)"\s*\}/);
            if (match) {
                fileContent = match[1];
            }
        }
        
        // Создаем файл с зашифрованным контентом
        const blob = new Blob([fileContent], { type: 'application/json' });
        const link = document.createElement('a');
        link.href = URL.createObjectURL(blob);
        link.download = filename;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        
        showStatus(tr('passwords.export.success'));
        console.log('✅ Export file downloaded:', filename);
    })
    .catch(err => {
        console.error('❌ Export failed:', err);
        showStatus('Export failed: ' + err.message, true);
    });
}

function handleImport(url, password, file, callbackOnSuccess) {
    if (!file) {
        showStatus('No file selected for import.', true);
        return;
    }
    const reader = new FileReader();
    reader.onload = function(event) {
        const fileContent = event.target.result;
        
        // 🔍 DEBUG: Логируем что прочитано из файла

        if (!fileContent || fileContent.length === 0) {
            console.error('❌ Import file is empty!');
            showStatus('Import file is empty!', true);
            return;
        }
        
        // Extract fileContent from wrapper if present (handles export wrapper)
        let importData = fileContent;
        try {
            const parsed = JSON.parse(importData);
            if (parsed.fileContent) {
                importData = parsed.fileContent;
                console.log('📂 Extracted fileContent from wrapper for import');
            }
        } catch(e) {
            const match = importData.match(/"fileContent"\s*:\s*"([\s\S]+?)(?="}\s*$)/);
            if (match) {
                importData = match[1];
                console.log('📂 Extracted fileContent via regex for import');
            }
        }
        
        const formData = new FormData();
        formData.append('password', password);
        formData.append('data', importData);
        

        makeAuthenticatedRequest(url, {
            method: 'POST',
            body: formData
        })
        .then(response => {

            if (!response.ok) {
                // ❌ Ошибка от сервера
                return response.text().then(errorText => {
                    console.error(`❌ Import failed with ${response.status}: ${errorText}`);
                    throw new Error(`Import failed: ${errorText || response.statusText}`);
                });
            }
            
            return response.json();
        })
        .then(data => {

            // Обработка зашифрованного ответа от ESP32
            let message = tr('passwords.import.success');
            if (typeof data === 'object' && data.message && data.success === false) {
                message = data.message;
            } else if (typeof data === 'string' && data.toLowerCase().includes('error')) {
                message = data;
            }
            showStatus(message);
            
            // Invalidate cache before calling callback to force fresh data fetch
            if (url.includes('/api/import')) {
                CacheManager.invalidate('keys_list');
            } else if (url.includes('/api/passwords/import')) {
                CacheManager.invalidate('passwords_list');
            }
            
            callbackOnSuccess();
        })
        .catch(err => showStatus('Import failed: ' + err.message, true));
    };
    reader.readAsText(file);
}

document.getElementById('export-keys-btn').addEventListener('click', (e) => { e.preventDefault(); showPasswordModal('export-keys'); });
document.getElementById('import-keys-btn').addEventListener('click', (e) => { e.preventDefault(); document.getElementById('import-file').click(); });
document.getElementById('import-file').addEventListener('change', (e) => { if(e.target.files.length > 0) showPasswordModal('import-keys', e.target.files[0]); });

document.getElementById('export-passwords-btn').addEventListener('click', (e) => { e.preventDefault(); showPasswordModal('export-passwords'); });
document.getElementById('import-passwords-btn').addEventListener('click', (e) => { e.preventDefault(); document.getElementById('import-passwords-file').click(); });
document.getElementById('import-passwords-file').addEventListener('change', (e) => { if(e.target.files.length > 0) showPasswordModal('import-passwords', e.target.files[0]); });
// --- END of MODIFIED Logic ---

function fetchPasswords(){
    // ⚡ CACHE: Проверяем кеш для списка паролей
    const cachedPasswords = CacheManager.get('passwords_list');
    if (cachedPasswords) {
        console.log('⚡ Using cached passwords list');
        passwordsData = cachedPasswords;
        const tbody = document.querySelector('#passwords-table tbody');
        tbody.innerHTML = '';
        cachedPasswords.forEach((password, index) => {
            const row = tbody.insertRow();
            row.className = 'draggable-row';
            row.draggable = true;
            row.dataset.index = index;
            row.innerHTML = '<td><span class="drag-handle">::</span></td><td>' + password.name + '</td><td><button class="button-copy user-activity" onclick="copyPassword(' + index + ')">' + tr('passwords.btn.copy') + '</button><button class="button-action user-activity" onclick="editPassword(' + index + ')">' + tr('passwords.btn.edit') + '</button><button class="button-delete user-activity" onclick="removePassword(' + index + ')">' + tr('passwords.btn.remove') + '</button></td>';
        });
        initializeDragAndDrop('passwords-table', 'passwords');
        return; // Используем кеш, не запрашиваем сервер
    }
    
    makeAuthenticatedRequest('/api/passwords', {
        headers: {
            'X-User-Activity': 'true'  // Пользовательское действие
        }
    })
    .then(async response => {
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }
        
        const responseText = await response.text();
        
        // 🔐 Обработка зашифрованных паролей (аналогично TOTP ключам)
        let data;
        let originalData = JSON.parse(responseText);
        
        if (window.secureClient && window.secureClient.isReady) {
            data = await window.secureClient.decryptTOTPResponse(responseText);
            
            // Проверяем если расшифровка НЕ удалась
            if (originalData.type === "secure" && data && data[0] && data[0].name === "🔐 Encrypted Key 1") {
                showStatus('🔐 Passwords are encrypted - showing placeholders', false);
            } else if (originalData.type === "secure" && Array.isArray(data)) {
                showStatus('✅ Passwords decrypted successfully', false);
            }
        } else {
            data = originalData;
        }
        
        // ⚡ MEMORY: Храним только имена (пароли не нужны в памяти)
        passwordsData = data.map(pwd => ({ name: pwd.name }));
        
        // ⚡ CACHE: Сохраняем только имена паролей (без самих паролей!)
        CacheManager.set('passwords_list', passwordsData);
        
        const tbody = document.querySelector('#passwords-table tbody');
        tbody.innerHTML = '';
        passwordsData.forEach((password, index) => {
            const row = tbody.insertRow();
            row.className = 'draggable-row';
            row.draggable = true;
            row.dataset.index = index;
            row.innerHTML = '<td><span class="drag-handle">::</span></td><td>' + password.name + '</td><td><button class="button-copy user-activity" onclick="copyPassword(' + index + ')">' + tr('passwords.btn.copy') + '</button><button class="button-action user-activity" onclick="editPassword(' + index + ')">' + tr('passwords.btn.edit') + '</button><button class="button-delete user-activity" onclick="removePassword(' + index + ')">' + tr('passwords.btn.remove') + '</button></td>';
        });
        initializeDragAndDrop('passwords-table', 'passwords');
    })
    .catch(err => {
        console.error('Error fetching passwords:', err);
        showStatus('Error fetching passwords.', true);
    });
}
document.getElementById('add-password-form').addEventListener('submit',function(e){e.preventDefault();const name=document.getElementById('password-name').value;const password=document.getElementById('password-value').value;const formData=new FormData();formData.append('name',name);formData.append('password',password);makeAuthenticatedRequest('/api/passwords/add',{method:'POST',body:formData}).then(data=>{CacheManager.invalidate('passwords_list');showStatus(tr('passwords.added'));fetchPasswords();this.reset()}).catch(err=>showStatus('Error: '+err,true))});
function removePassword(index){if(!confirm('Are you sure?'))return;const formData=new FormData();formData.append('index',index);makeAuthenticatedRequest('/api/passwords/delete',{method:'POST',body:formData}).then(data=>{CacheManager.invalidate('passwords_list');showStatus(tr('passwords.removed'));fetchPasswords()}).catch(err=>showStatus('Error: '+err,true))};

async function copyPassword(index) {
    const formData = new FormData();
    formData.append('index', index);
    
    try {
        const response = await makeAuthenticatedRequest('/api/passwords/get', { 
            method: 'POST', 
            body: new URLSearchParams(formData) 
        });
        
        if (!response.ok) throw new Error('Failed to fetch password');
        
        const responseText = await response.text();
        
        // 🔐 Обработка зашифрованных паролей
        let data;
        let originalData = JSON.parse(responseText);
        
        if (window.secureClient && window.secureClient.isReady) {
            data = await window.secureClient.decryptTOTPResponse(responseText);
            
            if (originalData.type === "secure" && (!data || !data.password)) {
                console.warn('🔐 Password is encrypted but decryption failed');
                showStatus('🔐 Failed to decrypt password', true);
                return;
            }
        } else {
            data = originalData;
        }
        
        if (data && data.password) {
            // Try modern Clipboard API first (HTTPS only)
            if (navigator.clipboard && navigator.clipboard.writeText) {
                await navigator.clipboard.writeText(data.password);
                showCopyNotification(tr('passwords.copied'));
            } else {
                // Fallback for HTTP or older browsers
                fallbackCopyPassword(data.password);
            }
        }
    } catch (err) {
        console.error('Failed to copy password:', err);
        showStatus('Error: ' + err.message, true);
    }
}

function fallbackCopyPassword(password) {
    const textArea = document.createElement('textarea');
    textArea.value = password;
    textArea.style.position = 'fixed';
    textArea.style.left = '-999999px';
    textArea.style.top = '-999999px';
    document.body.appendChild(textArea);
    textArea.focus();
    textArea.select();
    
    try {
        const successful = document.execCommand('copy');
        if (successful) {
            showCopyNotification(tr('passwords.copied'));
        } else {
            showStatus('Failed to copy password', true);
        }
    } catch (err) {
        console.error('Copy failed:', err);
        showStatus('Copy not supported in this browser', true);
    } finally {
        document.body.removeChild(textArea);
    }
}

function copyTOTPCode(index) {
    if (!keysData || !keysData[index]) {
        showStatus('TOTP code not found!', true);
        return;
    }
    
    const totpCode = keysData[index].code;
    const keyType = (keysData[index].t === 'H') ? 'hotp' : 'totp';
    
    // Try modern Clipboard API first
    if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(totpCode).then(() => {
            showCopyNotification(tr(keyType === 'hotp' ? 'keys.hotp_copied' : 'keys.copied'));
        }).catch(err => {
            console.warn('Clipboard API failed:', err);
            fallbackCopyTOTPCode(totpCode, keyType);
        });
    } else {
        // Fallback for older browsers
        fallbackCopyTOTPCode(totpCode, keyType);
    }
}

function fallbackCopyTOTPCode(totpCode, keyType) {
    const textArea = document.createElement('textarea');
    textArea.value = totpCode;
    textArea.style.position = 'fixed';
    textArea.style.left = '-999999px';
    textArea.style.top = '-999999px';
    document.body.appendChild(textArea);
    textArea.focus();
    textArea.select();
    
    try {
        const successful = document.execCommand('copy');
        if (successful) {
            showCopyNotification(tr(keyType === 'hotp' ? 'keys.hotp_copied' : 'keys.copied'));
        } else {
            showStatus('Failed to copy TOTP code', true);
        }
    } catch (err) {
        console.error('Copy failed:', err);
        showStatus('Copy not supported in this browser', true);
    } finally {
        document.body.removeChild(textArea);
    }
}

function showCopyNotification(message) {
    // Remove existing notification if any
    const existingNotification = document.querySelector('.copy-notification');
    if (existingNotification) {
        existingNotification.remove();
    }
    
    // Create new notification
    const notification = document.createElement('div');
    notification.className = 'copy-notification';
    notification.textContent = message;
    document.body.appendChild(notification);
    
    // Show notification with animation
    setTimeout(() => {
        notification.classList.add('show');
    }, 10);
    
    // Hide and remove notification after 2 seconds
    setTimeout(() => {
        notification.classList.remove('show');
        setTimeout(() => {
            if (notification.parentNode) {
                notification.parentNode.removeChild(notification);
            }
        }, 300);
    }, 2000);
}

async function fetchThemeSettings(){
    // ⚡ CACHE: Проверяем кеш для theme
    const cachedTheme = CacheManager.get('theme_settings');
    if (cachedTheme) {
        console.log('⚡ Using cached theme settings');
        if(cachedTheme.theme==='light'||cachedTheme.theme===1){document.getElementById('theme-light').checked=true}else{document.getElementById('theme-dark').checked=true}
        return;
    }
    
    try{
        const response=await makeEncryptedRequest('/api/theme');
        const data=await response.json();
        
        // ⚡ CACHE: Сохраняем theme в кеш
        CacheManager.set('theme_settings', data);
        
        if(data.theme==='light'||data.theme===1){document.getElementById('theme-light').checked=true}else{document.getElementById('theme-dark').checked=true}
    }catch(err){
        showStatus('Error fetching theme settings.',true)
    }
}
async function fetchDisplaySettings(){try{const response=await makeEncryptedRequest('/api/display_settings');const data=await response.json();document.getElementById('display-timeout').value=data.display_timeout;if(document.getElementById('auto-lock-timeout'))document.getElementById('auto-lock-timeout').value=data.auto_lock_timeout||0;const splashResponse=await makeEncryptedRequest('/api/splash/mode');if(splashResponse.ok){const splashData=await splashResponse.json();const selectElement=document.getElementById('splash-mode-select');if(selectElement)selectElement.value=splashData.mode}const clockResponse=await makeEncryptedRequest('/api/clock_settings');if(clockResponse.ok){const clockData=await clockResponse.json();const tzSelect=document.getElementById('timezone-select');if(tzSelect&&clockData.timezone)tzSelect.value=clockData.timezone}await loadRtcStatus()}catch(err){showStatus('Error fetching display settings.',true)}}
async function loadBootMode(){try{const response=await makeEncryptedRequest('/api/boot-mode');if(!response.ok)return;const data=await response.json();const selectEl=document.getElementById('boot-mode-select');if(selectEl&&data.boot_mode)selectEl.value=data.boot_mode}catch(err){console.warn('Boot mode load failed',err)}}
document.getElementById('save-boot-mode-btn').addEventListener('click',async function(){this.disabled=true;this.textContent=tr('settings.boot.saving');try{const mode=document.getElementById('boot-mode-select').value;const response=await makeEncryptedRequest('/api/boot-mode',{method:'POST',body:JSON.stringify({boot_mode:mode}),headers:{'Content-Type':'application/json'}});if(response.ok){const data=await response.json();showStatus(data.success?tr('settings.boot.saved'):'Error saving boot mode',!data.success)}else{showStatus('Error saving boot mode',true)}}catch(err){showStatus('Error: '+err.message,true)}finally{this.disabled=false;this.textContent=tr('settings.boot.btn')}});
document.getElementById('theme-selection-form').addEventListener('submit',function(e){e.preventDefault();const selectedTheme=document.querySelector('input[name="theme"]:checked').value;const formData=new FormData();formData.append('theme',selectedTheme);makeEncryptedRequest('/api/theme',{method:'POST',body:new URLSearchParams(formData)}).then(res=>res.json()).then(data=>{CacheManager.invalidate('theme_settings');if(data.success){showStatus(tr('display.theme.saved'))}else{showStatus(data.message||'Error applying theme',true)}}).catch(err=>showStatus('Error applying theme: '+err,true))});
(function(){const AL_ORDER=[0,300,900,1800,3600,14400];function enforceAutoLock(){const t=parseInt(document.getElementById('display-timeout').value);const alSel=document.getElementById('auto-lock-timeout');const al=parseInt(alSel.value);if(t>0&&al>0&&al<=t){const next=AL_ORDER.find(function(v){return v>t});alSel.value=next!==undefined?String(next):'0';}};document.getElementById('display-timeout').addEventListener('change',enforceAutoLock);document.getElementById('display-timeout-form').addEventListener('submit',function(e){e.preventDefault();const timeout=parseInt(document.getElementById('display-timeout').value);const autoLock=parseInt(document.getElementById('auto-lock-timeout').value);if(timeout>0&&autoLock>0&&autoLock<=timeout){showStatus(tr('display.lock.error.order'),true);return;}const formData=new FormData();formData.append('display_timeout',timeout);formData.append('auto_lock_timeout',autoLock);makeEncryptedRequest('/api/display_settings',{method:'POST',body:new URLSearchParams(formData)}).then(res=>res.json()).then(data=>{if(data.success){showStatus(tr('display.lock.saved'))}else{showStatus(data.message||'Error saving settings',true)}}).catch(err=>showStatus('Error saving display settings: '+err,true));});})();
document.getElementById('save-timezone-btn').addEventListener('click',async function(){const tzSelect=document.getElementById('timezone-select');if(!tzSelect||!tzSelect.value){showStatus(tr('display.clock.tz.select'),true);return}const formData=new FormData();formData.append('timezone',tzSelect.value);const response=await makeEncryptedRequest('/api/clock_settings',{method:'POST',body:new URLSearchParams(formData)});if(response.ok){const data=await response.json();showStatus(data.success?tr('display.clock.tz.saved'):(data.message||'Error saving timezone'),!data.success)}else{showStatus('Error saving timezone',true)}});
document.getElementById('save-splash-mode-btn').addEventListener('click',async function(){const selectElement=document.getElementById('splash-mode-select');if(!selectElement||!selectElement.value){showStatus(tr('display.splash.select'),true);return}const formData=new FormData();formData.append('mode',selectElement.value);try{const response=await makeEncryptedRequest('/api/splash/mode',{method:'POST',body:formData});if(response.ok){const data=await response.json();showStatus(data.success?tr('display.splash.saved'):'Error saving splash mode')}else{const text=await response.text();showStatus('Error: '+text,true)}}catch(err){showStatus('Error saving splash mode: '+err.message,true)}});
async function loadRtcStatus(){try{const response=await makeEncryptedRequest('/api/rtc');if(!response.ok)return;const data=await response.json();const enabledEl=document.getElementById('rtc-enabled');if(!enabledEl)return;enabledEl.checked=!!data.enabled;document.getElementById('rtc-sda').value=data.sda_pin||21;document.getElementById('rtc-scl').value=data.scl_pin||22;document.getElementById('rtc-pin-config').style.display=data.enabled?'block':'none';const badge=document.getElementById('rtc-status-badge');const timeDisplay=document.getElementById('rtc-time-display');const statusRow=document.getElementById('rtc-status-row');if(data.enabled){statusRow.style.display='flex';if(!data.available){badge.style.background='#e74c3c';badge.textContent=tr('display.rtc.status.notfound');timeDisplay.textContent=tr('display.rtc.status.check')}else{const rtcTs=data.rtc_time?Math.floor(new Date(data.rtc_time).getTime()/1000):0;const drift=Math.abs(rtcTs-Math.floor(Date.now()/1000));if(drift>3600){badge.style.background='#f39c12';badge.textContent=tr('display.rtc.status.drifted');timeDisplay.textContent=data.rtc_time||''}else{badge.style.background='#27ae60';badge.textContent=tr('display.rtc.status.ok');timeDisplay.textContent=data.rtc_time||''}}}else{statusRow.style.display='none'}document.getElementById('rtc-sync-hint').textContent=data.wifi_connected?tr('display.rtc.hint.ntp'):tr('display.rtc.hint.browser')}catch(err){console.warn('RTC status load failed',err)}}
document.getElementById('rtc-enabled').addEventListener('change',function(){document.getElementById('rtc-pin-config').style.display=this.checked?'block':'none'});
document.getElementById('rtc-sync-btn').addEventListener('click',async function(){this.disabled=true;this.textContent=tr('display.rtc.saving');try{const payload={enabled:document.getElementById('rtc-enabled').checked,sda_pin:parseInt(document.getElementById('rtc-sda').value)||21,scl_pin:parseInt(document.getElementById('rtc-scl').value)||22,timestamp:Math.floor(Date.now()/1000)};const response=await makeEncryptedRequest('/api/rtc',{method:'POST',body:JSON.stringify(payload),headers:{'Content-Type':'application/json'}});if(response.ok){const data=await response.json();showStatus(data.success!==false?tr('display.rtc.saved'):(data.message||tr('display.rtc.saved')))}else{showStatus('Error saving RTC settings',true)}}catch(err){showStatus('Error: '+err.message,true)}finally{await loadRtcStatus();this.disabled=false;this.textContent=tr('display.rtc.btn')}});

async function fetchPinSettings(){
    // ⚡ CACHE: Проверяем кеш для PIN настроек
    const cachedPin = CacheManager.get('pin_settings');
    if (cachedPin) {
        console.log('⚡ Using cached PIN settings');
        document.getElementById('pin-enabled-device').checked = cachedPin.enabledForDevice;
        
        // Загружаем Device BLE PIN status
        if (cachedPin.deviceBlePinEnabled !== undefined) {
            document.getElementById('device-ble-pin-enabled').checked = cachedPin.deviceBlePinEnabled;
            toggleDeviceBlePinFields(); // Показываем/скрываем поля
        }
        
        return;
    }
    
    try{
    const response = await makeEncryptedRequest('/api/pincode_settings', {
        headers: {
            'X-User-Activity': 'true'  // Пользовательское действие для PIN настроек
        }
    });
    
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }
        
        const responseText = await response.text();
        
        // 🔐 Обработка зашифрованных PIN настроек (по аналогии с fetchKeys)
        let data;
        let originalData = JSON.parse(responseText);
        
        if (window.secureClient && window.secureClient.isReady) {
            data = await window.secureClient.decryptTOTPResponse(responseText);
            
            // Проверяем если расшифровка НЕ удалась
            if (originalData.type === "secure" && data) {
                showStatus('✅ PIN settings decrypted successfully', false);
            }
        } else {
            data = originalData;
        }
        
        // ⚡ CACHE: Сохраняем PIN настройки в кеш
        CacheManager.set('pin_settings', data);
        
        console.log('📌 PIN Settings loaded:', {
            deviceBlePinEnabled: data.deviceBlePinEnabled,
            deviceBlePinConfigured: data.deviceBlePinConfigured,
            enabledForDevice: data.enabledForDevice
        });
        
        document.getElementById('pin-enabled-device').checked = data.enabledForDevice;
        
        // Загружаем Device BLE PIN status
        if (data.deviceBlePinEnabled !== undefined) {
            document.getElementById('device-ble-pin-enabled').checked = data.deviceBlePinEnabled;
            toggleDeviceBlePinFields(); // Показываем/скрываем поля
        }
        
    }catch(err){
        showStatus('Error fetching PIN settings: ' + err.message, true);
    }
}

// Toggle Device BLE PIN fields visibility
function toggleDeviceBlePinFields() {
    const enabled = document.getElementById('device-ble-pin-enabled').checked;
    const fields = document.getElementById('device-ble-pin-fields');
    fields.style.display = enabled ? 'block' : 'none';
    
    // Очищаем поля при выключении
    if (!enabled) {
        document.getElementById('device-ble-pin').value = '';
        document.getElementById('device-ble-pin-confirm').value = '';
        
        // Проверяем что это действие пользователя, а не программное изменение
        const cachedPin = CacheManager.get('pin_settings');
        if (cachedPin && cachedPin.deviceBlePinConfigured && cachedPin.deviceBlePinEnabled) {
            // Device BLE PIN был включен, пользователь выключает - отправляем запрос
            const formData = new FormData();
            formData.append('device_ble_pin_enabled', 'false');
            
            makeEncryptedRequest('/api/ble_pin_update', {method: 'POST', body: formData})
                .then(res => res.json())
                .then(data => {
                    if (data.success) {
                        showStatus(tr('pin.device.disabled'));
                        CacheManager.invalidate('pin_settings');
                        fetchPinSettings();
                    } else {
                        showStatus(data.message || 'Error disabling Device BLE PIN', true);
                        // Возвращаем switch в включенное состояние при ошибке
                        document.getElementById('device-ble-pin-enabled').checked = true;
                        fields.style.display = 'block';
                    }
                })
                .catch(err => {
                    showStatus('Error disabling Device BLE PIN: ' + err, true);
                    // Возвращаем switch в включенное состояние при ошибке
                    document.getElementById('device-ble-pin-enabled').checked = true;
                    fields.style.display = 'block';
                });
        }
    }
}

document.getElementById('pincode-settings-form').addEventListener('submit', async function(e) {
    e.preventDefault();
    
    const enabledForDevice = document.getElementById('pin-enabled-device').checked;
    
    // ✅ Предупреждение при изменении PIN защиты
    const cachedPin = CacheManager.get('pin_settings');
    const wasEnabled = cachedPin ? cachedPin.enabledForDevice : false;
    
    if (enabledForDevice !== wasEnabled) {
        let warningMessage = '';
        
        if (enabledForDevice) {
            // Включение PIN
            warningMessage = '⚠️ ENABLE PIN PROTECTION\n\n' +
                           '🔒 SECURITY:\n' +
                           '• Device key will be encrypted with PIN\n' +
                           '• PIN required on every boot\n' +
                           '• 5 failed attempts = device lock\n\n' +
                           '⚠️ WARNING:\n' +
                           '• If device key is NOT encrypted, ALL DATA will be DELETED (factory reset)\n' +
                           '• You will choose PIN length and create PIN on device screen after reboot\n' +
                           '• Export your data first!\n\n' +
                           'Continue?';
        } else {
            // Отключение PIN
            warningMessage = '⚠️ DISABLE PIN PROTECTION\n\n' +
                           '🔓 RISKS:\n' +
                           '• Device key will be stored UNENCRYPTED\n' +
                           '• Anyone with physical access can read your data\n' +
                           '• No PIN required on boot (less secure)\n\n' +
                           '⚠️ WARNING:\n' +
                           '• You will need to confirm on device screen\n' +
                           '• Re-enabling PIN will require factory reset\n\n' +
                           'Continue?';
        }
        
        if (!confirm(warningMessage)) {
            // Отменяем - возвращаем старое значение
            document.getElementById('pin-enabled-device').checked = wasEnabled;
            showStatus(tr('pin.cancelled'), false);
            return;
        }
    }
    
    // ✅ Отправляем настройки (без длины PIN - выбирается на устройстве)
    const jsonData = {
        enabledForDevice: enabledForDevice
    };
    
    try {
        const res = await makeEncryptedRequest('/api/pincode_settings', {
            method: 'POST',
            body: JSON.stringify(jsonData),
            headers: {'Content-Type': 'application/json'}
        });
        
        const data = await res.json();
        
        // Проверяем нужно ли подтверждение factory reset
        if (!data.success && data.message && (data.message.includes('FACTORY RESET') || data.message.includes('Factory reset') || data.message.includes('factory reset'))) {
            // Показываем подтверждение factory reset
            if (!confirm('⚠️ FACTORY RESET REQUIRED!\n\n' + 
                        'Enabling PIN will DELETE ALL DATA:\n' +
                        '• TOTP keys\n' +
                        '• Passwords\n' +
                        '• WiFi credentials\n\n' +
                        'Export your data first!\n\n' +
                        'After reboot:\n' +
                        '1. Choose PIN length (4-10 digits)\n' +
                        '2. Create PIN on device screen\n\n' +
                        'Continue?')) {
                document.getElementById('pin-enabled-device').checked = wasEnabled;
                showStatus(tr('pin.reset.cancelled'), false);
                return;
            }
            
            // Подтверждаем factory reset
            jsonData.factory_reset_confirmed = true;
            
            const confirmRes = await makeEncryptedRequest('/api/pincode_settings', {
                method: 'POST',
                body: JSON.stringify(jsonData),
                headers: {'Content-Type': 'application/json'}
            });
            
            const confirmData = await confirmRes.json();
            
            CacheManager.invalidate('pin_settings');
            
            if (confirmData.success) {
                showStatus(confirmData.message);
                if (confirmData.requiresReboot) {
                    showStatus(tr('pin.rebooting'), false);
                }
            } else {
                showStatus(confirmData.message || 'Error updating PIN settings', true);
            }
            
            return;
        }
        
        // Обычный ответ
        CacheManager.invalidate('pin_settings');
        
        if (data.success) {
            showStatus(data.message);
            if (data.requiresReboot) {
                showStatus(tr('pin.rebooting'), false);
            }
        } else {
            showStatus(data.message || 'Error updating PIN settings', true);
        }
    } catch (err) {
        showStatus('Error updating PIN settings: ' + err, true);
    }
});

// BLE Bonding PIN Management
document.getElementById('ble-bonding-pin-form').addEventListener('submit',function(e){
    e.preventDefault();
    const blePin=document.getElementById('ble-bonding-pin').value;
    const blePinConfirm=document.getElementById('ble-bonding-pin-confirm').value;
    if(blePin.length!==6||!/^\d{6}$/.test(blePin)){
        showStatus(tr('pin.ble.error.digits'),true);
        return;
    }
    if(blePin!==blePinConfirm){
        showStatus(tr('pin.ble.error.match'),true);
        return;
    }
    const formData=new FormData();
    formData.append('ble_bonding_pin',blePin);
    makeEncryptedRequest('/api/ble_pin_update',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{
        if(data.success){
            showStatus(tr('pin.ble.saved'));
            document.getElementById('ble-bonding-pin').value='';
            document.getElementById('ble-bonding-pin-confirm').value='';
        }else{
            showStatus(data.message||'Error updating BLE Bonding PIN',true);
        }
    }).catch(err=>showStatus('Error updating BLE Bonding PIN: '+err,true));
});

// Device BLE PIN Management
document.getElementById('device-ble-pin-form').addEventListener('submit',function(e){
    e.preventDefault();
    const enabled=document.getElementById('device-ble-pin-enabled').checked;
    const deviceBlePin=document.getElementById('device-ble-pin').value;
    const deviceBlePinConfirm=document.getElementById('device-ble-pin-confirm').value;
    
    if(!enabled){
        showStatus(tr('pin.device.error.enable'),true);
        return;
    }
    
    if(deviceBlePin.length!==6||!/^\d{6}$/.test(deviceBlePin)){
        showStatus(tr('pin.device.error.digits'),true);
        return;
    }
    if(deviceBlePin!==deviceBlePinConfirm){
        showStatus(tr('pin.device.error.match'),true);
        return;
    }
    
    const formData=new FormData();
    formData.append('device_ble_pin_enabled',enabled);
    formData.append('device_ble_pin',deviceBlePin);
    makeEncryptedRequest('/api/ble_pin_update',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{
        if(data.success){
            showStatus(tr('pin.device.saved'));
            document.getElementById('device-ble-pin').value='';
            document.getElementById('device-ble-pin-confirm').value='';
            
            // Обновляем кеш и перезагружаем настройки
            CacheManager.invalidate('pin_settings');
            fetchPinSettings();
        }else{
            showStatus(data.message||'Error updating Device BLE PIN',true);
        }
    }).catch(err=>showStatus('Error updating Device BLE PIN: '+err,true));
});

// Clear BLE Clients Management (🔐 Зашифровано)
document.getElementById('clear-ble-clients-btn').addEventListener('click',function(){if(!confirm('Are you sure you want to clear all BLE client connections? This will remove all paired devices and they will need to pair again.')){return}const formData=new FormData();makeEncryptedRequest('/api/clear_ble_clients',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{if(data.success){showStatus(tr('settings.system.ble.cleared'))}else{showStatus(data.message||'Failed to clear BLE clients',true)}}).catch(err=>showStatus('Error clearing BLE clients: '+err,true))});


async function fetchStartupMode(){try{const response=await makeEncryptedRequest('/api/startup_mode');const data=await response.json();document.getElementById('startup-mode').value=data.mode}catch(err){showStatus('Error fetching startup mode.',true)}}
async function fetchDeviceSettings(){try{const response=await makeEncryptedRequest('/api/settings');const data=await response.json();document.getElementById('web-server-timeout').value=data.web_server_timeout;if(data.admin_login){document.getElementById('current-admin-login').textContent=data.admin_login}}catch(err){showStatus('Error fetching device settings.',true)}}
document.getElementById('startup-mode-form').addEventListener('submit',function(e){e.preventDefault();const mode=document.getElementById('startup-mode').value;const formData=new FormData();formData.append('mode',mode);makeEncryptedRequest('/api/startup_mode',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{if(data.success){showStatus(tr('settings.startup.saved'))}else{showStatus(data.message,true)}}).catch(err=>showStatus('Error saving startup mode: '+err,true))});
document.getElementById('web-server-settings-form').addEventListener('submit',function(e){e.preventDefault();const timeout=document.getElementById('web-server-timeout').value;if(!confirm('Changing the web server timeout requires a device restart. Do you want to continue?')){return;}showStatus(tr('settings.webserver.saving'),false);const formData=new FormData();formData.append('web_server_timeout',timeout);makeEncryptedRequest('/api/settings',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{if(data.success){showStatus(tr('settings.webserver.saving'),false);}else{showStatus(data.message,true);}}).catch(err=>showStatus('Error saving settings: '+err,true))});
// Password validation for change password form
const passwordCriteria = {
    length: { el: document.getElementById('pwd-length'), regex: /.{8,}/ },
    uppercase: { el: document.getElementById('pwd-uppercase'), regex: /[A-Z]/ },
    lowercase: { el: document.getElementById('pwd-lowercase'), regex: /[a-z]/ },
    number: { el: document.getElementById('pwd-number'), regex: /[0-9]/ },
    special: { el: document.getElementById('pwd-special'), regex: /[!@#$%]/ }
};

function validateNewPassword() {
    const password = document.getElementById('new-password').value;
    let allValid = true;
    for (const key in passwordCriteria) {
        const isValid = passwordCriteria[key].regex.test(password);
        passwordCriteria[key].el.classList.toggle('valid', isValid);
        if (!isValid) allValid = false;
    }
    return allValid;
}

function validatePasswordConfirmation() {
    const password = document.getElementById('new-password').value;
    const confirmPassword = document.getElementById('confirm-password').value;
    const confirmMessage = document.getElementById('password-confirm-message');
    
    if (confirmPassword.length === 0) {
        confirmMessage.textContent = '';
        return false;
    }
    if (password === confirmPassword) {
        confirmMessage.textContent = tr('settings.pwd.match');
        confirmMessage.className = 'password-match';
        return true;
    } else {
        confirmMessage.textContent = 'Passwords do not match.';
        confirmMessage.className = 'password-no-match';
        return false;
    }
}

function checkChangePasswordFormValidity() {
    const isPasswordStrong = validateNewPassword();
    const doPasswordsMatch = validatePasswordConfirmation();
    
    const isFormValid = currentPasswordType === 'web' ? 
        (isPasswordStrong && doPasswordsMatch) :
        (document.getElementById('new-password').value.length >= 8 && doPasswordsMatch);
    
    document.getElementById('change-password-btn').disabled = !isFormValid;
}

// Password visibility toggle function
function togglePasswordVisibility(inputId, toggleElement) {
    const passwordInput = document.getElementById(inputId);
    if (passwordInput.type === 'password') {
        passwordInput.type = 'text';
        toggleElement.textContent = 'X';
    } else {
        passwordInput.type = 'password';
        toggleElement.textContent = 'O';
    }
}

// Toggle password visibility in modal
function toggleModalPassword() {
    const input = document.getElementById('modal-password');
    input.type = input.type === 'password' ? 'text' : 'password';
}

// Password Type Toggle Functionality
let currentPasswordType = 'web'; // 'web' or 'wifi'

function switchPasswordType(type) {
    currentPasswordType = type;
    try { if(!i18n) return; } catch(e) { return; }
    const webToggle = document.getElementById('web-password-toggle');
    const wifiToggle = document.getElementById('wifi-password-toggle');
    const formTitle = document.getElementById('password-form-title-text');
    const titleIcon = document.querySelector('.title-icon');
    const description = document.getElementById('password-type-description');
    const newLabel = document.getElementById('new-password-label');
    const confirmLabel = document.getElementById('confirm-password-label');
    const criteriaList = document.querySelector('.password-criteria');
    const submitBtn = document.getElementById('change-password-btn');
    
    // Reset toggles
    webToggle.classList.remove('active', 'web-active');
    wifiToggle.classList.remove('active', 'wifi-active');
    
    if (type === 'web') {
        webToggle.classList.add('active', 'web-active');
        formTitle.textContent = tr('settings.pwd.web.form.title');
        titleIcon.textContent = '🔒';
        description.textContent = tr('settings.pwd.web.desc');
        newLabel.textContent = tr('settings.pwd.web.new.label');
        confirmLabel.textContent = tr('settings.pwd.web.confirm.label');
        criteriaList.style.display = 'block';
        submitBtn.textContent = tr('settings.pwd.web.btn');
    } else {
        wifiToggle.classList.add('active', 'wifi-active');
        formTitle.textContent = tr('settings.pwd.wifi.form.title');
        titleIcon.textContent = '📶';
        description.textContent = tr('settings.pwd.wifi.desc');
        newLabel.textContent = tr('settings.pwd.wifi.new.label');
        confirmLabel.textContent = tr('settings.pwd.wifi.confirm.label');
        criteriaList.style.display = 'none'; // WiFi password has different requirements
        submitBtn.textContent = tr('settings.pwd.wifi.btn');
    }
    
    // Clear form
    document.getElementById('change-password-form').reset();
    checkChangePasswordFormValidity();
}

// Event listeners for toggles
document.getElementById('web-password-toggle').addEventListener('click', () => {
    console.log('🔄 Switching to WEB password mode');
    switchPasswordType('web');
});
document.getElementById('wifi-password-toggle').addEventListener('click', () => {
    console.log('🔄 Switching to WIFI password mode');
    switchPasswordType('wifi');
});

// Initialize with web password type
console.log('🔧 Initializing password form with WEB mode');
switchPasswordType('web');

// Add event listeners for password validation
document.getElementById('new-password').addEventListener('input', checkChangePasswordFormValidity);
document.getElementById('confirm-password').addEventListener('input', checkChangePasswordFormValidity);

document.getElementById('change-password-form').addEventListener('submit',function(e){
    e.preventDefault();
    const newPass=document.getElementById('new-password').value;
    const confirmPass=document.getElementById('confirm-password').value;
    
    if(newPass!==confirmPass){
        showStatus(tr('settings.pwd.err.match'),true);
        return;
    }
    
    // Validate based on password type
    if(currentPasswordType === 'web' && !validateNewPassword()){
        showStatus(tr('settings.pwd.err.requirements'),true);
        return;
    }
    
    if(currentPasswordType === 'wifi' && newPass.length < 8){
        showStatus(tr('settings.pwd.err.wifi.short'),true);
        return;
    }
    
    // Create FormData object
    const formData=new FormData();
    formData.append('password',newPass);
    
    const endpoint = currentPasswordType === 'web' ? '/api/change_password' : '/api/change_ap_password';
    
    if(currentPasswordType === 'wifi') {
        if(!confirm('Changing the WiFi AP password requires a device restart. You will need to reconnect with the new password. Continue?')) {
            return;
        }
    }
    
    console.log('🚀 Submitting password change:', {
        type: currentPasswordType,
        endpoint: endpoint,
        passwordLength: newPass.length
    });
    
    makeEncryptedRequest(endpoint,{method:'POST',body:formData})
        .then(res=>res.text().then(text=>{
            let message = text;
            try {
                const json = JSON.parse(text);
                message = json.message || json.error || text;
            } catch(e) {}
            if(res.ok) {
                showStatus(currentPasswordType==='web'?tr('settings.pwd.web.saved'):tr('settings.pwd.wifi.saved'));
                document.getElementById('change-password-form').reset();
                document.getElementById('password-confirm-message').textContent = '';
                checkChangePasswordFormValidity();
                try {
                    const json = JSON.parse(text);
                    if(json.restart) {
                        setTimeout(() => {
                            showStatus(tr('settings.pwd.wifi.restarted'), false);
                        }, 4000);
                    }
                } catch(e) {}
            } else {
                showStatus(message, true);
            }
        }));
});

document.getElementById('wifi-scan-btn').addEventListener('click', async function() {
    if (this.dataset.scanning === 'true') return;
    this.dataset.scanning = 'true';
    const btn = this;
    const dropdown = document.getElementById('wifi-scan-dropdown');
    btn.disabled = true;
    btn.textContent = tr('settings.scan.scanning');
    dropdown.style.display = 'none';
    dropdown.innerHTML = '';
    
    // Trigger scan then poll until results ready
    await fetch('/scan').catch(() => {});
    let networks = [];
    try {
        for (let attempt = 0; attempt < 10; attempt++) {
            await new Promise(r => setTimeout(r, 1500));
            const resp = await fetch('/scan');
            if (resp.status === 202) continue; // still scanning
            networks = await resp.json();
            if (networks.length > 0) break;
            if (attempt >= 3) break; // give up after 4 empty results
        }
        
        if (!networks || networks.length === 0) {
            dropdown.innerHTML = '<div style="padding:10px;opacity:0.6;font-size:12px;">No networks found</div>';
        } else {
            networks.sort((a, b) => b.rssi - a.rssi);
            networks.forEach(net => {
                const item = document.createElement('div');
                const bars = net.rssi > -50 ? '▂▄▆█' :
                             net.rssi > -65 ? '▂▄▆_' :
                             net.rssi > -75 ? '▂▄__' : '▂___';
                item.textContent = bars + '  ' + net.ssid;
                item.style.cssText = 'padding:8px 12px;cursor:pointer;font-size:13px;' +
                                     'border-bottom:1px solid rgba(255,255,255,0.06);';
                item.addEventListener('mouseenter', () => 
                    item.style.background = 'rgba(255,255,255,0.08)');
                item.addEventListener('mouseleave', () => 
                    item.style.background = '');
                item.addEventListener('click', () => {
                    document.getElementById('wifi-ssid').value = net.ssid;
                    dropdown.style.display = 'none';
                    document.getElementById('wifi-password').focus();
                });
                dropdown.appendChild(item);
            });
        }
        dropdown.style.display = 'block';
    } catch(e) {
        dropdown.innerHTML = '<div style="padding:10px;opacity:0.6;font-size:12px;">' + tr('settings.scan.failed') + '</div>';
        dropdown.style.display = 'block';
    }
    
    btn.textContent = '⏳ Reconnecting...';
    await new Promise(r => setTimeout(r, 4000));
    btn.disabled = false;
    btn.textContent = tr('settings.scan.btn');
    btn.dataset.scanning = 'false';
});

// Close dropdown on outside click// Close dropdown on outside click
document.addEventListener('click', function(e) {
    if (!e.target.closest('#wifi-scan-dropdown') && 
        e.target.id !== 'wifi-scan-btn' && 
        e.target.id !== 'wifi-ssid') {
        const dd = document.getElementById('wifi-scan-dropdown');
        if (dd) dd.style.display = 'none';
    }
});

document.getElementById('save-wifi-credentials-btn').addEventListener('click', async function() {
    const ssid = document.getElementById('wifi-ssid').value.trim();
    const password = document.getElementById('wifi-password').value;
    const confirm = document.getElementById('wifi-password-confirm').value;
    const statusEl = document.getElementById('wifi-credentials-status');
    
    if (!ssid) {
        statusEl.textContent = tr('settings.wifi.err.ssid');
        statusEl.className = 'status-message error';
        statusEl.style.display = 'block';
        return;
    }
    if (password !== confirm) {
        statusEl.textContent = tr('settings.wifi.err.match');
        statusEl.className = 'status-message error';
        statusEl.style.display = 'block';
        return;
    }
    if (password.length > 0 && password.length < 8) {
        statusEl.textContent = tr('settings.wifi.err.short');
        statusEl.className = 'status-message error';
        statusEl.style.display = 'block';
        return;
    }
    
    try {
        const response = await makeEncryptedRequest('/api/wifi_credentials', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ ssid, password, confirm_password: confirm })
        });
        const data = await response.json();
        statusEl.textContent = data.success ? tr('settings.wifi.saved') : (data.message || 'Error');
        statusEl.className = 'status-message ' + (data.success ? 'success' : 'error');
        statusEl.style.display = 'block';
        if (data.success) {
            document.getElementById('wifi-ssid').value = '';
            document.getElementById('wifi-password').value = '';
            document.getElementById('wifi-password-confirm').value = '';
        }
    } catch (e) {
        statusEl.textContent = tr('settings.wifi.err.failed');
        statusEl.className = 'status-message error';
        statusEl.style.display = 'block';
    }
});

// Custom splash upload/delete handlers removed - feature disabled for security

document.getElementById('reboot-btn').addEventListener('click',()=>{if(!confirm('Are you sure you want to reboot?'))return;const formData=new FormData();makeEncryptedRequest('/api/reboot',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{if(data.success){showStatus(tr('settings.system.rebooting'))}else{showStatus('Reboot failed',true)}}).catch(()=>showStatus(tr('settings.system.rebooting')))});
document.getElementById('reboot-with-web-btn').addEventListener('click',()=>{if(!confirm('Reboot and auto-start web server on next boot?'))return;const formData=new FormData();makeEncryptedRequest('/api/reboot_with_web',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{if(data.success){showStatus(tr('settings.system.rebooting.web'))}else{showStatus('Reboot failed',true)}}).catch(()=>showStatus(tr('settings.system.rebooting.web')))});

async function fetchBleSettings(){try{const response=await makeEncryptedRequest('/api/ble_settings');const data=await response.json();document.getElementById('ble-device-name').value=data.device_name}catch(err){showStatus('Error fetching BLE settings.',true)}}
document.getElementById('ble-settings-form').addEventListener('submit',function(e){e.preventDefault();const deviceName=document.getElementById('ble-device-name').value;const formData=new FormData();formData.append('device_name',deviceName);makeEncryptedRequest('/api/ble_settings',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{if(data.success){showStatus(tr('settings.ble.saved'));fetchBleSettings()}else{showStatus(data.message,true)}}).catch(err=>showStatus('Error saving BLE settings: '+err,true))});

async function fetchMdnsSettings(){try{const response=await makeEncryptedRequest('/api/mdns_settings');const data=await response.json();document.getElementById('mdns-hostname').value=data.hostname}catch(err){showStatus('Error fetching mDNS settings.',true)}}
document.getElementById('mdns-settings-form').addEventListener('submit',function(e){e.preventDefault();const hostname=document.getElementById('mdns-hostname').value;const formData=new FormData();formData.append('hostname',hostname);makeEncryptedRequest('/api/mdns_settings',{method:'POST',body:formData}).then(res=>res.json()).then(data=>{if(data.success){showStatus(tr('settings.mdns.saved'));fetchMdnsSettings()}else{showStatus(data.message,true)}}).catch(err=>showStatus('Error saving mDNS settings: '+err,true))});


let keysData = [];
let passwordsData = [];
let csrfToken = '';

// CSRF Token Management
async function fetchCsrfToken() {
    try {
        const response = await fetch('/api/csrf_token');
        if (response.ok) {
            const data = await response.json();
            if (data.csrf_token && data.csrf_token.length > 0) {
                csrfToken = data.csrf_token;
                return true;
            } else {
                // Empty or missing CSRF token indicates invalid session
                console.log('Invalid session detected (empty CSRF token), redirecting to login...');
                CacheManager.clear(); // 🧹 Очистка кеша при истечении сессии
                redirectToLogin();
                return false;
            }
        } else if (response.status === 401 || response.status === 403) {
            // Session expired or forbidden - redirect to login
            console.log('Session expired or forbidden, redirecting to login...');
            CacheManager.clear(); // 🧹 Очистка кеша при истечении сессии
            redirectToLogin();
            return false;
        } else {
            console.error('Failed to fetch CSRF token, status:', response.status);
            // For other errors, also redirect to login as fallback
            console.log('Authentication error, redirecting to login...');
            CacheManager.clear(); // 🧹 Очистка кеша при ошибке аутентификации
            redirectToLogin();
            return false;
        }
    } catch (err) {
        console.error('Error fetching CSRF token:', err);
        // On network errors, try to redirect to login
        console.log('Network error during token fetch, redirecting to login...');
        CacheManager.clear(); // 🧹 Очистка кеша при сетевой ошибке
        redirectToLogin();
        return false;
    }
}

// ===== CRYPTO-JS LIBRARY (МИНИМАЛЬНАЯ AES-GCM РЕАЛИЗАЦИЯ) =====
// Настоящая AES-GCM библиотека (упрощенная версия crypto-js)
window.CryptoJS = {
    // AES S-box и другие константы (упрощенные)
    _sbox: new Uint8Array([
        0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
        0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0
        // ... (полная таблица 256 байт слишком большая для демонстрации)
    ]),
    
    // Простая AES-GCM расшифровка (базовая реализация)
    AES: {
        decrypt: function(ciphertext, key, options) {
            // ВАЖНО: Это упрощенная реализация для демонстрации
            // В продакшене нужна полная AES-GCM библиотека
            
            const keyBytes = CryptoJS.enc.Hex.parse(key);
            const dataBytes = CryptoJS.enc.Hex.parse(ciphertext.ciphertext);
            const ivBytes = CryptoJS.enc.Hex.parse(options.iv);
            
            // Простая XOR расшифровка как fallback
            const result = new Uint8Array(dataBytes.sigBytes);
            for (let i = 0; i < result.length; i++) {
                result[i] = dataBytes.words[Math.floor(i/4)] >> (24 - (i % 4) * 8) & 0xff;
                result[i] ^= keyBytes.words[i % keyBytes.sigBytes] >> (24 - (i % 4) * 8) & 0xff;
                result[i] ^= ivBytes.words[i % ivBytes.sigBytes] >> (24 - (i % 4) * 8) & 0xff;
            }
            
            return CryptoJS.enc.Utf8.stringify({words: Array.from(result), sigBytes: result.length});
        }
    },
    
    // Утилиты для работы с форматами
    enc: {
        Hex: {
            parse: function(hexStr) {
                const bytes = [];
                for (let i = 0; i < hexStr.length; i += 2) {
                    bytes.push(parseInt(hexStr.substr(i, 2), 16));
                }
                const words = [];
                for (let i = 0; i < bytes.length; i += 4) {
                    words.push((bytes[i] << 24) | (bytes[i+1] << 16) | (bytes[i+2] << 8) | bytes[i+3]);
                }
                return {words: words, sigBytes: bytes.length};
            }
        },
        Utf8: {
            stringify: function(wordArray) {
                const bytes = [];
                for (let i = 0; i < wordArray.sigBytes; i++) {
                    bytes.push(wordArray.words[Math.floor(i/4)] >> (24 - (i % 4) * 8) & 0xff);
                }
                return new TextDecoder().decode(new Uint8Array(bytes));
            }
        }
    },
    
    mode: {
        GCM: {} // Placeholder для GCM режима
    }
};

// ===== SECURE CLIENT CLASS =====
/**
 * SecureClient - Рабочий JavaScript клиент для ESP32 шифрования
 * Использует ПРОВЕРЕННЫЙ ECDH ключ из page_test_encryption.h
 */
class SecureClient {
    constructor() {
        this.sessionId = null;
        this.isReady = false;
        this.logs = [];
        this.requestCounter = 1; // Счетчик для защиты от replay атак
        
        // Method Tunneling поддержка
        this.methodTunnelingEnabled = false;
        this.tunnelingStats = { totalRequests: 0, tunneledRequests: 0 };
        
        // 🎭 Header Obfuscation поддержка
        this.headerObfuscationEnabled = true; // Автоматически включено
        this.headerMappings = {
            'X-Client-ID': 'X-Req-UUID',
            'X-Secure-Request': 'X-Security-Level'
        };
        this.headerObfuscationStats = {
            totalObfuscated: 0,
            headersMapped: 0,
            fakeHeadersInjected: 0,
            decoyTokensGenerated: 0,
            payloadEmbedded: 0
        };
    }

    generateSessionId() {
        return Array.from(crypto.getRandomValues(new Uint8Array(16)))
            .map(b => b.toString(16).padStart(2, '0')).join('');
    }

    log(message, type = 'info') {
        const timestamp = new Date().toLocaleTimeString();
        const logEntry = `[${timestamp}] ${message}`;
        this.logs.push({ message: logEntry, type });
        console.log(logEntry);
    }

    async establishSoftwareSecureConnection() {
        try {
            this.sessionId = this.generateSessionId();
            this.log(`[SecureClient] Initializing secure connection...`);
            this.log(`[SecureClient] Generated session ID: ${this.sessionId.substring(0,8)}...`);

            // ТЕСТ: Используем серверный ключ из ответа как клиентский
            const ec = new elliptic.ec('p256');
            this._ecKeyPair = ec.genKeyPair();
            const clientPubKeyHex = this._ecKeyPair.getPublic('hex');
            const keyExchangeData = {
                client_id: this.sessionId,
                client_public_key: clientPubKeyHex
            };
            
            this.log(`[SecureClient] Attempting key exchange like test page...`);

            // 🔗 URL Obfuscation: применяем obfuscation если есть mapping
            let keyExchangeURL = '/api/secure/keyexchange';
            if (window.urlObfuscationMap && window.urlObfuscationMap[keyExchangeURL]) {
                keyExchangeURL = window.urlObfuscationMap[keyExchangeURL];
                console.log(`🔗 URL OBFUSCATION: /api/secure/keyexchange -> ${keyExchangeURL}`);
            }

            // 🎭 Header Obfuscation: обфусцируем заголовки для KeyExchange
            const obfuscatedHeaders = {
                'Content-Type': 'application/json',
                'X-Req-UUID': this.sessionId,                    // Обфусцировано: X-Client-ID → X-Req-UUID
                'X-Browser-Engine': 'Chromium/120.0',            // Fake header
                'X-Request-Time': Date.now().toString(),         // Fake header
                'X-Client-Version': '2.1.0',                     // Fake header
                'X-Feature-Flags': 'ecdh,xor,obfuscation'       // Fake header
            };
            console.log(`🎭 HEADER OBFUSCATION: X-Client-ID → X-Req-UUID + 4 fake headers`);

            const response = await fetch(keyExchangeURL, {
                method: 'POST',
                headers: obfuscatedHeaders,
                body: JSON.stringify(keyExchangeData)
            });

            if (response.ok) {
                const data = await response.json();
                this.log(`[SecureClient] Key exchange OK!`, 'success');
                
                // Сохраняем серверный ключ для ECDH
                this.serverPublicKey = data.pubkey;
                // 📉 Убран DEBUG лог - технические детали
                
                // ECDH + HKDF: Derive session key locally (no transmission!)
                if (data.pubkey && data.salt && this._ecKeyPair) {
                    const ec = new elliptic.ec('p256');
                    const serverPubKey = ec.keyFromPublic(data.pubkey, 'hex');
                    
                    // ECDH shared secret (X coordinate, 32 bytes)
                    const sharedPoint = this._ecKeyPair.derive(serverPubKey.getPublic());
                    const sharedSecretHex = sharedPoint.toString('hex').padStart(64, '0');
                    const sharedSecretBits = sjcl.codec.hex.toBits(sharedSecretHex);
                    
                    // HKDF-Extract: PRK = HMAC-SHA256(salt, sharedSecret)
                    const saltBits = sjcl.codec.hex.toBits(data.salt);
                    const hmac1 = new sjcl.misc.hmac(saltBits, sjcl.hash.sha256);
                    const prk = hmac1.encrypt(sharedSecretBits);
                    
                    // HKDF-Expand: sessionKey = HMAC-SHA256(PRK, "SecureLayerV1" || 0x01)
                    const infoBits = sjcl.codec.utf8String.toBits("SecureLayerV1");
                    const counterBits = sjcl.codec.hex.toBits("01");
                    const hmac2 = new sjcl.misc.hmac(prk, sjcl.hash.sha256);
                    hmac2.update(infoBits);
                    hmac2.update(counterBits);
                    const sessionKeyBits = hmac2.digest();
                    
                    this.sessionKey = sjcl.codec.hex.fromBits(sessionKeyBits);
                    this._ecKeyPair = null; // Clear ephemeral private key
                    this.log('ECDH + HKDF complete, session key derived locally', 'info');
                } else {
                    this.log('Key exchange failed: missing pubkey or salt', 'error');
                    return false;
                }
                
                // Попытка вычисления AES ключа
                this.deriveAESKey();
                
                // 🚇 АВТОМАТИЧЕСКОЕ ВКЛЮЧЕНИЕ ТУННЕЛИРОВАНИЯ
                this.enableMethodTunneling();
                // 📉 Убран DEBUG лог - повторяет информацию
                
                // 🎭 АВТОМАТИЧЕСКОЕ ВКЛЮЧЕНИЕ HEADER OBFUSCATION
                this.enableHeaderObfuscation();
                // 📉 Убран DEBUG лог - повторяет информацию
                
                this.isReady = true;
                return true;
            } else {
                const errorText = await response.text();
                this.log(`❌ Key exchange failed: ${response.status} - ${errorText}`, 'error');
                return false;
            }
        } catch (error) {
            this.log(`❌ Key exchange network error: ${error.message}`, 'error');
            return false;
        }
    }

    deriveAESKey() {
        // Теперь мы можем вычислить детерминированный AES ключ!
        // 📉 Убран DEBUG лог - технические детали
        
        // Логика ESP32:
        // 1. clientNonce = первые 16 символов sessionId
        // 2. shared_secret = фиктивный (так как принимает любой ключ)
        // 3. AES key = HKDF(shared_secret, clientNonce)
        
        const clientNonce = this.sessionId.substring(0, 16);
        // 📉 Убран DEBUG лог - технические детали
        
        // Session key received from ESP32 key exchange response
        if (this.sessionKey && this.sessionKey.length === 64) {
            this.aesKey = this.sessionKey;
            // 📉 Убран DEBUG лог - технические детали
        } else {
            this.aesKey = null;
            this.log(`⚠️ Session key missing — encryption disabled`, 'warn');
        }
        
        // 📉 Убраны DEBUG логи - технические детали
    }

    simpleHash(input) {
        // Простой hash для создания детерминированного ключа
        let hash = 0;
        let result = '';
        
        for (let i = 0; i < input.length; i++) {
            hash = ((hash << 5) - hash + input.charCodeAt(i)) & 0xffffffff;
        }
        
        // Расширяем hash до 64 hex символов (32 bytes)
        const baseHash = Math.abs(hash).toString(16).padStart(8, '0');
        for (let i = 0; i < 8; i++) {
            result += baseHash;
        }
        
        return result.substring(0, 64);
    }

    // Method Tunneling Functions
    xorEncrypt(data, key) {
        // XOR fallback шифрование для method header (как в ESP32)
        let result = '';
        for (let i = 0; i < data.length; i++) {
            const charCode = data.charCodeAt(i) ^ key.charCodeAt(i % key.length);
            // Конвертируем в HEX (как на сервере)
            result += charCode.toString(16).padStart(2, '0');
        }
        return result; // HEX encoded string
    }

    encryptMethod(method) {
        // Генерируем тот же ключ что и сервер: "MT_ESP32_" + clientId + "_METHOD_KEY"
        const clientId = this.sessionId || 'UNKNOWN';
        const encryptionKey = 'MT_ESP32_' + clientId + '_METHOD_KEY';
        
        // Ограничиваем длину ключа (max 32 символа как на сервере)
        const limitedKey = encryptionKey.substring(0, 32);
        
        const encryptedMethod = this.xorEncrypt(method, limitedKey);
        // 📉 Убран INFO лог - повторяется на каждый запрос
        return encryptedMethod;
    }

    shouldTunnelEndpoint(endpoint) {
        // Проверяем должен ли endpoint быть туннелирован
        const tunneledEndpoints = [
            // TOTP Keys Management
            '/api/keys',              // ✅ GET - получение ключей
            '/api/add',               // ✅ POST - добавление TOTP ключа
            '/api/remove',            // ✅ POST - удаление TOTP ключа
            '/api/show_qr',           // ✅ POST - показ QR кода на экране
            '/api/hotp/generate',     // ✅ POST - генерация следующего HOTP кода
            '/api/keys/reorder',      // ✅ POST - переупорядочивание TOTP ключей
            '/api/export',            // ✅ POST - экспорт TOTP ключей
            '/api/import',            // ✅ POST - импорт TOTP ключей
            // Passwords Management
            '/api/passwords',
            '/api/passwords/get',
            '/api/passwords/add',
            '/api/passwords/delete',
            '/api/passwords/update',
            '/api/passwords/reorder',
            '/api/passwords/export',
            '/api/passwords/import',
            // Display Settings Management
            '/api/theme',             // ✅ GET/POST - тема устройства
            '/api/display_settings',  // ✅ GET/POST - таймаут экрана
            '/api/splash/mode',       // ✅ GET/POST - выбор splash экрана
            '/api/clock_settings',    // ✅ GET/POST - настройки часов
            '/api/rtc',               // ✅ GET/POST - DS3231 RTC модуль
            '/api/boot-mode',         // ✅ GET/POST - выбор режима загрузки
            '/api/battery',           // ✅ GET - battery status
            // PIN Settings Management
            '/api/pincode_settings',  // ✅ GET/POST - настройки PIN
            '/api/ble_pin_update',    // ✅ POST - BLE PIN обновление
            '/api/clear_ble_clients', // ✅ POST - очистка BLE клиентов
            // Device Settings Management
            '/api/config',            // ✅ GET - конфигурация сервера (timeout)
            '/api/startup_mode',      // ✅ GET/POST - режим запуска
            '/api/settings',          // ✅ GET/POST - настройки устройства
            '/api/ble_settings',      // ✅ GET/POST - настройки BLE
            '/api/mdns_settings',     // ✅ GET/POST - настройки mDNS
            '/api/session_duration',  // ✅ GET/POST - длительность сессии
            '/api/activity',          // ✅ POST - сброс таймера активности
            // API Access Management 🔑
            '/api/enable_import_export',  // ✅ POST - включение API доступа
            '/api/import_export_status',  // ✅ GET - статус API доступа
            // Critical System Operations (NEW) 🔥
            '/logout',                // ✅ POST - выход из системы
            '/api/change_password',   // ✅ POST - смена пароля администратора
            '/api/change_ap_password', // ✅ POST - смена пароля WiFi AP
            '/api/wifi_credentials',  // ✅ POST - смена WiFi credentials
            '/api/clear_ble_clients', // ✅ POST - очистка BLE клиентов
            '/api/reboot',            // ✅ POST - перезагрузка устройства
            '/api/reboot_with_web'    // ✅ POST - перезагрузка с веб-сервером
        ];
        return tunneledEndpoints.some(e => endpoint === e || endpoint.startsWith(e + '?'));
    }

    enableMethodTunneling() {
        this.methodTunnelingEnabled = true;
        // 📉 Убран SUCCESS лог - избыточная информация
    }

    disableMethodTunneling() {
        this.methodTunnelingEnabled = false;
        this.log('🚇 Method Tunneling DISABLED - Using standard HTTP methods', 'info');
    }

    // 🎭 Header Obfuscation Functions
    processHeadersWithObfuscation(headers, endpoint, method) {
        if (!this.headerObfuscationEnabled) return headers;
        
        this.headerObfuscationStats.totalObfuscated++;
        let obfuscatedHeaders = { ...headers };
        let headersMappedCount = 0;
        
        // A) Header Mapping - переименование заголовков
        for (const [original, replacement] of Object.entries(this.headerMappings)) {
            if (obfuscatedHeaders[original]) {
                obfuscatedHeaders[replacement] = obfuscatedHeaders[original];
                delete obfuscatedHeaders[original];
                headersMappedCount++;
            }
        }
        
        // B) Fake Headers Injection - добавление ложных заголовков
        const fakeHeaders = {
            'X-Browser-Engine': 'Mozilla/5.0 (compatible; MSIE 10.0)',
            'X-Request-Time': Date.now().toString(),
            'X-Client-Version': '2.4.1-stable',
            'X-Feature-Flags': 'analytics,tracking,ads',
            'X-Session-State': 'active'
        };
        Object.assign(obfuscatedHeaders, fakeHeaders);
        this.headerObfuscationStats.fakeHeadersInjected += Object.keys(fakeHeaders).length;
        
        // C) Decoy Session Tokens - ложные токены для camouflage
        // Генерируем 2-3 токена похожих на реальные session/JWT
        const generateRandomToken = (length, format = 'hex') => {
            const chars = format === 'hex' ? '0123456789abcdef' : 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
            let token = '';
            for (let i = 0; i < length; i++) {
                token += chars.charAt(Math.floor(Math.random() * chars.length));
            };
            return token;
        };
        
        // Токен 1: JWT-подобный (3 части base64)
        const jwtHeader = btoa(JSON.stringify({alg:'HS256',typ:'JWT'})).replace(/=/g, '');
        const jwtPayload = generateRandomToken(43, 'base64');
        const jwtSignature = generateRandomToken(43, 'base64');
        obfuscatedHeaders['Authorization'] = `Bearer ${jwtHeader}.${jwtPayload}.${jwtSignature}`;
        
        // Токен 2: Session token (32-64 hex)
        const sessionLength = 32 + Math.floor(Math.random() * 33); // 32-64 символов
        obfuscatedHeaders['X-Session-Token'] = generateRandomToken(sessionLength, 'hex');
        
        // Токен 3: CSRF token (40-48 hex)
        const csrfLength = 40 + Math.floor(Math.random() * 9); // 40-48 символов
        obfuscatedHeaders['X-CSRF-Token'] = generateRandomToken(csrfLength, 'hex');
        
        // Иногда добавляем Refresh token (30% вероятность)
        if (Math.random() < 0.3) {
            const refreshLength = 48 + Math.floor(Math.random() * 17); // 48-64
            obfuscatedHeaders['X-Refresh-Token'] = generateRandomToken(refreshLength, 'base64');
            this.headerObfuscationStats.decoyTokensGenerated = (this.headerObfuscationStats.decoyTokensGenerated || 0) + 4;
        } else {
            this.headerObfuscationStats.decoyTokensGenerated = (this.headerObfuscationStats.decoyTokensGenerated || 0) + 3;
        }
        
        // 📉 Убран DEBUG лог - повторяется на каждый запрос
        
        // D) Header Payload Embedding - встраивание метаданных в User-Agent
        const metadata = {
            endpoint: endpoint,
            method: method,
            ts: Date.now()
        };
        const encodedData = btoa(JSON.stringify(metadata));
        const baseUserAgent = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36';
        obfuscatedHeaders['User-Agent'] = `${baseUserAgent} (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 EdgeInsight/${encodedData}`;
        this.headerObfuscationStats.payloadEmbedded++;
        
        this.headerObfuscationStats.headersMapped += headersMappedCount;
        
        return obfuscatedHeaders;
    }
    
    enableHeaderObfuscation() {
        this.headerObfuscationEnabled = true;
        // 📉 Убран SUCCESS лог - избыточная информация
    }
    
    disableHeaderObfuscation() {
        this.headerObfuscationEnabled = false;
        this.log('🎭 Header Obfuscation DISABLED - Using standard headers', 'info');
    }

    // Простой метод проверки что нужно шифровать
    shouldSecureEndpoint(url) {
        const secureEndpoints = [
            '/api/keys',
            '/api/add',        // 🔐 TOTP key management
            '/api/remove',     // 🔐 TOTP key management
            '/api/show_qr',    // 🔐 QR code export (displays secret on screen)
            '/api/hotp/generate', // 🔐 HOTP code generation
            '/api/export',     // 🔐 TOTP key export
            '/api/import',     // 🔐 TOTP key import
            '/api/config',     // 🔐 Server configuration (timeout settings)
            '/api/keys/reorder', // 🔐 TOTP keys reordering
            '/api/passwords',  // 🔐 All passwords list 
            '/api/passwords/get', 
            '/api/passwords/add',
            '/api/passwords/delete',
            '/api/passwords/update',
            '/api/passwords/reorder',
            '/api/passwords/export',
            '/api/passwords/import',
            '/api/pincode_settings',   // 🔐 PIN settings (security configuration)
            '/api/ble_pin_update',     // 🔐 BLE PIN update (security sensitive)
            '/api/settings',           // 🔐 Device settings (admin login info)
            '/api/ble_settings',       // 🔐 BLE device name configuration
            '/api/mdns_settings',      // 🔐 mDNS hostname configuration
            '/api/startup_mode',       // 🔐 Startup mode configuration
            '/api/change_password',    // 🔐 Admin password change (critical!)
            '/api/change_ap_password', // 🔐 WiFi AP password change (critical!)
            '/api/wifi_credentials',   // 🔐 WiFi credentials update (critical!)
            '/api/session_duration',   // 🔐 Session duration settings (security)
            '/api/activity',           // 🔐 Session activity keepalive
            '/logout',                 // 🔐 Admin logout (session termination)
            '/api/clear_ble_clients',  // 🔐 Clear BLE bonded clients (critical!)
            '/api/reboot',             // 🔐 System reboot (critical!)
            '/api/reboot_with_web',    // 🔐 System reboot with web server (critical!)
            '/api/theme',              // 🔐 Display theme settings (NEW)
            '/api/display_settings',   // 🔐 Display timeout settings (NEW)
            '/api/splash/mode',        // 🔐 Splash screen selection (NEW)
            '/api/clock_settings',     // 🔐 Clock display settings (NEW)
            '/api/rtc',                // 🔐 DS3231 RTC module (NEW)
            '/api/boot-mode',          // 🔐 Boot mode selection (NEW)
            '/api/battery',            // 🔐 Battery status (NEW)
            '/api/enable_import_export', // 🔐 API access control (security)
            '/api/import_export_status'  // 🔐 API access status (security)
        ];
        return secureEndpoints.some(endpoint => url === endpoint || url.startsWith(endpoint + '/') || url.startsWith(endpoint + '?'));
    }

    async decryptTOTPResponse(responseText) {
        try {
            const data = JSON.parse(responseText);
            
            // Проверяем что это зашифрованный ответ ESP32
            if (data.type === "secure" && data.data && data.iv && data.tag) {
                // Попытка расшифровки с нашим AES ключом
                if (this.aesKey) {
                    try {
                        const decrypted = await this.simpleAESDecrypt(data.data, data.iv, data.tag);
                        if (decrypted) {
                            const decryptedData = JSON.parse(decrypted);
                            // 📉 Убран DEBUG лог - повторяется очень часто
                            return decryptedData;
                        }
                    } catch (decErr) {
                        this.log(`❌ Decryption error: ${decErr.message}`, 'error');
                    }
                }
                
                // Fallback: показываем placeholder как массив
                this.log(`⚠️ Decryption failed, showing placeholders`, 'warn');
                return [
                    {
                        name: "🔐 Encrypted Key 1",
                        code: "------",
                        timeLeft: 30
                    },
                    {
                        name: "🔐 Encrypted Key 2", 
                        code: "------",
                        timeLeft: 30
                    }
                ];
            }
            
            // Если не зашифровано, возвращаем как есть
            return data;
            
        } catch (error) {
            this.log(`❌ TOTP decryption error: ${error.message}`, 'error');
            return JSON.parse(responseText); // Fallback
        }
    }

    // 🔐 Шифрование запроса (симметрично с decryption на сервере)
    async simpleAESEncrypt(plaintext) {
        if (!this.aesKey || this.aesKey.length !== 64) {
            this.log('No valid AES key', 'error');
            return null;
        }
        
        try {
            const keyBits = sjcl.codec.hex.toBits(this.aesKey);
            const ivArray = new Uint8Array(12);
            crypto.getRandomValues(ivArray);
            const ivHex = Array.from(ivArray).map(b => b.toString(16).padStart(2, '0')).join('');
            const ivBits = sjcl.codec.hex.toBits(ivHex);
            
            const cipher = new sjcl.cipher.aes(keyBits);
            const plaintextBits = sjcl.codec.utf8String.toBits(plaintext);
            
            // AES-256-GCM encrypt, 128-bit tag
            const encrypted = sjcl.mode.gcm.encrypt(cipher, plaintextBits, ivBits, [], 128);
            
            // sjcl appends 128-bit tag to ciphertext — split them
            const encryptedHex = sjcl.codec.hex.fromBits(encrypted);
            const dataHex = encryptedHex.slice(0, -32);   // all except last 16 bytes
            const tagHex  = encryptedHex.slice(-32);       // last 16 bytes = 128-bit tag
            
            return JSON.stringify({
                type: "secure",
                data: dataHex,
                iv: ivHex,
                tag: tagHex,
                counter: this.requestCounter++
            });
        } catch (err) {
            this.log('AES-GCM encrypt error: ' + err.message, 'error');
            return null;
        }
    }

    async simpleAESDecrypt(hexData, hexIv, hexTag) {
        try {
            const keyBits = sjcl.codec.hex.toBits(this.aesKey);
            const ivBits  = sjcl.codec.hex.toBits(hexIv);
            
            // Recombine data + tag for sjcl (it expects them concatenated)
            const combinedHex = hexData + hexTag;
            const combinedBits = sjcl.codec.hex.toBits(combinedHex);
            
            const cipher = new sjcl.cipher.aes(keyBits);
            
            // AES-256-GCM decrypt — throws if tag is invalid
            const decryptedBits = sjcl.mode.gcm.decrypt(cipher, combinedBits, ivBits, [], 128);
            const decrypted = sjcl.codec.utf8String.fromBits(decryptedBits);
            
            let clean = decrypted.trim();
            let start = clean.indexOf('[');
            if (start === -1) start = clean.indexOf('{');
            if (start >= 0) return clean.substring(start);
            
            this.log('No JSON found in decrypted response', 'warn');
            return null;
        } catch (err) {
            this.log('AES-GCM decrypt failed (tag mismatch or corrupt): ' + err.message, 'error');
            return null;
        }
    }

    hexToBytes(hex) {
        const bytes = [];
        for (let i = 0; i < hex.length; i += 2) {
            bytes.push(parseInt(hex.substr(i, 2), 16));
        }
        return bytes;
    }
}

// Создаем глобальный экземпляр SecureClient
window.secureClient = new SecureClient();

// 🔐 УНИВЕРСАЛЬНАЯ ФУНКЦИЯ ДЛЯ ЗАШИФРОВАННЫХ ЗАПРОСОВ
async function makeEncryptedRequest(url, options = {}) {
    // 🔐 КРИТИЧНО: Добавляем заголовки для активации шифрования
    if (!options.headers) {
        options.headers = {};
    }
    
    // Добавляем Client ID если secureClient готов
    if (window.secureClient && window.secureClient.isReady && window.secureClient.sessionId) {
        options.headers['X-Client-ID'] = window.secureClient.sessionId;
        options.headers['X-Secure-Request'] = 'true';
        console.log('🔐 Adding headers for encryption:', window.secureClient.sessionId.substring(0,8) + '...');
        // Шифрование будет выполнено в makeAuthenticatedRequest
    }
    
    // Добавляем заголовки для принудительной активации шифрования
    options.headers['X-Security-Level'] = 'secure';
    options.headers['X-User-Activity'] = 'true';
    
    const response = await makeAuthenticatedRequest(url, options);
    
    // Возвращаем Response объект для совместимости
    return response;
}

// Authenticated fetch wrapper with CSRF protection and auto-logout
async function makeAuthenticatedRequest(url, options = {}) {
    if (!options.headers) {
        options.headers = {};
    }
    
    // Add CSRF token to POST requests
    if (options.method === 'POST' && csrfToken) {
        options.headers['X-CSRF-Token'] = csrfToken;
    }
    
    // 🚇 METHOD TUNNELING - СКРЫВАЕМ HTTP МЕТОДЫ ОТ АНАЛИЗА ТРАФИКА
    let originalUrl = url;
    let originalMethod = options.method || 'GET';
    
    if (window.secureClient && window.secureClient.methodTunnelingEnabled && 
        window.secureClient.shouldTunnelEndpoint(url)) {
        
        // Увеличиваем статистику
        window.secureClient.tunnelingStats.totalRequests++;
        window.secureClient.tunnelingStats.tunneledRequests++;
        
        // Шифруем реальный метод
        const encryptedMethod = window.secureClient.encryptMethod(originalMethod);
        options.headers['X-Real-Method'] = encryptedMethod;
        
        // 🔧 КОНВЕРТИРУЕМ FormData/URLSearchParams в объект
        let bodyData = {};
        
        if (options.body instanceof FormData) {
            // ✅ Конвертируем FormData в объект
            for (const [key, value] of options.body.entries()) {
                // ⚠️ ВАЖНО: File/Blob объекты уже должны быть прочитаны как строки!
                // handleImport использует FileReader.readAsText() перед вызовом
                if (value instanceof File || value instanceof Blob) {
                    console.error(`❌ FormData contains File/Blob for key '${key}' - should be read as text first!`);
                    console.warn(`⚠️ File name: ${value.name || 'unknown'}, size: ${value.size}b`);
                    // Пропускаем File объекты - они должны быть прочитаны через FileReader
                    bodyData[key] = ''; // Пустая строка вместо File объекта
                } else {
                    bodyData[key] = value;
                }
            }
            // 📉 Убран DEBUG лог - повторяется часто
        } else if (options.body instanceof URLSearchParams) {
            // ✅ Конвертируем URLSearchParams в объект
            for (const [key, value] of options.body.entries()) {
                bodyData[key] = value;
            }
            // 📉 Убран DEBUG лог - повторяется часто
        } else if (typeof options.body === 'string') {
            // ✅ Проверяем на JSON строку (начинается с { или [)
            const trimmed = options.body.trim();
            if (trimmed.startsWith('{') || trimmed.startsWith('[')) {
                // Это JSON - парсим как JSON
                try {
                    bodyData = JSON.parse(options.body);
                    // Body parsed successfully
                } catch (e) {
                    console.error(`❌ Failed to parse JSON body:`, e.message);
                    bodyData = { raw: options.body };
                }
            } else {
                // Это URL-encoded - парсим как URLSearchParams
                try {
                    const params = new URLSearchParams(options.body);
                    for (const [key, value] of params.entries()) {
                        bodyData[key] = value;
                    }
                    console.log(`🔧 Parsed URL-encoded string to object:`, Object.keys(bodyData));
                } catch (e) {
                    bodyData = { raw: options.body };
                    console.warn(`⚠️ Failed to parse body string, using raw:`, e.message);
                }
            }
        } else if (options.body && typeof options.body === 'object') {
            // ✅ Уже объект - используем как есть
            bodyData = options.body;
        }
        // else: GET запросы имеют пустой bodyData = {}
        
        // Преобразуем в POST запрос к /api/tunnel
        const tunnelBody = {
            endpoint: url,
            method: originalMethod,  // 👉 Добавляем метод для сервера
            data: bodyData
        };
        
        // ОБНОВЛЯЕМ URL И МЕТОД
        url = '/api/tunnel';
        options.method = 'POST';
        options.body = JSON.stringify(tunnelBody);
        options.headers['Content-Type'] = 'application/json';
        
        // 📉 Убраны DEBUG логи - повторяются на каждый запрос
        
        // 🔗 URL OBFUSCATION - применяем к /api/tunnel endpoint
        if (window.urlObfuscationMap && window.urlObfuscationMap['/api/tunnel']) {
            const obfuscatedUrl = window.urlObfuscationMap['/api/tunnel'];
            // 📉 Убран DEBUG лог - повторяется на каждый запрос
            url = obfuscatedUrl;
        }
        // 🔍 DEBUG: Показываем размеры данных для import
        if (originalUrl === '/api/import' && bodyData.data) {
        }
    }
    
    // 🔐 ШИФРОВАНИЕ ДЛЯ ЧУВСТВИТЕЛЬНЫХ ENDPOINTS
    if (window.secureClient && window.secureClient.shouldSecureEndpoint(originalUrl)) {
        if (window.secureClient.isReady && window.secureClient.sessionId) {
            options.headers['X-Client-ID'] = window.secureClient.sessionId;
            options.headers['X-Secure-Request'] = 'true';
            // 📉 Убран DEBUG лог - повторяется на каждый запрос
            
            // 🔐 ШИФРОВАНИЕ ТЕЛА ЗАПРОСА
            if (options.method === 'POST' && options.body && window.secureClient.aesKey) {
                // 📉 Убран DEBUG лог - повторяется на каждый запрос
                
                // Поддержка FormData и URLSearchParams
                let plaintext;
                if (options.body instanceof FormData) {
                    // Конвертируем FormData в URLSearchParams строку
                    plaintext = new URLSearchParams(options.body).toString();
                } else if (options.body instanceof URLSearchParams) {
                    plaintext = options.body.toString();
                } else {
                    plaintext = options.body;
                }
                
                const encryptedBody = await window.secureClient.simpleAESEncrypt(plaintext);
                
                if (encryptedBody) {
                    options.body = encryptedBody;
                    options.headers['Content-Type'] = 'application/json'; // Зашифрованные данные в JSON
                    // 📉 Убран DEBUG лог - повторяется на каждый запрос
                } else {
                    console.warn('🔐 Failed to encrypt request body, sending as-is'); // ❗ Оставлен - важное предупреждение
                }
            }
        } else {
            console.warn(`SecureClient not ready (exists: ${!!window.secureClient}, ready: ${window.secureClient?.isReady})`);
        }
    }
    
    // 🎭 HEADER OBFUSCATION - применяем ПОСЛЕ добавления X-Client-ID и X-Secure-Request
    if (window.secureClient && window.secureClient.headerObfuscationEnabled) {
        options.headers = window.secureClient.processHeadersWithObfuscation(
            options.headers,
            originalUrl,
            originalMethod
        );
        // 📉 Убран DEBUG лог - повторяется на каждый запрос
    }
    
    try {
        const response = await fetch(url, options);
        
        // Auto-logout on authentication/authorization failures
        if (response.status === 401 || response.status === 403) {
            console.log('Authentication failed, redirecting to login...');
            CacheManager.clear(); // 🧹 Очистка кеша при провале аутентификации
            redirectToLogin();
            return response;
        }
        
        // 🔐 АВТОМАТИЧЕСКАЯ РАСШИФРОВКА ОТВЕТОВ для зашифрованных запросов
        // ✅ FIX: Расшифровываем ответы независимо от статус кода (400, 200, etc.)
        if (window.secureClient && window.secureClient.shouldSecureEndpoint(originalUrl) && 
            window.secureClient.isReady) {
            
            // Export endpoints: unwrap secure layer but return raw decrypted string (password-encrypted blob)
            if (originalUrl === '/api/export' || originalUrl === '/api/passwords/export') {
                const exportText = await response.clone().text();
                try {
                    const exportData = JSON.parse(exportText);
                    if (exportData.type === "secure") {
                        const decrypted = await window.secureClient.simpleAESDecrypt(
                            exportData.data,
                            exportData.iv,
                            exportData.tag
                        );
                        if (decrypted) {
                            return new Response(decrypted, { status: response.status, headers: response.headers });
                        }
                    }
                } catch(e) {
                    console.warn('Export decrypt error:', e);
                }
                return response;
            }
            
            // Создаем новый response с расшифрованными данными
            const responseText = await response.clone().text();
            
            try {
                const originalData = JSON.parse(responseText);
                
                // Проверяем, является ли ответ зашифрованным
                if (originalData.type === "secure") {
                    const decryptedData = await window.secureClient.decryptTOTPResponse(responseText);
                    
                    if (decryptedData) {
                        // Создаем новый response с расшифрованными данными
                        const decryptedText = typeof decryptedData === 'string' ? decryptedData : JSON.stringify(decryptedData);
                        
                        return new Response(decryptedText, {
                            status: response.status,
                            statusText: response.statusText,
                            headers: response.headers
                        });
                    } else {
                        console.warn(`⚠️ Failed to decrypt response for ${originalUrl}, using original`);
                    }
                }
            } catch (parseError) {
                // Возможно, это не JSON ответ или ошибка расшифровки - используем оригинальный
                console.log(`🔐 Response parsing failed for ${originalUrl}, using original:`, parseError.message);
            }
        }
        
        return response;
    } catch (error) {
        console.error('Network error in API request:', error);
        // On network errors, check if we can still access the login page
        // If not, likely a server connectivity issue
        if (error instanceof TypeError && error.message.includes('NetworkError')) {
            console.log('Network connectivity issue detected');
            // Don't redirect on network errors - might be temporary server restart
        }
        throw error; // Re-throw to be handled by caller
    }
}


// Drag and Drop functionality
function initializeDragAndDrop(tableId, dataType) {
    const table = document.getElementById(tableId);
    if (!table) return;
    
    const tbody = table.querySelector('tbody');
    let draggedElement = null;
    
    // Desktop drag and drop
    tbody.addEventListener('dragstart', function(e) {
        if (e.target.closest('.draggable-row')) {
            draggedElement = e.target.closest('.draggable-row');
            draggedElement.classList.add('dragging');
            e.dataTransfer.effectAllowed = 'move';
            e.dataTransfer.setData('text/html', draggedElement.outerHTML);
        }
    });
    
    tbody.addEventListener('dragend', function(e) {
        if (draggedElement) {
            draggedElement.classList.remove('dragging');
            draggedElement = null;
        }
        // Remove all drop-zone classes
        tbody.querySelectorAll('.drop-zone').forEach(row => {
            row.classList.remove('drop-zone');
        });
    });
    
    tbody.addEventListener('dragover', function(e) {
        e.preventDefault();
        const afterElement = getDragAfterElement(tbody, e.clientY);
        const dragging = tbody.querySelector('.dragging');
        
        // Remove existing drop-zone classes
        tbody.querySelectorAll('.drop-zone').forEach(row => {
            row.classList.remove('drop-zone');
        });
        
        if (afterElement == null) {
            tbody.appendChild(dragging);
        } else {
            afterElement.classList.add('drop-zone');
            tbody.insertBefore(dragging, afterElement);
        }
    });
    
    tbody.addEventListener('drop', function(e) {
        e.preventDefault();
        const fromIndex = parseInt(draggedElement.dataset.index);
        const toIndex = getNewIndex(tbody, draggedElement);
        
        if (fromIndex !== toIndex) {
            reorderItems(dataType, fromIndex, toIndex);
        }
        
        // Clean up
        tbody.querySelectorAll('.drop-zone').forEach(row => {
            row.classList.remove('drop-zone');
        });
    });
    
    // Mobile touch support
    let touchStartY = 0;
    let touchElement = null;
    
    tbody.addEventListener('touchstart', function(e) {
        const row = e.target.closest('.draggable-row');
        if (row && e.target.closest('.drag-handle')) {
            touchStartY = e.touches[0].clientY;
            touchElement = row;
            row.classList.add('dragging');
            e.preventDefault();
        }
    });
    
    tbody.addEventListener('touchmove', function(e) {
        if (touchElement) {
            e.preventDefault();
            const touch = e.touches[0];
            const currentY = touch.clientY;
            
            // Visual feedback
            const afterElement = getDragAfterElement(tbody, currentY);
            tbody.querySelectorAll('.drop-zone').forEach(row => {
                row.classList.remove('drop-zone');
            });
            
            if (afterElement) {
                afterElement.classList.add('drop-zone');
            }
        }
    });
    
    tbody.addEventListener('touchend', function(e) {
        if (touchElement) {
            const touch = e.changedTouches[0];
            const afterElement = getDragAfterElement(tbody, touch.clientY);
            const fromIndex = parseInt(touchElement.dataset.index);
            
            // Calculate new position
            if (afterElement) {
                tbody.insertBefore(touchElement, afterElement);
            } else {
                tbody.appendChild(touchElement);
            }
            
            const toIndex = getNewIndex(tbody, touchElement);
            
            if (fromIndex !== toIndex) {
                reorderItems(dataType, fromIndex, toIndex);
            }
            
            // Clean up
            touchElement.classList.remove('dragging');
            touchElement = null;
            tbody.querySelectorAll('.drop-zone').forEach(row => {
                row.classList.remove('drop-zone');
            });
        }
    });
}

function getDragAfterElement(container, y) {
    const draggableElements = [...container.querySelectorAll('.draggable-row:not(.dragging)')];
    
    return draggableElements.reduce((closest, child) => {
        const box = child.getBoundingClientRect();
        const offset = y - box.top - box.height / 2;
        
        if (offset < 0 && offset > closest.offset) {
            return { offset: offset, element: child };
        } else {
            return closest;
        }
    }, { offset: Number.NEGATIVE_INFINITY }).element;
}

function getNewIndex(tbody, draggedElement) {
    const rows = [...tbody.querySelectorAll('.draggable-row')];
    return rows.indexOf(draggedElement);
}

function reorderItems(dataType, fromIndex, toIndex) {
    if (dataType === 'keys') {
        const item = keysData.splice(fromIndex, 1)[0];
        keysData.splice(toIndex, 0, item);
        // Send update to server (includes localStorage backup)
        saveKeysOrder();
        updateKeysTable(keysData); // Rebuild table to fix timer/progress element IDs
    } else if (dataType === 'passwords') {
        const item = passwordsData.splice(fromIndex, 1)[0];
        passwordsData.splice(toIndex, 0, item);
        // Send update to server (includes localStorage backup)
        savePasswordsOrder();
    }
}

function saveKeysOrder() {
    const orderData = keysData.map((key, index) => ({ name: key.name, order: index }));
    makeAuthenticatedRequest('/api/keys/reorder', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ order: orderData })
    }).then(res => {
        if (res.ok) {
            CacheManager.invalidate('keys_list'); // ♻️ Инвалидация кеша
            showStatus(tr('keys.order.saved'));
        } else {
            showStatus('Failed to save keys order.', true);
        }
    }).catch(err => {
        console.warn('Keys reorder API error:', err);
        showStatus('Failed to save keys order.', true);
    });
}

function savePasswordsOrder() {
    const orderData = passwordsData.map((password, index) => ({ name: password.name, order: index }));
    makeAuthenticatedRequest('/api/passwords/reorder', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ order: orderData })
    }).then(res => {
        if (res.ok) {
            CacheManager.invalidate('passwords_list'); // ♻️ Инвалидация кеша
            fetchPasswords();
            showStatus(tr('passwords.order.saved'));
        } else {
            showStatus(tr('passwords.order.failed'), true);
        }
    }).catch(err => {
        console.warn('Passwords reorder API error:', err);
        showStatus(tr('passwords.order.failed'), true);
    });
}

const i18n={ru:{"app.title":"Панель управления","tab.keys":"Ключи","tab.passwords":"Пароли","tab.display":"Экран","tab.pin":"PIN","tab.settings":"Настройки","keys.title":"Управление ключами","keys.add.title":"Добавить ключ","keys.add.btn":"Добавить ключ","keys.current.title":"Текущие ключи","keys.importexport.title":"Импорт/Экспорт ключей","keys.advanced":"Расширенные настройки","keys.hotp.sync":"Счётчики HOTP синхронизируются вместе с TOTP ключами","keys.api.status":"Статус API:","keys.api.inactive":"Неактивен","keys.add.success":"✅ Ключ успешно добавлен!","keys.remove.success":"Ключ удалён!","keys.order.saved":"Порядок ключей сохранён!","keys.api.enabled":"Доступ к API открыт на 5 минут.","keys.hotp.generated":"✅ Код HOTP сгенерирован","keys.copied":"TOTP код скопирован!","keys.hotp_copied":"HOTP код скопирован!","ui.loading":"Загрузка...","keys.col.name":"Имя","keys.col.code":"Код","keys.col.timer":"Таймер","keys.col.progress":"Прогресс","keys.col.actions":"Действия","keys.btn.remove":"Удалить","keys.btn.next":"🔄 След","keys.api.enable":"Включить API (5 мин)","keys.field.name":"Имя:","keys.field.secret":"Секрет (Base32):","keys.field.algorithm":"Алгоритм:","keys.type.totp":"TOTP (на основе времени)","keys.type.hotp":"HOTP (на основе счётчика)","keys.digits.6":"6 цифр","keys.digits.8":"8 цифр","keys.period.30":"30 секунд","keys.period.60":"60 секунд","keys.field.digits":"Цифры:","keys.field.period":"Период:","keys.algo.sha1":"SHA1 (Стандарт)","passwords.title":"Управление паролями","passwords.add.title":"Добавить пароль","passwords.field.name":"Имя:","passwords.field.password":"Пароль:","passwords.add.btn":"Добавить пароль","passwords.current.title":"Текущие пароли","passwords.col.name":"Имя","passwords.col.actions":"Действия","passwords.btn.copy":"Копировать","passwords.btn.edit":"Изменить","passwords.btn.remove":"Удалить","passwords.copied":"Пароль скопирован!","passwords.updated":"Пароль обновлён!","passwords.removed":"Пароль удалён!","passwords.added":"Пароль добавлен!","passwords.order.saved":"Порядок паролей сохранён!","passwords.order.failed":"Ошибка сохранения порядка паролей.","passwords.gen.title":"Генератор паролей","passwords.gen.desc":"Выберите длину и сгенерируйте надёжный пароль","passwords.gen.generated":"Сгенерированный пароль:","passwords.gen.btn.generate":"Сгенерировать","passwords.gen.btn.use":"Использовать пароль","passwords.edit.title":"Редактировать пароль","passwords.edit.desc":"Изменить имя и пароль для этой записи","passwords.edit.name":"Имя:","passwords.edit.password":"Пароль:","passwords.edit.btn.save":"Сохранить","passwords.edit.btn.cancel":"Отмена","passwords.strength.weak":"Слабый пароль","passwords.strength.medium":"Средний пароль","passwords.strength.strong":"Надёжный пароль","passwords.strength.encryption":"Ключ шифрования","passwords.importexport.title":"Импорт/Экспорт паролей","passwords.btn.export":"Экспорт паролей","passwords.btn.import":"Импорт паролей","passwords.gen.length":"Длина пароля:","passwords.modal.confirm":"Подтвердить","passwords.export.title":"Подтверждение экспорта","passwords.export.desc":"Введите пароль администратора для шифрования и экспорта паролей.","passwords.import.title":"Подтверждение импорта","passwords.import.desc":"Введите пароль администратора для расшифровки и импорта паролей.","system.shutdown.desc":"Веб-сервер автоматически отключится из-за неактивности.","system.shutdown.remaining":"Осталось:","display.title":"Настройки экрана","display.theme.saved":"Тема успешно применена!","display.theme.title":"Выбор темы","display.theme.light":"Светлая тема","display.theme.dark":"Тёмная тема","display.theme.btn":"Применить тему","display.splash.title":"Заставка","display.splash.label":"Режим встроенной заставки:","display.splash.disabled":"Отключено (без заставки)","display.splash.btn":"Сохранить","display.splash.saved":"Режим заставки сохранён! Перезагрузите устройство.","display.splash.select":"Выберите режим заставки","display.clock.title":"Настройки часов","display.clock.timezone":"Часовой пояс:","display.clock.save.tz":"Сохранить часовой пояс","display.clock.tz.saved":"Часовой пояс сохранён! Применено немедленно.","display.clock.tz.select":"Выберите часовой пояс","display.rtc.title":"Модуль DS3231 RTC","display.rtc.desc":"Аппаратные часы для офлайн/AP режима","display.rtc.sda":"Пин SDA","display.rtc.scl":"Пин SCL","display.rtc.btn":"Синхронизировать и сохранить","display.rtc.saving":"Сохранение...","display.rtc.saved":"Настройки RTC сохранены","display.rtc.status.notfound":"Не найдено","display.rtc.status.check":"Проверьте пины I2C","display.rtc.status.drifted":"Дрейф времени","display.rtc.status.ok":"ОК","display.rtc.hint.ntp":"Время синхронизируется с NTP при следующей загрузке с WiFi.","display.rtc.hint.browser":"Время синхронизируется с часами браузера при сохранении.","display.lock.title":"Экран и блокировка","display.lock.screen.label":"Тайм-аут экрана (выключить дисплей через):","display.lock.auto.label":"Автоблокировка (глубокий сон + PIN через):","display.lock.warning":"Автоблокировка должна быть длиннее тайм-аута экрана, если оба включены.","display.lock.btn":"Сохранить настройки","display.lock.saved":"Настройки экрана успешно сохранены!","display.lock.error.order":"Тайм-аут автоблокировки должен быть больше тайм-аута экрана!","display.lock.never":"Никогда","display.lock.opt.15s":"15 секунд","display.lock.opt.30s":"30 секунд","display.lock.opt.1m":"1 минута","display.lock.opt.5m":"5 минут","display.lock.opt.15m":"15 минут","display.lock.opt.30m":"30 минут","display.lock.opt.1h":"1 час","display.lock.opt.4h":"4 часа","pin.title":"Настройки PIN-кода","pin.startup.label":"PIN при запуске","pin.startup.desc":"Шифровать ключ устройства с PIN при запуске","pin.startup.btn":"Сохранить настройки PIN","pin.cancelled":"Изменение PIN отменено.","pin.reset.cancelled":"Сброс отменён.","pin.rebooting":"Устройство перезагружается... Подождите.","pin.ble.title":"PIN для BLE-сопряжения","pin.ble.notice":"Этот PIN отображается на экране устройства во время BLE-сопряжения для аутентификации клиента.","pin.ble.label.new":"Новый PIN для BLE (6 цифр):","pin.ble.label.confirm":"Подтвердите PIN для BLE:","pin.ble.important":"Этот PIN будет отображаться на экране ESP32 во время BLE-сопряжения для ввода клиентом.","pin.ble.btn":"Обновить PIN для BLE","pin.ble.saved":"PIN для BLE обновлён! Все BLE-клиенты отключены.","pin.ble.error.digits":"PIN для BLE должен быть ровно 6 цифр!","pin.ble.error.match":"PIN для BLE не совпадают!","pin.placeholder.enter":"Введите 6-значный PIN","pin.placeholder.confirm":"Подтвердите 6-значный PIN","pin.device.title":"PIN устройства BLE","pin.device.notice":"Этот PIN требуется на аппаратном устройстве при передаче паролей через BLE.","pin.device.label.enable":"Включить PIN устройства BLE","pin.device.desc":"Требовать PIN на устройстве для передачи паролей через BLE","pin.device.label.new":"Новый PIN устройства BLE (6 цифр):","pin.device.label.confirm":"Подтвердите PIN устройства BLE:","pin.device.important":"Этот PIN будет требоваться на экране устройства при передаче паролей через BLE.","pin.device.btn":"Сохранить PIN устройства BLE","pin.device.saved":"PIN устройства BLE обновлён!","pin.device.disabled":"PIN устройства BLE отключён","pin.device.error.enable":"Сначала включите PIN устройства BLE","pin.device.error.digits":"PIN устройства BLE должен быть ровно 6 цифр!","pin.device.error.match":"PIN устройства BLE не совпадают!","settings.title":"Настройки устройства","settings.pwd.title":"Управление паролями","settings.pwd.toggle.web":"Веб-кабинет","settings.pwd.toggle.wifi":"Точка доступа WiFi","settings.pwd.web.form.title":"Сменить пароль веб-кабинета","settings.pwd.web.desc":"Сменить пароль для доступа к этому веб-интерфейсу.","settings.pwd.current.login":"Текущий логин:","settings.pwd.web.new.label":"Новый пароль веб-кабинета","settings.pwd.web.confirm.label":"Подтвердите новый пароль","settings.pwd.req.length":"Минимум 8 символов","settings.pwd.req.upper":"Заглавная буква","settings.pwd.req.lower":"Строчная буква","settings.pwd.req.number":"Цифра","settings.pwd.req.special":"Специальный символ (!@#$%)","settings.pwd.web.btn":"Сменить пароль","settings.pwd.wifi.form.title":"Сменить пароль точки доступа WiFi","settings.pwd.wifi.desc":"Сменить пароль точки доступа WiFi в режиме AP.","settings.pwd.wifi.new.label":"Новый пароль WiFi AP","settings.pwd.wifi.confirm.label":"Подтвердите новый пароль WiFi AP","settings.pwd.wifi.btn":"Сменить пароль WiFi","settings.pwd.err.match":"Пароли не совпадают!","settings.pwd.err.requirements":"Пароль не соответствует требованиям!","settings.pwd.err.wifi.short":"Пароль WiFi должен быть не менее 8 символов!","settings.pwd.web.saved":"Пароль успешно изменён!","settings.pwd.wifi.saved":"Пароль AP изменён. Устройство перезагружается.","settings.pwd.wifi.restarted":"Устройство перезагружено. Подключитесь к WiFi AP с новым паролем.","settings.pwd.match":"Пароли совпадают!","settings.ble.title":"Настройки Bluetooth","settings.ble.label":"Имя BLE устройства (макс. 15 символов):","settings.ble.btn":"Сохранить имя BLE","settings.ble.saved":"Имя BLE устройства обновлено!","settings.mdns.title":"Настройки mDNS","settings.mdns.label":"Имя хоста mDNS (напр., 't-disp-totp'):","settings.mdns.btn":"Сохранить имя хоста mDNS","settings.mdns.saved":"Имя хоста mDNS обновлено!","settings.wifi.title":"📶 Сеть WiFi","settings.wifi.ssid.label":"SSID сети","settings.wifi.ssid.placeholder":"Название сети WiFi","settings.wifi.pwd.label":"Пароль","settings.wifi.pwd.placeholder":"Пароль WiFi (мин. 8 символов)","settings.wifi.confirm.label":"Подтвердите пароль","settings.wifi.confirm.placeholder":"Повторите пароль","settings.wifi.btn":"Сохранить данные WiFi","settings.wifi.note":"⚠ Изменения применятся после перезагрузки.","settings.wifi.err.ssid":"SSID обязателен.","settings.wifi.err.match":"Пароли не совпадают.","settings.wifi.err.short":"Пароль должен быть не менее 8 символов.","settings.wifi.err.failed":"Запрос не выполнен.","settings.wifi.saved":"Данные WiFi сохранены!","settings.scan.scanning":"⏳ Сканирование...","settings.scan.btn":"📡 Сканировать","settings.scan.failed":"Сканирование не удалось","settings.startup.title":"Режим запуска","settings.startup.label":"Режим по умолчанию при запуске:","settings.startup.opt.totp":"TOTP-аутентификатор","settings.startup.opt.pwd":"Менеджер паролей","settings.startup.btn":"Сохранить режим запуска","settings.startup.saved":"Режим запуска успешно сохранён!","settings.boot.title":"Режим загрузки","settings.boot.label":"Сетевой режим по умолчанию при загрузке:","settings.boot.opt.wifi":"Режим WiFi","settings.boot.opt.ap":"Режим AP","settings.boot.opt.offline":"Автономный режим","settings.boot.note":"Переопределение доступно кнопками при загрузке (окно 2 сек).","settings.boot.btn":"Сохранить режим загрузки","settings.boot.saving":"Сохранение...","settings.boot.saved":"Режим загрузки сохранён! Применится при следующей перезагрузке.","settings.webserver.title":"Веб-сервер","settings.webserver.label":"Автовыключение при неактивности:","settings.webserver.opt.5m":"5 минут","settings.webserver.opt.10m":"10 минут","settings.webserver.opt.1h":"1 час","settings.webserver.opt.never":"Никогда","settings.webserver.btn":"Сохранить","settings.webserver.saving":"Сохранение настроек и перезапуск устройства...","settings.session.title":"Таймер автовыхода","settings.session.label":"Время сессии:","settings.session.opt.reboot":"До перезагрузки устройства","settings.session.opt.1h":"1 час","settings.session.opt.6h":"6 часов (по умолчанию)","settings.session.opt.24h":"24 часа","settings.session.opt.3d":"3 дня","settings.session.notice":"Управляет временем автовыхода для безопасности. Сессии сохраняются при перезагрузке, кроме режима «До перезагрузки», который требует повторного входа. Длинные сессии удобны, но снижают безопасность при потере устройства.","settings.session.btn":"Сохранить таймер автовыхода","settings.session.info.reboot":"Сессии будут истекать при перезагрузке устройства.","settings.session.info.1h":"Сессии будут истекать через 1 час.","settings.session.info.6h":"Сессии будут истекать через 6 часов.","settings.session.info.24h":"Сессии будут истекать через 24 часа.","settings.session.info.3d":"Сессии будут истекать через 3 дня.","settings.session.info.fallback":"Длительность сессии обновлена.","settings.session.err":"Не удалось обновить длительность сессии.","settings.system.title":"Система","settings.system.reboot":"Перезагрузить устройство","settings.system.reboot.web":"Перезагрузить с веб-сервером","settings.system.ble.clear":"Очистить BLE-клиенты","settings.system.logout":"Выйти","settings.system.ble.cleared":"BLE-клиенты успешно очищены!","settings.system.rebooting.web":"Перезагрузка с включённым веб-сервером...","settings.system.rebooting":"Перезагрузка...","system.shutdown.title":"Сессия истекает","system.shutdown.btn":"Продолжить сессию","system.shutdown.shutdown.title":"Сервер отключается","system.shutdown.shutdown.desc":"Сервер отключился из-за неактивности. Для доступа перезагрузите устройство.","passwords.modal.password":"Пароль:","passwords.export.success":"Экспорт выполнен успешно!","passwords.import.success":"Импорт выполнен успешно!","keys.api.active":"Активен ({s}с осталось)","keys.btn.export":"Экспорт ключей","keys.btn.import":"Импорт ключей","keys.export.title":"Подтверждение экспорта","keys.export.desc":"Введите пароль администратора для шифрования и экспорта данных.","keys.import.title":"Подтверждение импорта","keys.import.desc":"Введите пароль администратора для расшифровки и импорта файла. Существующие данные будут перезаписаны."},de:{"app.title":"Authentifikator-Systemsteuerung","tab.keys":"Schlüssel","tab.passwords":"Passwörter","tab.display":"Anzeige","tab.pin":"PIN","tab.settings":"Einstellungen","keys.title":"Schlüssel verwalten","keys.add.title":"Neuen Schlüssel hinzufügen","keys.add.btn":"Schlüssel hinzufügen","keys.current.title":"Aktuelle Schlüssel","keys.importexport.title":"Schlüssel importieren/exportieren","keys.advanced":"Erweiterte Einstellungen","keys.hotp.sync":"HOTP-Zähler werden zusammen mit TOTP-Schlüsseln synchronisiert","keys.api.status":"API-Status:","keys.api.inactive":"Inaktiv","keys.add.success":"✅ Schlüssel erfolgreich hinzugefügt!","keys.remove.success":"Schlüssel entfernt!","keys.order.saved":"Schlüsselreihenfolge gespeichert!","keys.api.enabled":"API-Zugriff für 5 Minuten aktiviert.","keys.hotp.generated":"✅ HOTP-Code generiert","keys.copied":"TOTP-Code kopiert!","keys.hotp_copied":"HOTP-Code kopiert!","ui.loading":"Laden...","keys.col.name":"Name","keys.col.code":"Code","keys.col.timer":"Timer","keys.col.progress":"Fortschritt","keys.col.actions":"Aktionen","keys.btn.remove":"Entfernen","keys.btn.next":"🔄 Weiter","keys.api.enable":"API aktivieren (5 Min)","keys.field.name":"Name:","keys.field.secret":"Geheimnis (Base32):","keys.field.algorithm":"Algorithmus:","keys.type.totp":"TOTP (zeitbasiert)","keys.type.hotp":"HOTP (zählerbasiert)","keys.digits.6":"6 Ziffern","keys.digits.8":"8 Ziffern","keys.period.30":"30 Sekunden","keys.period.60":"60 Sekunden","keys.field.digits":"Ziffern:","keys.field.period":"Zeitraum:","keys.algo.sha1":"SHA1 (Standard)","keys.api.active":"Aktiv (noch {s}s)","passwords.title":"Passwörter verwalten","passwords.add.title":"Neues Passwort hinzufügen","passwords.field.name":"Name:","passwords.field.password":"Passwort:","passwords.add.btn":"Passwort hinzufügen","passwords.current.title":"Aktuelle Passwörter","passwords.col.name":"Name","passwords.col.actions":"Aktionen","passwords.btn.copy":"Kopieren","passwords.btn.edit":"Bearbeiten","passwords.btn.remove":"Entfernen","passwords.copied":"Passwort kopiert!","passwords.updated":"Passwort aktualisiert!","passwords.removed":"Passwort entfernt!","passwords.added":"Passwort hinzugefügt!","passwords.order.saved":"Passwortreihenfolge gespeichert!","passwords.order.failed":"Fehler beim Speichern der Reihenfolge.","passwords.gen.title":"Passwort-Generator","passwords.gen.desc":"Wählen Sie eine Länge und generieren Sie ein sicheres Passwort","passwords.gen.generated":"Generiertes Passwort:","passwords.gen.btn.generate":"Generieren","passwords.gen.btn.use":"Dieses Passwort verwenden","passwords.edit.title":"Passwort bearbeiten","passwords.edit.desc":"Name und Passwort für diesen Eintrag ändern","passwords.edit.name":"Name:","passwords.edit.password":"Passwort:","passwords.edit.btn.save":"Speichern","passwords.edit.btn.cancel":"Abbrechen","passwords.strength.weak":"Schwaches Passwort","passwords.strength.medium":"Mittleres Passwort","passwords.strength.strong":"Starkes Passwort","passwords.strength.encryption":"Verschlüsselungsschlüssel","passwords.importexport.title":"Passwörter importieren/exportieren","passwords.btn.export":"Passwörter exportieren","passwords.btn.import":"Passwörter importieren","passwords.gen.length":"Passwortlänge:","passwords.modal.confirm":"Bestätigen","passwords.export.title":"Export bestätigen","passwords.export.desc":"Geben Sie Ihr Admin-Passwort ein, um die Passwörter zu verschlüsseln und zu exportieren.","passwords.import.title":"Import bestätigen","passwords.import.desc":"Geben Sie Ihr Admin-Passwort ein, um die Passwörter zu entschlüsseln und zu importieren.","system.shutdown.desc":"Der Webserver wird aufgrund von Inaktivität automatisch heruntergefahren.","system.shutdown.remaining":"Verbleibende Zeit:","display.title":"Anzeigeeinstellungen","display.theme.saved":"Design erfolgreich übernommen!","display.theme.title":"Themenauswahl","display.theme.light":"Helles Design","display.theme.dark":"Dunkles Design","display.theme.btn":"Design anwenden","display.splash.title":"Startbildschirm","display.splash.label":"Eingebetteter Startbildschirm-Modus:","display.splash.disabled":"Deaktiviert (kein Startbildschirm)","display.splash.btn":"Modus speichern","display.splash.saved":"Startbildschirm-Modus gespeichert! Neustart erforderlich.","display.splash.select":"Bitte Startbildschirm-Modus auswählen","display.clock.title":"Uhrzeitanzeige","display.clock.timezone":"Zeitzone:","display.clock.save.tz":"Zeitzone speichern","display.clock.tz.saved":"Zeitzone gespeichert! Sofort angewendet.","display.clock.tz.select":"Bitte Zeitzone auswählen","display.rtc.title":"DS3231-RTC-Modul","display.rtc.desc":"Hardwareuhr für Offline-/AP-Modus","display.rtc.sda":"SDA-Pin","display.rtc.scl":"SCL-Pin","display.rtc.btn":"Sync & Speichern","display.rtc.saving":"Speichern...","display.rtc.saved":"RTC-Einstellungen gespeichert","display.rtc.status.notfound":"Nicht gefunden","display.rtc.status.check":"I2C-Pins prüfen","display.rtc.status.drifted":"Zeitdrift","display.rtc.status.ok":"OK","display.rtc.hint.ntp":"Die Zeit wird beim nächsten WiFi-Start automatisch mit NTP synchronisiert.","display.rtc.hint.browser":"Die Zeit wird beim Speichern mit der Browseruhr synchronisiert.","display.lock.title":"Bildschirm & Sperre","display.lock.screen.label":"Bildschirm-Timeout (Display ausschalten nach):","display.lock.auto.label":"Automatische Sperre (Tiefschlaf + PIN nach):","display.lock.warning":"Die automatische Sperre muss länger als der Bildschirm-Timeout sein, wenn beide aktiviert sind.","display.lock.btn":"Einstellungen speichern","display.lock.saved":"Anzeigeeinstellungen erfolgreich gespeichert!","display.lock.error.order":"Der Auto-Sperr-Timeout muss größer als der Bildschirm-Timeout sein!","display.lock.never":"Nie","display.lock.opt.15s":"15 Sekunden","display.lock.opt.30s":"30 Sekunden","display.lock.opt.1m":"1 Minute","display.lock.opt.5m":"5 Minuten","display.lock.opt.15m":"15 Minuten","display.lock.opt.30m":"30 Minuten","display.lock.opt.1h":"1 Stunde","display.lock.opt.4h":"4 Stunden","pin.title":"PIN-Code-Einstellungen","pin.startup.label":"Start-PIN","pin.startup.desc":"Geräteschlüssel beim Start mit PIN verschlüsseln","pin.startup.btn":"Start-PIN-Einstellungen speichern","pin.cancelled":"PIN-Änderung abgebrochen.","pin.reset.cancelled":"Zurücksetzen abgebrochen.","pin.rebooting":"Gerät wird neu gestartet... Bitte warten.","pin.ble.title":"BLE-Kopplungs-PIN","pin.ble.notice":"Dieser PIN wird beim BLE-Koppeln zur Client-Authentifizierung auf dem Gerät angezeigt.","pin.ble.label.new":"Neuer BLE-Kopplungs-PIN (6 Ziffern):","pin.ble.label.confirm":"BLE-Kopplungs-PIN bestätigen:","pin.ble.important":"Dieser PIN wird beim BLE-Koppeln auf dem ESP32-Bildschirm zur Eingabe durch den Client angezeigt.","pin.ble.btn":"BLE-Kopplungs-PIN aktualisieren","pin.ble.saved":"BLE-Kopplungs-PIN aktualisiert! Alle BLE-Clients getrennt.","pin.ble.error.digits":"BLE-Kopplungs-PIN muss genau 6 Ziffern haben!","pin.ble.error.match":"BLE-Kopplungs-PINs stimmen nicht überein!","pin.placeholder.enter":"6-stelligen PIN eingeben","pin.placeholder.confirm":"6-stelligen PIN bestätigen","pin.device.title":"Geräte-BLE-PIN","pin.device.notice":"Dieser PIN wird auf dem Gerät beim Übertragen von Passwörtern über BLE benötigt.","pin.device.label.enable":"Geräte-BLE-PIN aktivieren","pin.device.desc":"PIN auf dem Gerät für BLE-Passwortübertragung erforderlich","pin.device.label.new":"Neuer Geräte-BLE-PIN (6 Ziffern):","pin.device.label.confirm":"Geräte-BLE-PIN bestätigen:","pin.device.important":"Dieser PIN wird auf dem Gerät beim Übertragen von Passwörtern über BLE benötigt.","pin.device.btn":"Geräte-BLE-PIN speichern","pin.device.saved":"Geräte-BLE-PIN aktualisiert!","pin.device.disabled":"Geräte-BLE-PIN deaktiviert","pin.device.error.enable":"Bitte zuerst Geräte-BLE-PIN aktivieren","pin.device.error.digits":"Geräte-BLE-PIN muss genau 6 Ziffern haben!","pin.device.error.match":"Geräte-BLE-PINs stimmen nicht überein!","settings.title":"Geräteeinstellungen","settings.pwd.title":"Passwortverwaltung","settings.pwd.toggle.web":"Web-Kabinett","settings.pwd.toggle.wifi":"WLAN-Zugangspunkt","settings.pwd.web.form.title":"Web-Kabinett-Passwort ändern","settings.pwd.web.desc":"Passwort für den Zugriff auf diese Weboberfläche ändern.","settings.pwd.current.login":"Aktuelle Anmeldung:","settings.pwd.web.new.label":"Neues Web-Passwort","settings.pwd.web.confirm.label":"Neues Web-Passwort bestätigen","settings.pwd.req.length":"Mindestens 8 Zeichen","settings.pwd.req.upper":"Ein Großbuchstabe","settings.pwd.req.lower":"Ein Kleinbuchstabe","settings.pwd.req.number":"Eine Zahl","settings.pwd.req.special":"Ein Sonderzeichen (!@#$%)","settings.pwd.web.btn":"Passwort ändern","settings.pwd.wifi.form.title":"WLAN-Zugangspunkt-Passwort ändern","settings.pwd.wifi.desc":"Passwort für den WLAN-Zugangspunkt im AP-Modus ändern.","settings.pwd.wifi.new.label":"Neues WLAN-AP-Passwort","settings.pwd.wifi.confirm.label":"Neues WLAN-AP-Passwort bestätigen","settings.pwd.wifi.btn":"WLAN-Passwort ändern","settings.pwd.err.match":"Passwörter stimmen nicht überein!","settings.pwd.err.requirements":"Passwort erfüllt die Anforderungen nicht!","settings.pwd.err.wifi.short":"WLAN-Passwort muss mindestens 8 Zeichen haben!","settings.pwd.web.saved":"Passwort erfolgreich geändert!","settings.pwd.wifi.saved":"AP-Passwort geändert. Gerät wird neu gestartet.","settings.pwd.wifi.restarted":"Gerät neu gestartet. Mit neuem Passwort mit WLAN-AP verbinden.","settings.pwd.match":"Passwörter stimmen überein!","settings.ble.title":"Bluetooth-Einstellungen","settings.ble.label":"BLE-Gerätename (max. 15 Zeichen):","settings.ble.btn":"BLE-Name speichern","settings.ble.saved":"BLE-Gerätename aktualisiert!","settings.mdns.title":"mDNS-Einstellungen","settings.mdns.label":"mDNS-Hostname (z.B. 't-disp-totp'):","settings.mdns.btn":"mDNS-Hostname speichern","settings.mdns.saved":"mDNS-Hostname aktualisiert!","settings.wifi.title":"📶 WLAN-Netzwerk","settings.wifi.ssid.label":"Netzwerk-SSID","settings.wifi.ssid.placeholder":"WLAN-Netzwerkname","settings.wifi.pwd.label":"Passwort","settings.wifi.pwd.placeholder":"WLAN-Passwort (min. 8 Zeichen)","settings.wifi.confirm.label":"Passwort bestätigen","settings.wifi.confirm.placeholder":"Passwort wiederholen","settings.wifi.btn":"WLAN-Zugangsdaten speichern","settings.wifi.note":"⚠ Änderungen werden nach dem Neustart angewendet.","settings.wifi.err.ssid":"SSID ist erforderlich.","settings.wifi.err.match":"Passwörter stimmen nicht überein.","settings.wifi.err.short":"Passwort muss mindestens 8 Zeichen haben.","settings.wifi.err.failed":"Anfrage fehlgeschlagen.","settings.wifi.saved":"WLAN-Zugangsdaten gespeichert!","settings.scan.scanning":"⏳ Scannen...","settings.scan.btn":"📡 Scannen","settings.scan.failed":"Scan fehlgeschlagen","settings.startup.title":"Startmodus","settings.startup.label":"Standardmodus beim Start:","settings.startup.opt.totp":"TOTP-Authentifikator","settings.startup.opt.pwd":"Passwort-Manager","settings.startup.btn":"Startmodus speichern","settings.startup.saved":"Startmodus erfolgreich gespeichert!","settings.boot.title":"Boot-Modus","settings.boot.label":"Standard-Netzwerkmodus beim Booten:","settings.boot.opt.wifi":"WLAN-Modus","settings.boot.opt.ap":"AP-Modus","settings.boot.opt.offline":"Offline-Modus","settings.boot.note":"Überschreibung über Tasten beim Booten möglich (2-Sek.-Fenster).","settings.boot.btn":"Boot-Modus speichern","settings.boot.saving":"Speichern...","settings.boot.saved":"Boot-Modus gespeichert! Wird beim nächsten Neustart angewendet.","settings.webserver.title":"Webserver","settings.webserver.label":"Automatisches Herunterfahren bei Inaktivität:","settings.webserver.opt.5m":"5 Minuten","settings.webserver.opt.10m":"10 Minuten","settings.webserver.opt.1h":"1 Stunde","settings.webserver.opt.never":"Nie","settings.webserver.btn":"Einstellung speichern","settings.webserver.saving":"Einstellungen werden gespeichert und Gerät neu gestartet...","settings.session.title":"Auto-Abmelde-Timer","settings.session.label":"Angemeldete Sitzungsdauer:","settings.session.opt.reboot":"Bis zum Geräte-Neustart","settings.session.opt.1h":"1 Stunde","settings.session.opt.6h":"6 Stunden (Standard)","settings.session.opt.24h":"24 Stunden","settings.session.opt.3d":"3 Tage","settings.session.notice":"Steuert die automatische Abmeldezeit für erhöhte Sicherheit. Sitzungen überleben Neustarts, außer im Modus «Bis Neustart», der nach dem Einschalten eine neue Anmeldung erfordert. Längere Sitzungen reduzieren die Anmeldefrequenz, können aber die Sicherheit bei Geräteverlust gefährden.","settings.session.btn":"Auto-Abmelde-Timer speichern","settings.session.info.reboot":"Sitzungen laufen jetzt beim Geräte-Neustart ab.","settings.session.info.1h":"Sitzungen laufen jetzt nach 1 Stunde ab.","settings.session.info.6h":"Sitzungen laufen jetzt nach 6 Stunden ab.","settings.session.info.24h":"Sitzungen laufen jetzt nach 24 Stunden ab.","settings.session.info.3d":"Sitzungen laufen jetzt nach 3 Tagen ab.","settings.session.info.fallback":"Sitzungsdauer aktualisiert.","settings.session.err":"Sitzungsdauer konnte nicht aktualisiert werden.","settings.system.title":"System","settings.system.reboot":"Gerät neu starten","settings.system.reboot.web":"Mit Webserver neu starten","settings.system.ble.clear":"BLE-Clients löschen","settings.system.logout":"Abmelden","settings.system.ble.cleared":"BLE-Clients erfolgreich gelöscht!","settings.system.rebooting.web":"Neustart mit aktiviertem Webserver...","settings.system.rebooting":"Neustart...","system.shutdown.title":"Sitzungs-Timeout","system.shutdown.btn":"Sitzung fortsetzen","system.shutdown.shutdown.title":"Server wird heruntergefahren","system.shutdown.shutdown.desc":"Der Server wurde aufgrund von Inaktivität heruntergefahren. Bitte starten Sie das Gerät neu.","passwords.modal.password":"Passwort:","passwords.export.success":"Export erfolgreich!","passwords.import.success":"Import erfolgreich!","keys.btn.export":"Schlüssel exportieren","keys.btn.import":"Schlüssel importieren","keys.export.title":"Export bestätigen","keys.export.desc":"Geben Sie Ihr Admin-Passwort ein, um die Daten zu verschlüsseln und zu exportieren.","keys.import.title":"Import bestätigen","keys.import.desc":"Geben Sie Ihr Admin-Passwort ein, um die Datei zu entschlüsseln und zu importieren. Bestehende Daten werden überschrieben."},zh:{"app.title":"验证器控制面板","tab.keys":"密钥","tab.passwords":"密码","tab.display":"显示","tab.pin":"PIN码","tab.settings":"设置","keys.title":"管理密钥","keys.add.title":"添加新密钥","keys.add.btn":"添加密钥","keys.current.title":"当前密钥","keys.importexport.title":"导入/导出密钥","keys.advanced":"高级设置","keys.hotp.sync":"HOTP计数器与TOTP密钥同步","keys.api.status":"API状态:","keys.api.inactive":"未激活","keys.add.success":"✅ 密钥添加成功！","keys.remove.success":"密钥已删除！","keys.order.saved":"密钥顺序已保存！","keys.api.enabled":"API访问已开放5分钟。","keys.hotp.generated":"✅ 已生成HOTP代码","keys.copied":"TOTP代码已复制！","keys.hotp_copied":"HOTP代码已复制！","ui.loading":"加载中...","keys.col.name":"名称","keys.col.code":"代码","keys.col.timer":"计时器","keys.col.progress":"进度","keys.col.actions":"操作","keys.btn.remove":"删除","keys.btn.next":"🔄 下一个","keys.api.enable":"启用API (5分钟)","keys.field.name":"名称:","keys.field.secret":"密钥 (Base32):","keys.field.algorithm":"算法:","keys.type.totp":"TOTP (基于时间)","keys.type.hotp":"HOTP (基于计数器)","keys.digits.6":"6位数字","keys.digits.8":"8位数字","keys.period.30":"30秒","keys.period.60":"60秒","keys.field.digits":"位数:","keys.field.period":"周期:","keys.algo.sha1":"SHA1 (标准)","passwords.title":"管理密码","passwords.add.title":"添加新密码","passwords.field.name":"名称:","passwords.field.password":"密码:","passwords.add.btn":"添加密码","passwords.current.title":"当前密码","passwords.col.name":"名称","passwords.col.actions":"操作","passwords.btn.copy":"复制","passwords.btn.edit":"编辑","passwords.btn.remove":"删除","passwords.copied":"密码已复制！","passwords.updated":"密码已更新！","passwords.removed":"密码已删除！","passwords.added":"密码已添加！","passwords.order.saved":"密码顺序已保存！","passwords.order.failed":"保存密码顺序失败。","passwords.gen.title":"密码生成器","passwords.gen.desc":"选择密码长度并生成安全密码","passwords.gen.generated":"生成的密码:","passwords.gen.btn.generate":"立即生成","passwords.gen.btn.use":"使用此密码","passwords.edit.title":"编辑密码","passwords.edit.desc":"更改此条目的名称和密码","passwords.edit.name":"名称:","passwords.edit.password":"密码:","passwords.edit.btn.save":"保存","passwords.edit.btn.cancel":"取消","passwords.strength.weak":"弱密码","passwords.strength.medium":"中等密码","passwords.strength.strong":"强密码","passwords.strength.encryption":"加密密钥","passwords.importexport.title":"导入/导出密码","passwords.btn.export":"导出密码","passwords.btn.import":"导入密码","passwords.gen.length":"密码长度:","passwords.modal.confirm":"确认","passwords.export.title":"确认导出","passwords.export.desc":"请输入管理员密码以加密并导出密码。","passwords.import.title":"确认导入","passwords.import.desc":"请输入管理员密码以解密并导入密码。","system.shutdown.desc":"由于不活动，Web服务器将自动关闭。","system.shutdown.remaining":"剩余时间:","display.title":"显示设置","display.theme.saved":"主题已成功应用！","display.theme.title":"主题选择","display.theme.light":"浅色主题","display.theme.dark":"深色主题","display.theme.btn":"应用主题","display.splash.title":"启动画面","display.splash.label":"内置启动画面模式:","display.splash.disabled":"禁用（无启动画面）","display.splash.btn":"保存模式","display.splash.saved":"启动画面模式已保存！重启后生效。","display.splash.select":"请选择启动画面模式","display.clock.title":"时钟显示","display.clock.timezone":"时区:","display.clock.save.tz":"保存时区","display.clock.tz.saved":"时区已保存！立即生效。","display.clock.tz.select":"请选择时区","display.rtc.title":"DS3231 RTC模块","display.rtc.desc":"用于离线/AP模式的硬件时钟","display.rtc.sda":"SDA引脚","display.rtc.scl":"SCL引脚","display.rtc.btn":"同步并保存","display.rtc.saving":"保存中...","display.rtc.saved":"RTC设置已保存","display.rtc.status.notfound":"未找到","display.rtc.status.check":"检查I2C引脚","display.rtc.status.drifted":"时间偏差","display.rtc.status.ok":"正常","display.rtc.hint.ntp":"下次WiFi启动时将自动从NTP同步时间。","display.rtc.hint.browser":"保存时将从浏览器时钟同步时间。","display.lock.title":"屏幕与锁定","display.lock.screen.label":"屏幕超时（关闭显示后）：","display.lock.auto.label":"自动锁定（深度睡眠+PIN后）：","display.lock.warning":"当两者都启用时，自动锁定必须长于屏幕超时。","display.lock.btn":"保存设置","display.lock.saved":"显示设置保存成功！","display.lock.error.order":"自动锁定超时必须大于屏幕超时！","display.lock.never":"从不","display.lock.opt.15s":"15秒","display.lock.opt.30s":"30秒","display.lock.opt.1m":"1分钟","display.lock.opt.5m":"5分钟","display.lock.opt.15m":"15分钟","display.lock.opt.30m":"30分钟","display.lock.opt.1h":"1小时","display.lock.opt.4h":"4小时","pin.title":"PIN码设置","pin.startup.label":"启动PIN","pin.startup.desc":"启动时使用PIN加密设备密钥","pin.startup.btn":"保存启动PIN设置","pin.cancelled":"PIN设置更改已取消。","pin.reset.cancelled":"重置已取消。","pin.rebooting":"设备正在重启... 请稍候。","pin.ble.title":"BLE配对PIN","pin.ble.notice":"此PIN在BLE配对时显示在设备屏幕上用于客户端验证。","pin.ble.label.new":"新BLE配对PIN（6位数字）：","pin.ble.label.confirm":"确认BLE配对PIN：","pin.ble.important":"此PIN将在BLE配对时显示在ESP32屏幕上供客户端输入。","pin.ble.btn":"更新BLE配对PIN","pin.ble.saved":"BLE配对PIN已更新！所有BLE客户端已断开。","pin.ble.error.digits":"BLE配对PIN必须恰好6位数字！","pin.ble.error.match":"BLE配对PIN不匹配！","pin.placeholder.enter":"输入6位PIN","pin.placeholder.confirm":"确认6位PIN","pin.device.title":"设备BLE PIN","pin.device.notice":"通过BLE传输密码时需要在硬件设备上输入此PIN。","pin.device.label.enable":"启用设备BLE PIN","pin.device.desc":"通过BLE传输密码时需要在设备上输入PIN","pin.device.label.new":"新设备BLE PIN（6位数字）：","pin.device.label.confirm":"确认设备BLE PIN：","pin.device.important":"通过BLE传输密码时需要在设备屏幕上输入此PIN。","pin.device.btn":"保存设备BLE PIN","pin.device.saved":"设备BLE PIN已更新！","pin.device.disabled":"设备BLE PIN已禁用","pin.device.error.enable":"请先启用设备BLE PIN","pin.device.error.digits":"设备BLE PIN必须恰好6位数字！","pin.device.error.match":"设备BLE PIN不匹配！","settings.title":"设备设置","settings.pwd.title":"密码管理","settings.pwd.toggle.web":"Web控制台","settings.pwd.toggle.wifi":"WiFi热点","settings.pwd.web.form.title":"更改Web控制台密码","settings.pwd.web.desc":"更改访问此Web界面的密码。","settings.pwd.current.login":"当前登录:","settings.pwd.web.new.label":"新Web密码","settings.pwd.web.confirm.label":"确认新Web密码","settings.pwd.req.length":"至少8个字符","settings.pwd.req.upper":"一个大写字母","settings.pwd.req.lower":"一个小写字母","settings.pwd.req.number":"一个数字","settings.pwd.req.special":"一个特殊字符 (!@#$%)","settings.pwd.web.btn":"更改密码","settings.pwd.wifi.form.title":"更改WiFi热点密码","settings.pwd.wifi.desc":"更改AP模式下WiFi热点的密码。","settings.pwd.wifi.new.label":"新WiFi AP密码","settings.pwd.wifi.confirm.label":"确认新WiFi AP密码","settings.pwd.wifi.btn":"更改WiFi密码","settings.pwd.err.match":"密码不匹配！","settings.pwd.err.requirements":"密码不符合要求！","settings.pwd.err.wifi.short":"WiFi密码至少需要8个字符！","settings.pwd.web.saved":"密码更改成功！","settings.pwd.wifi.saved":"AP密码已更改，设备正在重启。","settings.pwd.wifi.restarted":"设备已重启，请使用新密码连接WiFi热点。","settings.pwd.match":"密码匹配！","settings.ble.title":"蓝牙设置","settings.ble.label":"BLE设备名称（最多15个字符）：","settings.ble.btn":"保存BLE名称","settings.ble.saved":"BLE设备名称已更新！","settings.mdns.title":"mDNS设置","settings.mdns.label":"mDNS主机名（例如 't-disp-totp'）：","settings.mdns.btn":"保存mDNS主机名","settings.mdns.saved":"mDNS主机名已更新！","settings.wifi.title":"📶 WiFi网络","settings.wifi.ssid.label":"网络SSID","settings.wifi.ssid.placeholder":"WiFi网络名称","settings.wifi.pwd.label":"密码","settings.wifi.pwd.placeholder":"WiFi密码（最少8个字符）","settings.wifi.confirm.label":"确认密码","settings.wifi.confirm.placeholder":"重复密码","settings.wifi.btn":"保存WiFi凭据","settings.wifi.note":"⚠ 更改将在重启后生效。","settings.wifi.err.ssid":"SSID为必填项。","settings.wifi.err.match":"密码不匹配。","settings.wifi.err.short":"密码至少需要8个字符。","settings.wifi.err.failed":"请求失败。","settings.wifi.saved":"WiFi凭据已保存！","settings.scan.scanning":"⏳ 扫描中...","settings.scan.btn":"📡 扫描","settings.scan.failed":"扫描失败","settings.startup.title":"启动模式","settings.startup.label":"启动时的默认模式：","settings.startup.opt.totp":"TOTP验证器","settings.startup.opt.pwd":"密码管理器","settings.startup.btn":"保存启动模式","settings.startup.saved":"启动模式保存成功！","settings.boot.title":"引导模式","settings.boot.label":"引导时的默认网络模式：","settings.boot.opt.wifi":"WiFi模式","settings.boot.opt.ap":"AP模式","settings.boot.opt.offline":"离线模式","settings.boot.note":"可在引导时通过按钮覆盖（2秒窗口）。","settings.boot.btn":"保存引导模式","settings.boot.saving":"保存中...","settings.boot.saved":"引导模式已保存！将在下次重启时生效。","settings.webserver.title":"Web服务器","settings.webserver.label":"不活动时自动关闭：","settings.webserver.opt.5m":"5分钟","settings.webserver.opt.10m":"10分钟","settings.webserver.opt.1h":"1小时","settings.webserver.opt.never":"从不","settings.webserver.btn":"保存设置","settings.webserver.saving":"正在保存设置并重启设备...","settings.session.title":"自动注销计时器","settings.session.label":"登录保持时长：","settings.session.opt.reboot":"直到设备重启","settings.session.opt.1h":"1小时","settings.session.opt.6h":"6小时（默认）","settings.session.opt.24h":"24小时","settings.session.opt.3d":"3天","settings.session.notice":"控制自动注销时间以增强安全性。会话在设备重启后继续有效，但「直到重启」模式除外，该模式在断电后需要重新登录。较长的持续时间可减少登录频率，但设备丢失时可能降低安全性。","settings.session.btn":"保存自动注销计时器","settings.session.info.reboot":"会话现在将在设备重启时过期。","settings.session.info.1h":"会话现在将在1小时后过期。","settings.session.info.6h":"会话现在将在6小时后过期。","settings.session.info.24h":"会话现在将在24小时后过期。","settings.session.info.3d":"会话现在将在3天后过期。","settings.session.info.fallback":"会话时长已更新。","settings.session.err":"无法更新会话时长。","settings.system.title":"系统","settings.system.reboot":"重启设备","settings.system.reboot.web":"重启并启用Web服务器","settings.system.ble.clear":"清除BLE客户端","settings.system.logout":"退出登录","settings.system.ble.cleared":"BLE客户端已成功清除！","settings.system.rebooting.web":"正在重启并启用Web服务器...","settings.system.rebooting":"正在重启...","system.shutdown.title":"会话超时","system.shutdown.btn":"继续会话","system.shutdown.shutdown.title":"服务器正在关闭","system.shutdown.shutdown.desc":"服务器因不活动已关闭。请重启设备以重新访问。","passwords.modal.password":"密码:","passwords.export.success":"导出成功！","passwords.import.success":"导入成功！","keys.api.active":"活跃 (剩余{s}秒)","keys.btn.export":"导出密钥","keys.btn.import":"导入密钥","keys.export.title":"确认导出","keys.export.desc":"请输入管理员密码以加密并导出数据。","keys.import.title":"确认导入","keys.import.desc":"请输入管理员密码以解密并导入所选文件。现有数据将被覆盖。"},es:{"app.title":"Panel de control del autenticador","tab.keys":"Claves","tab.passwords":"Contraseñas","tab.display":"Pantalla","tab.pin":"PIN","tab.settings":"Ajustes","keys.title":"Gestionar claves","keys.add.title":"Añadir nueva clave","keys.add.btn":"Añadir clave","keys.current.title":"Claves actuales","keys.importexport.title":"Importar/Exportar claves","keys.advanced":"Configuración avanzada","keys.hotp.sync":"Los contadores HOTP se sincronizan junto con las claves TOTP","keys.api.status":"Estado API:","keys.api.inactive":"Inactivo","keys.add.success":"✅ ¡Clave añadida correctamente!","keys.remove.success":"¡Clave eliminada!","keys.order.saved":"¡Orden de claves guardado!","keys.api.enabled":"Acceso API habilitado por 5 minutos.","keys.hotp.generated":"✅ Código HOTP generado","keys.copied":"¡Código TOTP copiado!","keys.hotp_copied":"¡Código HOTP copiado!","ui.loading":"Cargando...","keys.col.name":"Nombre","keys.col.code":"Código","keys.col.timer":"Temporizador","keys.col.progress":"Progreso","keys.col.actions":"Acciones","keys.btn.remove":"Eliminar","keys.btn.next":"🔄 Siguiente","keys.api.enable":"Activar API (5 min)","keys.field.name":"Nombre:","keys.field.secret":"Secreto (Base32):","keys.field.algorithm":"Algoritmo:","keys.type.totp":"TOTP (basado en tiempo)","keys.type.hotp":"HOTP (basado en contador)","keys.digits.6":"6 dígitos","keys.digits.8":"8 dígitos","keys.period.30":"30 segundos","keys.period.60":"60 segundos","keys.field.digits":"Dígitos:","keys.field.period":"Período:","keys.algo.sha1":"SHA1 (Estándar)","passwords.title":"Gestionar contraseñas","passwords.add.title":"Añadir nueva contraseña","passwords.field.name":"Nombre:","passwords.field.password":"Contraseña:","passwords.add.btn":"Añadir contraseña","passwords.current.title":"Contraseñas actuales","passwords.col.name":"Nombre","passwords.col.actions":"Acciones","passwords.btn.copy":"Copiar","passwords.btn.edit":"Editar","passwords.btn.remove":"Eliminar","passwords.copied":"¡Contraseña copiada!","passwords.updated":"¡Contraseña actualizada!","passwords.removed":"¡Contraseña eliminada!","passwords.added":"¡Contraseña añadida!","passwords.order.saved":"¡Orden de contraseñas guardado!","passwords.order.failed":"Error al guardar el orden.","passwords.gen.title":"Generador de contraseñas","passwords.gen.desc":"Elige la longitud y genera una contraseña segura","passwords.gen.generated":"Contraseña generada:","passwords.gen.btn.generate":"Generar ahora","passwords.gen.btn.use":"Usar esta contraseña","passwords.edit.title":"Editar contraseña","passwords.edit.desc":"Cambiar el nombre y la contraseña de esta entrada","passwords.edit.name":"Nombre:","passwords.edit.password":"Contraseña:","passwords.edit.btn.save":"Guardar","passwords.edit.btn.cancel":"Cancelar","passwords.strength.weak":"Contraseña débil","passwords.strength.medium":"Contraseña media","passwords.strength.strong":"Contraseña fuerte","passwords.strength.encryption":"Clave de cifrado","passwords.importexport.title":"Importar/Exportar contraseñas","passwords.btn.export":"Exportar contraseñas","passwords.btn.import":"Importar contraseñas","passwords.gen.length":"Longitud de contraseña:","passwords.modal.confirm":"Confirmar","passwords.export.title":"Confirmar exportación","passwords.export.desc":"Ingrese su contraseña de administrador para cifrar y exportar las contraseñas.","passwords.import.title":"Confirmar importación","passwords.import.desc":"Ingrese su contraseña de administrador para descifrar e importar las contraseñas.","system.shutdown.desc":"El servidor web se apagará automáticamente por inactividad.","system.shutdown.remaining":"Tiempo restante:","display.title":"Configuración de pantalla","display.theme.saved":"¡Tema aplicado correctamente!","display.theme.title":"Selección de tema","display.theme.light":"Tema claro","display.theme.dark":"Tema oscuro","display.theme.btn":"Aplicar tema","display.splash.title":"Pantalla de inicio","display.splash.label":"Modo de pantalla de inicio integrada:","display.splash.disabled":"Desactivado (sin pantalla de inicio)","display.splash.btn":"Guardar modo","display.splash.saved":"¡Modo de pantalla guardado! Reinicie para aplicar.","display.splash.select":"Por favor, seleccione un modo de pantalla","display.clock.title":"Reloj","display.clock.timezone":"Zona horaria:","display.clock.save.tz":"Guardar zona horaria","display.clock.tz.saved":"¡Zona horaria guardada! Aplicada inmediatamente.","display.clock.tz.select":"Por favor, seleccione una zona horaria","display.rtc.title":"Módulo RTC DS3231","display.rtc.desc":"Reloj hardware para modo sin conexión/AP","display.rtc.sda":"Pin SDA","display.rtc.scl":"Pin SCL","display.rtc.btn":"Sincronizar y guardar","display.rtc.saving":"Guardando...","display.rtc.saved":"Configuración RTC guardada","display.rtc.status.notfound":"No encontrado","display.rtc.status.check":"Verificar pines I2C","display.rtc.status.drifted":"Deriva de tiempo","display.rtc.status.ok":"OK","display.rtc.hint.ntp":"La hora se sincronizará desde NTP automáticamente en el próximo arranque con WiFi.","display.rtc.hint.browser":"La hora se sincronizará desde el reloj del navegador cuando guarde.","display.lock.title":"Pantalla y bloqueo","display.lock.screen.label":"Tiempo de espera de pantalla (apagar pantalla después de):","display.lock.auto.label":"Bloqueo automático (sueño profundo + PIN requerido después de):","display.lock.warning":"El bloqueo automático debe ser mayor que el tiempo de espera de pantalla cuando ambos están habilitados.","display.lock.btn":"Guardar configuración","display.lock.saved":"¡Configuración de pantalla guardada correctamente!","display.lock.error.order":"¡El tiempo de bloqueo automático debe ser mayor que el tiempo de espera de pantalla!","display.lock.never":"Nunca","display.lock.opt.15s":"15 segundos","display.lock.opt.30s":"30 segundos","display.lock.opt.1m":"1 minuto","display.lock.opt.5m":"5 minutos","display.lock.opt.15m":"15 minutos","display.lock.opt.30m":"30 minutos","display.lock.opt.1h":"1 hora","display.lock.opt.4h":"4 horas","pin.title":"Configuración de PIN","pin.startup.label":"PIN de inicio","pin.startup.desc":"Cifrar clave del dispositivo con PIN al arrancar","pin.startup.btn":"Guardar configuración de PIN de inicio","pin.cancelled":"Cambio de PIN cancelado.","pin.reset.cancelled":"Restablecimiento cancelado.","pin.rebooting":"El dispositivo se está reiniciando... Por favor espere.","pin.ble.title":"PIN de emparejamiento BLE","pin.ble.notice":"Este PIN se muestra en la pantalla del dispositivo durante el emparejamiento BLE para autenticación del cliente.","pin.ble.label.new":"Nuevo PIN de emparejamiento BLE (6 dígitos):","pin.ble.label.confirm":"Confirmar PIN de emparejamiento BLE:","pin.ble.important":"Este PIN se mostrará en la pantalla ESP32 durante el emparejamiento BLE para que el cliente lo ingrese.","pin.ble.btn":"Actualizar PIN de emparejamiento BLE","pin.ble.saved":"¡PIN de emparejamiento BLE actualizado! Todos los clientes BLE desconectados.","pin.ble.error.digits":"¡El PIN de emparejamiento BLE debe tener exactamente 6 dígitos!","pin.ble.error.match":"¡Los PINs de emparejamiento BLE no coinciden!","pin.placeholder.enter":"Ingresar PIN de 6 dígitos","pin.placeholder.confirm":"Confirmar PIN de 6 dígitos","pin.device.title":"PIN de dispositivo BLE","pin.device.notice":"Este PIN es requerido en el dispositivo al transmitir contraseñas vía BLE.","pin.device.label.enable":"Habilitar PIN de dispositivo BLE","pin.device.desc":"Requerir PIN en el dispositivo para transmisión de contraseñas BLE","pin.device.label.new":"Nuevo PIN de dispositivo BLE (6 dígitos):","pin.device.label.confirm":"Confirmar PIN de dispositivo BLE:","pin.device.important":"Este PIN será requerido en la pantalla del dispositivo al transmitir contraseñas vía BLE.","pin.device.btn":"Guardar PIN de dispositivo BLE","pin.device.saved":"¡PIN de dispositivo BLE actualizado!","pin.device.disabled":"PIN de dispositivo BLE deshabilitado","pin.device.error.enable":"Por favor habilite el PIN de dispositivo BLE primero","pin.device.error.digits":"¡El PIN de dispositivo BLE debe tener exactamente 6 dígitos!","pin.device.error.match":"¡Los PINs de dispositivo BLE no coinciden!","settings.title":"Configuración del dispositivo","settings.pwd.title":"Gestión de contraseñas","settings.pwd.toggle.web":"Gabinete Web","settings.pwd.toggle.wifi":"Punto de acceso WiFi","settings.pwd.web.form.title":"Cambiar contraseña del gabinete web","settings.pwd.web.desc":"Cambiar la contraseña para acceder a esta interfaz web.","settings.pwd.current.login":"Inicio de sesión actual:","settings.pwd.web.new.label":"Nueva contraseña web","settings.pwd.web.confirm.label":"Confirmar nueva contraseña web","settings.pwd.req.length":"Al menos 8 caracteres","settings.pwd.req.upper":"Una letra mayúscula","settings.pwd.req.lower":"Una letra minúscula","settings.pwd.req.number":"Un número","settings.pwd.req.special":"Un carácter especial (!@#$%)","settings.pwd.web.btn":"Cambiar contraseña","settings.pwd.wifi.form.title":"Cambiar contraseña del punto de acceso WiFi","settings.pwd.wifi.desc":"Cambiar la contraseña del punto de acceso WiFi en modo AP.","settings.pwd.wifi.new.label":"Nueva contraseña WiFi AP","settings.pwd.wifi.confirm.label":"Confirmar nueva contraseña WiFi AP","settings.pwd.wifi.btn":"Cambiar contraseña WiFi","settings.pwd.err.match":"¡Las contraseñas no coinciden!","settings.pwd.err.requirements":"¡La contraseña no cumple los requisitos!","settings.pwd.err.wifi.short":"¡La contraseña WiFi debe tener al menos 8 caracteres!","settings.pwd.web.saved":"¡Contraseña cambiada exitosamente!","settings.pwd.wifi.saved":"Contraseña AP cambiada. El dispositivo se reinicia.","settings.pwd.wifi.restarted":"Dispositivo reiniciado. Conéctese al AP WiFi con la nueva contraseña.","settings.pwd.match":"¡Las contraseñas coinciden!","settings.ble.title":"Configuración Bluetooth","settings.ble.label":"Nombre de dispositivo BLE (máx. 15 caracteres):","settings.ble.btn":"Guardar nombre BLE","settings.ble.saved":"¡Nombre de dispositivo BLE actualizado!","settings.mdns.title":"Configuración mDNS","settings.mdns.label":"Nombre de host mDNS (ej., 't-disp-totp'):","settings.mdns.btn":"Guardar nombre de host mDNS","settings.mdns.saved":"¡Nombre de host mDNS actualizado!","settings.wifi.title":"📶 Red WiFi","settings.wifi.ssid.label":"SSID de red","settings.wifi.ssid.placeholder":"Nombre de red WiFi","settings.wifi.pwd.label":"Contraseña","settings.wifi.pwd.placeholder":"Contraseña WiFi (mín. 8 caracteres)","settings.wifi.confirm.label":"Confirmar contraseña","settings.wifi.confirm.placeholder":"Repetir contraseña","settings.wifi.btn":"Guardar credenciales WiFi","settings.wifi.note":"⚠ Los cambios se aplican después del reinicio.","settings.wifi.err.ssid":"El SSID es obligatorio.","settings.wifi.err.match":"Las contraseñas no coinciden.","settings.wifi.err.short":"La contraseña debe tener al menos 8 caracteres.","settings.wifi.err.failed":"Solicitud fallida.","settings.wifi.saved":"¡Credenciales WiFi guardadas!","settings.scan.scanning":"⏳ Escaneando...","settings.scan.btn":"📡 Escanear","settings.scan.failed":"Escaneo fallido","settings.startup.title":"Modo de inicio","settings.startup.label":"Modo predeterminado al iniciar:","settings.startup.opt.totp":"Autenticador TOTP","settings.startup.opt.pwd":"Gestor de contraseñas","settings.startup.btn":"Guardar modo de inicio","settings.startup.saved":"¡Modo de inicio guardado correctamente!","settings.boot.title":"Modo de arranque","settings.boot.label":"Modo de red predeterminado al arrancar:","settings.boot.opt.wifi":"Modo WiFi","settings.boot.opt.ap":"Modo AP","settings.boot.opt.offline":"Modo sin conexión","settings.boot.note":"Anulación disponible con botones durante el arranque (ventana de 2 seg).","settings.boot.btn":"Guardar modo de arranque","settings.boot.saving":"Guardando...","settings.boot.saved":"¡Modo de arranque guardado! Se aplicará en el próximo reinicio.","settings.webserver.title":"Servidor web","settings.webserver.label":"Apagado automático por inactividad:","settings.webserver.opt.5m":"5 minutos","settings.webserver.opt.10m":"10 minutos","settings.webserver.opt.1h":"1 hora","settings.webserver.opt.never":"Nunca","settings.webserver.btn":"Guardar ajuste","settings.webserver.saving":"Guardando configuración y reiniciando dispositivo...","settings.session.title":"Temporizador de cierre de sesión","settings.session.label":"Tiempo de sesión activa:","settings.session.opt.reboot":"Hasta el reinicio del dispositivo","settings.session.opt.1h":"1 hora","settings.session.opt.6h":"6 horas (predeterminado)","settings.session.opt.24h":"24 horas","settings.session.opt.3d":"3 días","settings.session.notice":"Controla el tiempo de cierre de sesión automático para mayor seguridad. Las sesiones sobreviven a los reinicios excepto el modo «Hasta reinicio», que requiere inicio de sesión tras apagado. Las duraciones largas reducen la frecuencia de inicio de sesión pero pueden comprometer la seguridad si el dispositivo se pierde.","settings.session.btn":"Guardar temporizador de cierre","settings.session.info.reboot":"Las sesiones ahora expirarán al reiniciar el dispositivo.","settings.session.info.1h":"Las sesiones ahora expirarán después de 1 hora.","settings.session.info.6h":"Las sesiones ahora expirarán después de 6 horas.","settings.session.info.24h":"Las sesiones ahora expirarán después de 24 horas.","settings.session.info.3d":"Las sesiones ahora expirarán después de 3 días.","settings.session.info.fallback":"Duración de sesión actualizada.","settings.session.err":"No se pudo actualizar la duración de la sesión.","settings.system.title":"Sistema","settings.system.reboot":"Reiniciar dispositivo","settings.system.reboot.web":"Reiniciar con servidor web","settings.system.ble.clear":"Borrar clientes BLE","settings.system.logout":"Cerrar sesión","settings.system.ble.cleared":"¡Clientes BLE borrados correctamente!","settings.system.rebooting.web":"Reiniciando con servidor web habilitado...","settings.system.rebooting":"Reiniciando...","system.shutdown.title":"Tiempo de sesión agotado","system.shutdown.btn":"Continuar sesión","system.shutdown.shutdown.title":"Servidor apagándose","system.shutdown.shutdown.desc":"El servidor se apagó por inactividad. Por favor, reinicie el dispositivo para acceder.","passwords.modal.password":"Contraseña:","passwords.export.success":"¡Exportación exitosa!","passwords.import.success":"¡Importación exitosa!","keys.api.active":"Activo ({s}s restantes)","keys.btn.export":"Exportar claves","keys.btn.import":"Importar claves","keys.export.title":"Confirmar exportación","keys.export.desc":"Ingrese su contraseña de administrador para cifrar y exportar los datos.","keys.import.title":"Confirmar importación","keys.import.desc":"Ingrese su contraseña de administrador para descifrar e importar el archivo seleccionado. Los datos existentes serán sobrescritos."},en:{"app.title":"Authenticator Control Panel","tab.keys":"Keys","tab.passwords":"Passwords","tab.display":"Display","tab.pin":"PIN","tab.settings":"Settings","keys.title":"Manage Keys","keys.add.title":"Add New Key","keys.add.btn":"Add Key","keys.current.title":"Current Keys","keys.importexport.title":"Import/Export Keys","keys.advanced":"Advanced Settings","keys.hotp.sync":"HOTP counters are synced together with TOTP keys","keys.api.status":"API Status:","keys.api.inactive":"Inactive","keys.add.success":"✅ Key added successfully!","keys.remove.success":"Key removed successfully!","keys.order.saved":"Keys order saved!","keys.api.enabled":"API access enabled for 5 minutes.","keys.hotp.generated":"✅ HOTP code generated","keys.copied":"TOTP code copied!","keys.hotp_copied":"HOTP code copied!","ui.loading":"Loading...","keys.col.name":"Name","keys.col.code":"Code","keys.col.timer":"Timer","keys.col.progress":"Progress","keys.col.actions":"Actions","keys.btn.remove":"Remove","keys.btn.next":"🔄 Next","keys.api.enable":"Enable API Access (5 min)","keys.field.name":"Name:","keys.field.secret":"Secret (Base32):","keys.field.algorithm":"Algorithm:","keys.type.totp":"TOTP (Time-based)","keys.type.hotp":"HOTP (Counter-based)","keys.digits.6":"6 digits","keys.digits.8":"8 digits","keys.period.30":"30 seconds","keys.period.60":"60 seconds","keys.field.digits":"Digits:","keys.field.period":"Period:","keys.algo.sha1":"SHA1 (Standard)","keys.api.active":"Active ({s}s remaining)","passwords.title":"Manage Passwords","passwords.add.title":"Add New Password","passwords.field.name":"Name:","passwords.field.password":"Password:","passwords.add.btn":"Add Password","passwords.current.title":"Current Passwords","passwords.col.name":"Name","passwords.col.actions":"Actions","passwords.btn.copy":"Copy","passwords.btn.edit":"Edit","passwords.btn.remove":"Remove","passwords.copied":"Password copied to clipboard!","passwords.updated":"Password updated successfully!","passwords.removed":"Password removed successfully!","passwords.added":"Password added successfully!","passwords.order.saved":"Passwords order saved!","passwords.order.failed":"Failed to save passwords order.","passwords.gen.title":"Password Generator","passwords.gen.desc":"Choose password length and generate a secure password","passwords.gen.generated":"Generated Password:","passwords.gen.btn.generate":"Generate New","passwords.gen.btn.use":"Use This Password","passwords.edit.title":"Edit Password","passwords.edit.desc":"Change the name and password for this entry","passwords.edit.name":"Name:","passwords.edit.password":"Password:","passwords.edit.btn.save":"Save","passwords.edit.btn.cancel":"Cancel","passwords.strength.weak":"Weak Password","passwords.strength.medium":"Medium Password","passwords.strength.strong":"Strong Password","passwords.strength.encryption":"Encryption Key","passwords.importexport.title":"Import/Export Passwords","passwords.btn.export":"Export Passwords","passwords.btn.import":"Import Passwords","passwords.gen.length":"Password Length:","passwords.modal.confirm":"Confirm","passwords.export.title":"Confirm Export","passwords.export.desc":"Please enter your web admin password to encrypt and export your passwords.","passwords.import.title":"Confirm Import","passwords.import.desc":"Enter your web admin password to decrypt and import your passwords.","system.shutdown.desc":"The web server will automatically shut down due to inactivity.","system.shutdown.remaining":"Time remaining:","display.title":"Display Settings","display.theme.saved":"Theme updated successfully!","display.theme.title":"Theme Selection","display.theme.light":"Light Theme","display.theme.dark":"Dark Theme","display.theme.btn":"Apply Theme","display.splash.title":"Splash Screen","display.splash.label":"Embedded Splash Mode:","display.splash.disabled":"Disabled (No splash screen)","display.splash.btn":"Save Mode","display.splash.saved":"Splash mode saved! Reboot to apply.","display.splash.select":"Please select a splash mode","display.clock.title":"Clock Display","display.clock.timezone":"Timezone:","display.clock.save.tz":"Save Timezone","display.clock.tz.saved":"Timezone saved! Applied immediately.","display.clock.tz.select":"Please select a timezone","display.rtc.title":"DS3231 RTC Module","display.rtc.desc":"Hardware clock for offline/AP time","display.rtc.sda":"SDA Pin","display.rtc.scl":"SCL Pin","display.rtc.btn":"Sync & Save","display.rtc.saving":"Saving...","display.rtc.saved":"RTC settings saved","display.rtc.status.notfound":"Not Found","display.rtc.status.check":"Check I2C pins","display.rtc.status.drifted":"Drifted","display.rtc.status.ok":"OK","display.rtc.hint.ntp":"Time will be synced from NTP automatically on next WiFi boot.","display.rtc.hint.browser":"Time will be synced from your browser clock when you save.","display.lock.title":"Screen & Lock Settings","display.lock.screen.label":"Screen timeout (turn off display after):","display.lock.auto.label":"Auto lock (deep sleep + PIN required after):","display.lock.warning":"Auto lock must be longer than screen timeout when both are enabled.","display.lock.btn":"Save Settings","display.lock.saved":"Display settings saved successfully!","display.lock.error.order":"Auto lock timeout must be greater than screen timeout!","display.lock.never":"Never","display.lock.opt.15s":"15 seconds","display.lock.opt.30s":"30 seconds","display.lock.opt.1m":"1 minute","display.lock.opt.5m":"5 minutes","display.lock.opt.15m":"15 minutes","display.lock.opt.30m":"30 minutes","display.lock.opt.1h":"1 hour","display.lock.opt.4h":"4 hours","pin.title":"PIN Code Settings","pin.startup.label":"Startup PIN","pin.startup.desc":"Encrypt device key with PIN on startup","pin.startup.btn":"Save Startup PIN Settings","pin.cancelled":"PIN settings change cancelled.","pin.reset.cancelled":"Factory reset cancelled.","pin.rebooting":"Device is rebooting... Please wait.","pin.ble.title":"BLE Bonding PIN","pin.ble.notice":"This PIN is displayed on device screen during BLE pairing for client authentication.","pin.ble.label.new":"New BLE Bonding PIN (6 digits):","pin.ble.label.confirm":"Confirm BLE Bonding PIN:","pin.ble.important":"This PIN will be displayed on the ESP32 screen during BLE pairing for clients to enter.","pin.ble.btn":"Update BLE Bonding PIN","pin.ble.saved":"BLE Bonding PIN updated successfully! All BLE clients cleared.","pin.ble.error.digits":"BLE Bonding PIN must be exactly 6 digits!","pin.ble.error.match":"BLE Bonding PINs do not match!","pin.placeholder.enter":"Enter 6-digit PIN","pin.placeholder.confirm":"Confirm 6-digit PIN","pin.device.title":"Device BLE PIN","pin.device.notice":"This PIN is required on the hardware device when transmitting passwords via BLE.","pin.device.label.enable":"Enable Device BLE PIN","pin.device.desc":"Require PIN on device for BLE password transmission","pin.device.label.new":"New Device BLE PIN (6 digits):","pin.device.label.confirm":"Confirm Device BLE PIN:","pin.device.important":"This PIN will be required on the device screen when transmitting passwords via BLE.","pin.device.btn":"Save Device BLE PIN","pin.device.saved":"Device BLE PIN updated successfully!","pin.device.disabled":"Device BLE PIN disabled","pin.device.error.enable":"Please enable Device BLE PIN first","pin.device.error.digits":"Device BLE PIN must be exactly 6 digits!","pin.device.error.match":"Device BLE PINs do not match!","settings.title":"Device Settings","settings.pwd.title":"Password Management","settings.pwd.toggle.web":"Web Cabinet","settings.pwd.toggle.wifi":"WiFi Access Point","settings.pwd.web.form.title":"Change Web Cabinet Password","settings.pwd.web.desc":"Change the password for accessing this web interface.","settings.pwd.current.login":"Current Login:","settings.pwd.web.new.label":"New Web Password","settings.pwd.web.confirm.label":"Confirm New Web Password","settings.pwd.req.length":"At least 8 characters","settings.pwd.req.upper":"An uppercase letter","settings.pwd.req.lower":"A lowercase letter","settings.pwd.req.number":"A number","settings.pwd.req.special":"A special character (!@#$%)","settings.pwd.web.btn":"Change Password","settings.pwd.wifi.form.title":"Change WiFi Access Point Password","settings.pwd.wifi.desc":"Change the password for the WiFi Access Point used in AP mode.","settings.pwd.wifi.new.label":"New WiFi AP Password","settings.pwd.wifi.confirm.label":"Confirm New WiFi AP Password","settings.pwd.wifi.btn":"Change WiFi Password","settings.pwd.err.match":"Passwords do not match!","settings.pwd.err.requirements":"Password does not meet requirements!","settings.pwd.err.wifi.short":"WiFi password must be at least 8 characters!","settings.pwd.web.saved":"Password changed successfully!","settings.pwd.wifi.saved":"AP password changed. Device is restarting.","settings.pwd.wifi.restarted":"Device restarted. Reconnect to WiFi AP with new password.","settings.pwd.match":"Passwords match!","settings.ble.title":"Bluetooth Settings","settings.ble.label":"BLE Device Name (max 15 chars):","settings.ble.btn":"Save BLE Name","settings.ble.saved":"BLE device name updated!","settings.mdns.title":"mDNS Settings","settings.mdns.label":"mDNS Hostname (e.g., 't-disp-totp'):","settings.mdns.btn":"Save mDNS Hostname","settings.mdns.saved":"mDNS hostname updated!","settings.wifi.title":"📶 WiFi Network","settings.wifi.ssid.label":"Network SSID","settings.wifi.ssid.placeholder":"WiFi network name","settings.wifi.pwd.label":"Password","settings.wifi.pwd.placeholder":"WiFi password (min 8 chars)","settings.wifi.confirm.label":"Confirm Password","settings.wifi.confirm.placeholder":"Repeat password","settings.wifi.btn":"Save WiFi Credentials","settings.wifi.note":"⚠ Changes apply after reboot.","settings.wifi.err.ssid":"SSID is required.","settings.wifi.err.match":"Passwords do not match.","settings.wifi.err.short":"Password must be at least 8 characters.","settings.wifi.err.failed":"Request failed.","settings.wifi.saved":"WiFi credentials saved!","settings.scan.scanning":"⏳ Scanning...","settings.scan.btn":"📡 Scan","settings.scan.failed":"Scan failed","settings.startup.title":"Startup Mode","settings.startup.label":"Default mode on startup:","settings.startup.opt.totp":"TOTP Authenticator","settings.startup.opt.pwd":"Password Manager","settings.startup.btn":"Save Startup Mode","settings.startup.saved":"Startup mode saved successfully!","settings.boot.title":"Boot Mode","settings.boot.label":"Default network mode on boot:","settings.boot.opt.wifi":"WiFi Mode","settings.boot.opt.ap":"AP Mode","settings.boot.opt.offline":"Offline Mode","settings.boot.note":"Override available via buttons during boot (2 sec window).","settings.boot.btn":"Save Boot Mode","settings.boot.saving":"Saving...","settings.boot.saved":"Boot mode saved! Will apply on next reboot.","settings.webserver.title":"Web Server","settings.webserver.label":"Auto-shutdown on inactivity:","settings.webserver.opt.5m":"5 minutes","settings.webserver.opt.10m":"10 minutes","settings.webserver.opt.1h":"1 hour","settings.webserver.opt.never":"Never","settings.webserver.btn":"Save Setting","settings.webserver.saving":"Saving settings and restarting device...","settings.session.title":"Auto-Logout Timer","settings.session.label":"How long to stay logged in:","settings.session.opt.reboot":"Until device reboot","settings.session.opt.1h":"1 hour","settings.session.opt.6h":"6 hours (default)","settings.session.opt.24h":"24 hours","settings.session.opt.3d":"3 days","settings.session.notice":"Controls automatic logout timing for enhanced device security. Sessions survive device restarts except Until reboot mode which requires fresh login after power cycle. Longer durations reduce login frequency but may compromise security if device is lost or stolen.","settings.session.btn":"Save Auto-Logout Timer","settings.session.info.reboot":"Sessions will now expire on device reboot.","settings.session.info.1h":"Sessions will now expire after 1 hour.","settings.session.info.6h":"Sessions will now expire after 6 hours.","settings.session.info.24h":"Sessions will now expire after 24 hours.","settings.session.info.3d":"Sessions will now expire after 3 days.","settings.session.info.fallback":"Session duration updated.","settings.session.err":"Failed to update session duration.","settings.system.title":"System","settings.system.reboot":"Reboot Device","settings.system.reboot.web":"Reboot with Web Server","settings.system.ble.clear":"Clear BLE Clients","settings.system.logout":"Logout","settings.system.ble.cleared":"BLE clients cleared successfully!","settings.system.rebooting.web":"Rebooting with web server enabled...","settings.system.rebooting":"Rebooting...","system.shutdown.title":"Session Timeout","system.shutdown.btn":"Continue Session","system.shutdown.shutdown.title":"Server Shutting Down","system.shutdown.shutdown.desc":"The server has been shut down due to inactivity. Please reboot the device to access it again.","passwords.modal.password":"Password:","passwords.export.success":"Export successful!","passwords.import.success":"Import successful!","keys.btn.export":"Export Keys","keys.btn.import":"Import Keys","keys.export.title":"Confirm Export","keys.export.desc":"Please enter your web admin password to encrypt and export your data.","keys.import.title":"Confirm Import","keys.import.desc":"Enter your web admin password to decrypt and import the selected file. This will overwrite existing data."}};let currentLang=localStorage.getItem('lang')||'en';function tr(key){return i18n[currentLang]?.[key]??i18n['en']?.[key]??key;}async function loadLang(lang){if(i18n[lang]){applyLang(lang);return;}console.warn('Lang not available:',lang);}function applyLang(lang){currentLang=lang;localStorage.setItem('lang',lang);document.querySelectorAll('[data-i18n]').forEach(el=>{const val=tr(el.dataset.i18n);if(el.tagName==='INPUT'&&el.placeholder!==undefined&&el.getAttribute('data-i18n-attr')==='placeholder'){el.placeholder=val;}else{el.textContent=val;}});document.querySelectorAll('.lang-option').forEach(o=>{o.classList.toggle('active',o.dataset.lang===lang);});const flagEl=document.getElementById('lang-flag');const flags={en:'🇬🇧',ru:'🇷🇺',de:'🇩🇪',zh:'🇨🇳',es:'🇪🇸'};if(flagEl)flagEl.textContent=flags[lang]||'🇬🇧';if(typeof fetchKeys==='function'){CacheManager.invalidate('keys_list');fetchKeys();}if(typeof fetchPasswords==='function'){CacheManager.invalidate('passwords_list');fetchPasswords();}if(typeof loadRtcStatus==='function'){loadRtcStatus();}}(function initLangSwitcher(){const flag=document.getElementById('lang-flag');const dropdown=document.getElementById('lang-dropdown');if(!flag||!dropdown)return;flag.addEventListener('click',function(e){e.stopPropagation();dropdown.style.display=dropdown.style.display==='none'?'block':'none';});dropdown.querySelectorAll('.lang-option').forEach(function(opt){opt.addEventListener('click',function(e){e.stopPropagation();const lang=opt.dataset.lang;dropdown.style.display='none';loadLang(lang);});});document.addEventListener('click',function(){dropdown.style.display='none';});})();async function updateBatteryWidget() {
    try {
        const resp = await makeEncryptedRequest('/api/battery', { silentFail: true });
        if (!resp || !resp.ok) return;
        const data = await resp.json();
        if (!data || data.level === undefined) return;
        const w = document.getElementById('battery-widget');
        if (!w) return;
        const lvl = Math.max(0, Math.min(100, data.level));
        const filled = Math.round(lvl / 20);
        w.className = lvl < 20 ? 'critical' : '';
        const cells = [0,1,2,3,4].map(i =>
            '<span style="display:inline-block;width:4px;height:8px;border-radius:1px;' +
            'background:currentColor;opacity:' + (i < filled ? '1' : '0.18') + '"></span>'
        ).join('');
        const icon = '<span style="display:inline-flex;align-items:center;border:1px solid ' +
            'currentColor;border-radius:2px;padding:1px 2px;gap:1px;height:12px;">' +
            cells + '</span>';
        w.innerHTML = lvl + '% ' + (data.charging ? '⚡' : '') + icon;
    } catch(e) {
        console.warn('Battery widget update failed', e);
    }
}

window.onclick = function(event) {
    if (event.target.classList.contains('modal')) {
        event.target.style.display = 'none';
    }
}

document.addEventListener('DOMContentLoaded', async function(){
    try {
        document.getElementById('Keys').style.display = "block";
        
        // 🔒 Проверяем наличие сессионной cookie
        const sessionCookie = getCookie('session');
        if (!sessionCookie) {
            console.log('🧹 No session cookie found, clearing cache...');
            CacheManager.clear(); // Очищаем кеш если нет сессии
        }
        
        // Validate session and fetch CSRF token - redirect if invalid
        const isValidSession = await fetchCsrfToken();
        if (!isValidSession) {
            return;
        }
        
        // 🔗 ЗАГРУЖАЕМ URL MAPPINGS ПЕРВЫМИ - ДО keyExchange!
        // Важно: /api/secure/keyexchange тоже должен быть обфусцирован
        try {
            const cached = sessionStorage.getItem('clientConfig');
            if (cached) {
                const cfg = JSON.parse(cached);
                window.urlObfuscationMap = {};
                if (cfg.k) window.urlObfuscationMap['/api/secure/keyexchange'] = cfg.k;
                if (cfg.t) window.urlObfuscationMap['/api/tunnel'] = cfg.t;
                if (cfg.l) window.urlObfuscationMap['/login'] = cfg.l;
            } else {
                console.log('🔗 Loading URL obfuscation mappings...');
                const response = await fetch('/api/client/config');
                if (response.ok) {
                    const cfg = await response.json();
                    sessionStorage.setItem('clientConfig', JSON.stringify(cfg));
                    window.urlObfuscationMap = {};
                    if (cfg.k) window.urlObfuscationMap['/api/secure/keyexchange'] = cfg.k;
                    if (cfg.t) window.urlObfuscationMap['/api/tunnel'] = cfg.t;
                    if (cfg.l) window.urlObfuscationMap['/login'] = cfg.l;
                } else {
                    window.urlObfuscationMap = {};
                }
            }
        } catch (error) {
            console.warn('⚠️ Error loading URL mappings:', error.message);
            window.urlObfuscationMap = {};
        }
        
        // 🔐 ВАЖНО: Инициализируем SecureClient ПОСЛЕ загрузки mappings
        console.log('[SecureClient] Initializing secure connection...');
        try {
            if (window.secureClient && typeof window.secureClient.establishSoftwareSecureConnection === 'function') {
                const success = await window.secureClient.establishSoftwareSecureConnection();
                if (success) {
                    console.log('🔒 HTTPS-like encryption ACTIVATED!');
                } else {
                    console.warn('❌ Failed to establish secure connection. Using regular HTTP.');
                }
            } else {
                console.warn('⚠️ SecureClient not available, skipping encryption');
            }
        } catch (error) {
            console.error('Error initializing secure connection:', error);
        }
        
        // 🔐 ВАЖНО: Задержка для инициализации SecureClient
        setTimeout(async () => {
            try {
                // 🛡️ Загружаем все ПОСЛЕДОВАТЕЛЬНО чтобы не перегружать сервер
                document.getElementById('add-key-form').reset();
                document.getElementById('add-password-form').reset();
                (async () => {
                try { await fetchKeys(); } catch(e) {}
                try { await fetchPinSettings(); } catch(e) {}
                try { await fetchThemeSettings(); } catch(e) {}
                try { await updateBatteryWidget(); } catch(e) {}
                appInitialized = true;
                })();
                setInterval(updateBatteryWidget, 30000);
                const savedLang=localStorage.getItem('lang');if(savedLang&&savedLang!=='en'){loadLang(savedLang);}
                // ⚠️ ОПТИМИЗАЦИЯ: НЕ проверяем API статус при загрузке
                // API по умолчанию неактивен, проверка только после enableApi()
                // Polling запускается ТОЛЬКО после enableApi() когда нужно
                
                // 🕒 Инициализируем timeout систему ПОСЛЕ secureClient
                initializeTimeoutSystem();
            } catch (err) {
                console.error('Error during initialization:', err);
            }
        }, 1000); // 1 секунда задержки
    } catch (err) {
        console.error('❌ Fatal error in DOMContentLoaded:', err);
        showStatus('Initialization error. Please reload the page.', true);
    }
});

// --- New API Access Logic ---
let apiAccessInterval = null;
let apiRemainingSeconds = 0; // Локальный счетчик времени
let apiCountdownInterval = null;

function enableApi() {
    makeEncryptedRequest('/api/enable_import_export', { method: 'POST' })
    .then(res => {
        if (res.ok) {
            showStatus(tr('keys.api.enabled'));
            updateApiStatus(); // Получаем начальное время
            
            // ⚡ ОПТИМИЗАЦИЯ: Запускаем редкий polling (30 сек) для синхронизации
            if (apiAccessInterval) clearInterval(apiAccessInterval);
            apiAccessInterval = setInterval(updateApiStatus, 30000); // Каждые 30 сек
            
            // 🕐 Локальный countdown таймер (каждую секунду)
            if (apiCountdownInterval) clearInterval(apiCountdownInterval);
            apiCountdownInterval = setInterval(localCountdown, 1000);
        } else {
            showStatus('Failed to enable API access.', true);
        }
    }).catch(err => showStatus('Error: ' + err, true));
}

// 🕐 Локальный countdown без запросов к серверу
function localCountdown() {
    if (apiRemainingSeconds > 0) {
        apiRemainingSeconds--;
        
        // Обновляем UI локально
        const statusElements = document.querySelectorAll('.api-status');
        statusElements.forEach(el => {
            el.textContent = tr('keys.api.active').replace('{s}', apiRemainingSeconds);
            el.style.color = '#81c784'; // Green
        });
    } else {
        // Время истекло - отключаем кнопки и останавливаем таймеры
        const statusElements = document.querySelectorAll('.api-status');
        const exportKeysBtn = document.getElementById('export-keys-btn');
        const importKeysBtn = document.getElementById('import-keys-btn');
        const exportPasswordsBtn = document.getElementById('export-passwords-btn');
        const importPasswordsBtn = document.getElementById('import-passwords-btn');
        
        statusElements.forEach(el => {
            el.textContent = 'Inactive';
            el.style.color = '#ffc107'; // Yellow
        });
        exportKeysBtn.disabled = true;
        importKeysBtn.disabled = true;
        exportPasswordsBtn.disabled = true;
        importPasswordsBtn.disabled = true;
        
        // Останавливаем оба таймера
        if (apiAccessInterval) {
            clearInterval(apiAccessInterval);
            apiAccessInterval = null;
        }
        if (apiCountdownInterval) {
            clearInterval(apiCountdownInterval);
            apiCountdownInterval = null;
        }
        console.log('🛑 API access expired (local countdown)');
    }
}

// 🔄 Синхронизация с сервером (вызывается редко - каждые 30 сек)
function updateApiStatus() {
    makeEncryptedRequest('/api/import_export_status')
    .then(response => response.json())
    .then(data => {
        const statusElements = document.querySelectorAll('.api-status');
        const exportKeysBtn = document.getElementById('export-keys-btn');
        const importKeysBtn = document.getElementById('import-keys-btn');
        const exportPasswordsBtn = document.getElementById('export-passwords-btn');
        const importPasswordsBtn = document.getElementById('import-passwords-btn');

        if (data.enabled) {
            // 📥 Синхронизируем локальный счетчик с сервером
            apiRemainingSeconds = data.remaining;
            console.log(`🔄 Synced with server: ${apiRemainingSeconds}s remaining`);
            
            statusElements.forEach(el => {
                el.textContent = tr('keys.api.active').replace('{s}', apiRemainingSeconds);
                el.style.color = '#81c784'; // Green
            });
            exportKeysBtn.disabled = false;
            importKeysBtn.disabled = false;
            exportPasswordsBtn.disabled = false;
            importPasswordsBtn.disabled = false;
        } else {
            // API неактивен - останавливаем всё
            apiRemainingSeconds = 0;
            
            statusElements.forEach(el => {
                el.textContent = 'Inactive';
                el.style.color = '#ffc107'; // Yellow
            });
            exportKeysBtn.disabled = true;
            importKeysBtn.disabled = true;
            exportPasswordsBtn.disabled = true;
            importPasswordsBtn.disabled = true;
            
            // 🛑 Останавливаем оба таймера
            if (apiAccessInterval) {
                clearInterval(apiAccessInterval);
                apiAccessInterval = null;
            }
            if (apiCountdownInterval) {
                clearInterval(apiCountdownInterval);
                apiCountdownInterval = null;
            }
            console.log('🛑 Stopped API polling (API inactive from server)');
        }
    }).catch(err => console.error('Error fetching API status:', err));
}

document.querySelectorAll('.enable-api-btn').forEach(button => {
    button.addEventListener('click', enableApi);
});
// --- End of New API Access Logic ---

// Session Duration Settings Functions
async function fetchSessionDurationSettings() {
    try{
    const res = await makeEncryptedRequest('/api/session_duration', {
        method: 'GET'
    });
    const data = await res.json();
        const select = document.getElementById('session-duration');
        if (select && data.duration !== undefined) {
            select.value = data.duration;
        }
    }catch(err){
        console.error('Error fetching session duration settings:', err);
    }
}

// Session Duration Form Handler
document.addEventListener('DOMContentLoaded', function() {
    const sessionDurationForm = document.getElementById('session-duration-form');
    if (sessionDurationForm) {
        sessionDurationForm.addEventListener('submit', function(e) {
            e.preventDefault();
            
            const duration = document.getElementById('session-duration').value;
            
            const formData = new FormData();
            formData.append('duration', duration);
            
            makeEncryptedRequest('/api/session_duration', {
                method: 'POST',
                body: formData
            }).then(res => res.json()).then(data => {
                if (data.success) {
                    showStatus(tr('settings.session.info.fallback'));
                    
                    // Show info about when changes take effect
                    const infoMessages = {
                        '0': tr('settings.session.info.reboot'),
                        '1': tr('settings.session.info.1h'),
                        '6': tr('settings.session.info.6h'),
                        '24': tr('settings.session.info.24h'),
                        '72': tr('settings.session.info.3d')
                    };
                    
                    setTimeout(() => {
                        showStatus(infoMessages[duration] || tr('settings.session.info.fallback'));
                    }, 2000);
                } else {
                    showStatus(data.message, true);
                }
                
            }).catch(err => {
                console.error('Error updating session duration:', err);
                showStatus(tr('settings.session.err'), true);
            });
        });
    }
});

// ==================== QR SCANNER ====================
const isApMode = window.location.hostname === '192.168.4.1';

function loadQrLibrary(callback) {
    if (typeof Html5Qrcode !== 'undefined') {
        callback();
        return;
    }
    const script = document.createElement('script');
    script.src = 'https://cdn.jsdelivr.net/npm/html5-qrcode@2.3.8/html5-qrcode.min.js';
    script.onload = callback;
    script.onerror = function() {
        console.warn('QR library failed to load (no internet?)');
    };
    document.head.appendChild(script);
}

let html5QrCode = null;
let qrLibraryLoaded = false;

// Проверка загрузки библиотеки
function checkQrLibrary() {
    if (typeof Html5Qrcode !== 'undefined') {
        qrLibraryLoaded = true;
        console.log('✅ Html5Qrcode library loaded successfully');
        return true;
    }
    return false;
}

// Ждём загрузки библиотеки
window.addEventListener('load', function() {
    setTimeout(() => {
        if (checkQrLibrary()) {
            console.log('QR Scanner ready');
        } else {
            setTimeout(checkQrLibrary, 1000);
        }
    }, 500);
});

// Парсинг TOTP URI (otpauth://totp/Label?secret=SECRET&issuer=Issuer)
function parseTotpUri(uri) {
    try {
        // Санитизация входных данных
        if (typeof uri !== 'string') {
            throw new Error('Invalid input type');
        }
        
        uri = uri.trim();
        
        // Проверка на опасные символы
        if (uri.includes('<script') || uri.includes('javascript:') || uri.includes('data:')) {
            throw new Error('Potentially malicious content detected');
        }
        
        // Проверяем является ли это TOTP/HOTP URI
        if (uri.startsWith('otpauth://totp/') || uri.startsWith('otpauth://hotp/')) {
            const url = new URL(uri);
            
            if (url.protocol !== 'otpauth:') {
                throw new Error('Invalid OTP URI format');
            }
            
            const type = url.hostname; // 'totp' или 'hotp'
            
            if (type !== 'totp' && type !== 'hotp') {
                throw new Error('Invalid OTP type (must be totp or hotp)');
            }
            
            let label = decodeURIComponent(url.pathname.substring(1));
            const params = new URLSearchParams(url.search);
            const secret = params.get('secret');
            const issuer = params.get('issuer');
            
            // Расширенные параметры
            const algorithm = params.get('algorithm') || 'SHA1';
            const digits = parseInt(params.get('digits')) || 6;
            const period = parseInt(params.get('period')) || 30;
            const counter = parseInt(params.get('counter')) || 0;
            
            if (!secret) {
                throw new Error('Secret not found in QR code');
            }
            
            // Валидация secret (только Base32 символы)
            const base32Pattern = /^[A-Z2-7]+=*$/;
            if (!base32Pattern.test(secret.toUpperCase())) {
                throw new Error('Invalid secret format (must be Base32)');
            }
            
            // Валидация algorithm
            if (!['SHA1', 'SHA256', 'SHA512'].includes(algorithm.toUpperCase())) {
                throw new Error('Invalid algorithm (must be SHA1, SHA256, or SHA512)');
            }
            
            // Валидация digits
            if (![6, 8].includes(digits)) {
                throw new Error('Invalid digits (must be 6 or 8)');
            }
            
            // Валидация period (для TOTP)
            if (type === 'totp' && ![30, 60].includes(period)) {
                console.warn('Unusual period value: ' + period + ', using anyway');
            }
            
            // Санитизация label (удаляем опасные символы)
            label = label.replace(/[<>\"']/g, '');
            
            // Smart issuer handling to avoid duplicates
            if (issuer && label.startsWith(issuer + ':')) {
                // Label is "Issuer:account" - keep as is, it's already clean
            } else if (issuer && label.includes(':')) {
                // Label has colon separator already - don't add issuer again
            } else if (issuer && !label.includes(issuer)) {
                // Label doesn't mention issuer at all - add it
                const sanitizedIssuer = issuer.replace(/[<>\"']/g, '');
                label = sanitizedIssuer + ': ' + label;
            }
            
            // Ограничение длины label
            if (label.length > 100) {
                label = label.substring(0, 100);
            }
            
            // Возвращаем расширенные данные
            return {
                name: label,
                secret: secret.toUpperCase(),
                type: type === 'hotp' ? 'H' : 'T',
                algorithm: algorithm.toUpperCase() === 'SHA256' ? '256' : 
                           algorithm.toUpperCase() === 'SHA512' ? '512' : '1',
                digits: digits,
                period: period,
                counter: counter
            };
        } else {
            // Если это не URI, предполагаем что это просто Base32 secret
            // Проверяем что это похоже на Base32 (только A-Z, 2-7, и =)
            const base32Pattern = /^[A-Z2-7]+=*$/;
            const upperUri = uri.toUpperCase().trim();
            
            if (base32Pattern.test(upperUri)) {
                // Возвращаем с дефолтными значениями
                return {
                    name: '',
                    secret: upperUri,
                    type: 'T',
                    algorithm: '1',
                    digits: 6,
                    period: 30,
                    counter: 0
                };
            } else {
                throw new Error('Not a valid TOTP/HOTP URI or Base32 secret');
            }
        }
    } catch (error) {
        console.error('Error parsing TOTP URI:', error);
        throw error;
    }
}

// Показать статус
function showQrStatus(message, isSuccess) {
    const statusDiv = document.getElementById('qr-status');
    statusDiv.textContent = message;
    statusDiv.className = isSuccess ? 'success' : 'error';
    statusDiv.style.display = 'block';
    
    setTimeout(() => {
        statusDiv.style.display = 'none';
    }, 5000);
}

// Сканирование из файла
async function scanQrFromFile(file) {
    try {
        // 1. Проверка типа файла
        if (!file.type.startsWith('image/')) {
            showQrStatus('❌ Please select an image file (PNG, JPG, JPEG)', false);
            return;
        }
        
        // 2. Проверка размера файла (макс 10MB)
        const maxSize = 10 * 1024 * 1024; // 10MB
        if (file.size > maxSize) {
            showQrStatus('❌ Image too large. Maximum size: 10MB', false);
            return;
        }
        
        showQrStatus('📷 Scanning image...', true);
        
        // 3. Проверка что библиотека загружена
        if (!qrLibraryLoaded && !checkQrLibrary()) {
            showQrStatus('❌ QR library loading... Please wait and try again.', false);
            return;
        }
        
        if (!html5QrCode) {
            html5QrCode = new Html5Qrcode("qr-reader");
        }
        
        // 4. Сканирование с таймаутом
        const scanPromise = html5QrCode.scanFile(file, false);
        const timeoutPromise = new Promise((_, reject) => 
            setTimeout(() => reject(new Error('Scan timeout')), 10000)
        );
        
        const result = await Promise.race([scanPromise, timeoutPromise]);
        
        // 5. Проверка длины результата
        if (!result || result.length === 0) {
            showQrStatus('❌ QR code is empty', false);
            return;
        }
        
        if (result.length > 1000) {
            showQrStatus('❌ QR code data too long (max 1000 chars)', false);
            return;
        }
        
        // 6. Парсинг и валидация
        try {
            const totpData = parseTotpUri(result);
            
            // 7. Дополнительная проверка secret
            if (totpData.secret.length < 16 || totpData.secret.length > 128) {
                showQrStatus('❌ Invalid secret length (must be 16-128 chars)', false);
                return;
            }
            
            // 8. Заполнение полей
            if (totpData.name) {
                document.getElementById('key-name').value = totpData.name;
            }
            document.getElementById('key-secret').value = totpData.secret;
            
            // 9. Заполнение расширенных полей
            // Type (TOTP/HOTP)
            if (totpData.type === 'H') {
                document.getElementById('type-hotp').checked = true;
            } else {
                document.getElementById('type-totp').checked = true;
            }
            updateKeyTypeFields(); // Обновить видимость полей
            
            // Algorithm
            document.getElementById('key-algorithm').value = totpData.algorithm;
            
            // Digits
            if (totpData.digits === 8) {
                document.querySelector('input[name="key-digits"][value="8"]').checked = true;
            } else {
                document.querySelector('input[name="key-digits"][value="6"]').checked = true;
            }
            
            // Period (для TOTP)
            if (totpData.type === 'T') {
                if (totpData.period === 60) {
                    document.querySelector('input[name="key-period"][value="60"]').checked = true;
                } else {
                    document.querySelector('input[name="key-period"][value="30"]').checked = true;
                }
            }
            
            // Counter (для HOTP)
            if (totpData.type === 'H') {
                document.getElementById('key-counter').value = totpData.counter;
            }
            
            // Auto-expand advanced settings if any non-default values detected
            const isNonDefault = (totpData.algorithm !== '1' ||
                                  totpData.digits !== 6 ||
                                  (totpData.type === 'T' && totpData.period !== 30) ||
                                  totpData.type === 'H');
            if (isNonDefault) {
                const advancedDetails = document.querySelector('#add-key-form details');
                if (advancedDetails) advancedDetails.open = true;
            }
            
            // 10. Показать сообщение с информацией о типе
            let typeInfo = totpData.type === 'H' ? 'HOTP' : 'TOTP';
            let algoInfo = totpData.algorithm === '256' ? 'SHA256' : totpData.algorithm === '512' ? 'SHA512' : 'SHA1';
            let digitsInfo = totpData.digits + ' digits';
            
            if (totpData.name) {
                showQrStatus(`✅ ${typeInfo} key scanned! (${algoInfo}, ${digitsInfo})`, true);
            } else {
                showQrStatus(`✅ ${typeInfo} secret scanned! (${algoInfo}, ${digitsInfo}) Please enter a name.`, true);
            }
        } catch (error) {
            showQrStatus('❌ Invalid TOTP/HOTP QR code: ' + error.message, false);
        }
        
    } catch (error) {
        console.error('Error scanning file:', error);
        if (error.message === 'Scan timeout') {
            showQrStatus('❌ Scan timeout. Try a clearer image.', false);
        } else {
            showQrStatus('❌ No QR code found in image', false);
        }
    }
}

// Привязка к кнопке
document.addEventListener('DOMContentLoaded', function() {
    // Кнопка загрузки файла
    const scanFileBtn = document.getElementById('scan-qr-file');
    const fileInput = document.getElementById('qr-file-input');
    
    if (scanFileBtn && fileInput) {
        if (isApMode) {
            scanFileBtn.disabled = true;
            scanFileBtn.title = 'QR scan unavailable in AP mode (no internet)';
            scanFileBtn.style.opacity = '0.4';
            scanFileBtn.style.cursor = 'not-allowed';
        } else {
            scanFileBtn.addEventListener('click', () => {
                fileInput.click();
            });
        }
        
        fileInput.addEventListener('change', (e) => {
            const file = e.target.files[0];
            if (!file) return;
            loadQrLibrary(function() {
                scanQrFromFile(file);
            });
            e.target.value = '';
        });
    }
});
// ==================== END QR SCANNER ====================

</script>


<!-- START: WEB SERVER TIMEOUT MODAL -->
<div id="timeout-modal" style="display: none; position: fixed; z-index: 2000; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.7); backdrop-filter: blur(5px);">
    <div style="background: rgba(40, 40, 60, 0.9); border: 1px solid rgba(255, 255, 255, 0.1); margin: 15% auto; padding: 30px; width: 90%; max-width: 400px; border-radius: 15px; box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.5); color: #e0e0e0; text-align: center;">
        <h3 data-i18n="system.shutdown.title" style="color: #ffffff; margin-top: 0;">Session Timeout</h3>
        <p data-i18n="system.shutdown.desc">The web server will automatically shut down due to inactivity.</p>
        <p><span data-i18n="system.shutdown.remaining">Time remaining:</span> <span id="timeout-countdown" style="font-weight: bold; font-size: 1.2em; color: #5a9eee;">60</span>s</p>
        <button id="timeout-keep-alive-btn" data-i18n="system.shutdown.btn" class="button user-activity" style="width: 100%; padding: 15px; font-size: 1.1em;">Continue Session</button>
    </div>
</div>

<script>
// 🎯 Сделано глобальной функцией чтобы вызывать из DOMContentLoaded
(function() {
    // Get timeout from server configuration
    let WARNING_TIME = 1 * 60 * 1000; // Default fallback
    let COUNTDOWN_SECONDS = 60;
    let SERVER_TIMEOUT_MINUTES = 2; // Default fallback

    let inactivityTimer = null;
    let countdownTimer = null;
    let remainingSeconds = COUNTDOWN_SECONDS;

    const modal = document.getElementById('timeout-modal');
    const countdownSpan = document.getElementById('timeout-countdown');
    const keepAliveBtn = document.getElementById('timeout-keep-alive-btn');

    // Fetch server timeout configuration and initialize timers
    // 🎯 Сделано глобальной чтобы было доступно из DOMContentLoaded
    window.initializeTimeoutSystem = function() {
        makeAuthenticatedRequest('/api/config')
            .then(response => response.json())
            .then(config => {
                if (config.web_server_timeout && config.web_server_timeout > 0) {
                    SERVER_TIMEOUT_MINUTES = config.web_server_timeout;
                    // Show warning 1 minute before timeout, but at least 30 seconds for very short timeouts
                    if (SERVER_TIMEOUT_MINUTES <= 1) {
                        WARNING_TIME = (SERVER_TIMEOUT_MINUTES * 60 * 1000) / 2; // Half of timeout for very short timeouts
                    } else {
                        WARNING_TIME = (SERVER_TIMEOUT_MINUTES - 1) * 60 * 1000; // 1 minute before for normal timeouts
                    }
                } else {
                    // If timeout is 0 or disabled, don't show warning
                    WARNING_TIME = 0;
                }
                // Start the timeout system after getting configuration
                resetServerActivity();
            })
            .catch(err => {
                // Use default values and start anyway
                resetServerActivity();
            });
    }

    function showWarningModal() {
        remainingSeconds = COUNTDOWN_SECONDS;
        countdownSpan.textContent = remainingSeconds;
        modal.style.display = 'block';
        
        countdownTimer = setInterval(() => {
            remainingSeconds--;
            countdownSpan.textContent = remainingSeconds;
            if (remainingSeconds <= 0) {
                clearInterval(countdownTimer);
                modal.querySelector('h3').textContent = tr('system.shutdown.shutdown.title');
                modal.querySelector('p').textContent = tr('system.shutdown.shutdown.desc');
                keepAliveBtn.style.display = 'none';
            }
        }, 1000);
    }

    function hideWarningModal() {
        modal.style.display = 'none';
        if (countdownTimer) {
            clearInterval(countdownTimer);
            countdownTimer = null;
        }
    }

    function resetServerActivity() {
        hideWarningModal();

        if (WARNING_TIME > 0) {
            makeAuthenticatedRequest('/api/activity', { method: 'POST' }).catch(err => console.error("Failed to reset activity timer:", err));

            if (inactivityTimer) {
                clearTimeout(inactivityTimer);
            }
            inactivityTimer = setTimeout(showWarningModal, WARNING_TIME);
        }
    }

    function addActivityListeners() {
        document.querySelectorAll('.user-activity').forEach(elem => {
            elem.addEventListener('click', resetServerActivity);
            elem.addEventListener('input', resetServerActivity);
        });
    }

    // Initial setup
    // initializeTimeoutSystem(); // ❌ ПЕРЕМЕЩЕНО в DOMContentLoaded после secureClient
    addActivityListeners();
    
    // Re-attach listeners after fetching new content, e.g., after fetching keys/passwords
    // We can override the original functions to add this hook
    const originalFetchKeys = window.fetchKeys;
    window.fetchKeys = function() {
        originalFetchKeys();
        setTimeout(addActivityListeners, 500); // Re-add listeners after table is populated
    };
    
    const originalFetchPasswords = window.fetchPasswords;
    window.fetchPasswords = function() {
        originalFetchPasswords();
        setTimeout(addActivityListeners, 500); // Re-add listeners after table is populated
    };

})();
</script>
<!-- END: WEB SERVER TIMEOUT MODAL -->

</body></html>
)rawliteral";
