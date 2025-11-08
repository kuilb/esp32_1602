#include "web_component.h"

String getWebComponent() {
    return R"(
<script>
class WebStyles extends HTMLElement {
    connectedCallback() {
        if (!document.getElementById('web-styles')) {
            const style = document.createElement('style');
            style.id = 'web-styles';
            style.textContent = `
                :root {
                    --blue: #0067b6;
                    --blue-dark: #0045a4;
                    --red: #e57373;
                    --red-dark: #d35f5f;
                    --cyan: #66ddcc;
                    --cyan-dark: #55ccbb;
                    --gray: #6c757d;
                    --gray-dark: #6a7690;
                    --gray-light: #f8f9fa;
                    --gray-lighter: #343a40;
                    --white: #fff;
                    --border: #dee2e6;
                    --shadow: 0 4px 12px rgba(0,0,0,0.08);
                    --border-radius: 8px;
                }
                body {
                    background: linear-gradient(90deg, rgba(179,255,253,0.5) 0%, rgba(227,230,255,0.5) 50%, rgba(253,229,245,0.5) 100%);
                    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
                    background-color: var(--gray-light);
                    color: var(--gray-lighter);
                    line-height: 1.5;
                    text-align: center;
                    margin: 0;
                }
                .container {
                    max-width: 500px;
                    margin: 32px auto;
                    padding: 24px;
                    background: rgba(255,255,255,0.6);
                    backdrop-filter: blur(10px);
                    -webkit-backdrop-filter: blur(10px);
                    border-radius: 25px;
                    box-shadow: var(--shadow);
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                }
                h1 {
                    color: var(--blue);
                    font-weight: 600;
                    margin: 0 0 18px 0;
                    font-size: 1.5em;
                }
                h3 {
                    color: var(--gray-dark);
                    margin: 18px 0 10px 0;
                }
                label {
                    display: block;
                    margin: 0.8em 0 0.2em;
                    color: var(--blue);
                    font-weight: bold;
                    font-size: 1em;
                    text-align: left;
                }
                input[type=password], input[type=text], [type=file],select{
                    width: 100%;
                    padding: 10px;
                    margin: 10px 0;
                    border: 1px solid var(--border);
                    border-radius: 6px;
                    font-size: 16px;
                    transition: border-color 0.2s, box-shadow 0.2s;
                    background: linear-gradient(135deg, rgba(255,255,255,0.8), rgba(240,248,255,0.8));
                    box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);
                }
                input[type=password]:focus, input[type=text]:focus, [type=file]:focus {
                    outline: none;
                    border-color: var(--blue);
                    box-shadow: 0 0 0 3px rgba(0,123,255,0.15), inset 0 1px 2px rgba(0,0,0,0.1);
                    transform: scale(1.02);
                }
                input[type=password]::placeholder, input[type=text]::placeholder, [type=file]::placeholder {
                    color: var(--gray);
                    font-style: italic;
                    opacity: 0.7;
                }
                button, input[type=submit] {
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    gap: 8px;
                    padding: 12px;
                    border: none;
                    border-radius: var(--border-radius);
                    font-size: 16px;
                    font-weight: 500;
                    cursor: pointer;
                    transition: background-color 0.2s ease, transform 0.1s ease;
                    color: var(--white);
                    text-decoration: none;
                }
                button:hover, input[type=submit]:hover {
                    transform: translateY(-2px);
                }
                button:active, input[type=submit]:active {
                    transform: translateY(0);
                }
                button svg, input[type=submit] svg {
                    vertical-align: middle;
                }
                .btn-blue {
                    background-color: var(--blue);
                }
                .btn-blue:hover {
                    background-color: var(--blue-dark);
                }
                .btn-cyan {
                    background-color: var(--cyan);
                    color: var(--gray-lighter);
                }
                .btn-cyan:hover {
                    background-color: var(--cyan-dark);
                }
                .btn-red {
                    background-color: var(--red);
                }
                .btn-red:hover {
                    background-color: var(--red-dark);
                }
                .btn-gray {
                    background-color: var(--gray);
                }
                .btn-gray:hover {
                    background-color: var(--gray-dark);
                }
                .button-group {
                    display: flex;
                    justify-content: center;
                    gap: 10px;
                    width: 100%;
                    max-width: 340px;
                }
                @media(max-width:800px) {
                    .container {
                        width: 80vw;
                    }
                }
                .loading {
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                    margin-top: 20px;
                }
                .loading.hidden {
                    display: none;
                }
                .spinner {
                    width: 40px;
                    height: 40px;
                    border: 4px solid var(--gray-light);
                    border-top: 4px solid var(--blue);
                    border-radius: 50%;
                    animation: spin 1s linear infinite;
                }
                @keyframes spin {
                    0% { transform: rotate(0deg); }
                    100% { transform: rotate(360deg); }
                }
                .loading p {
                    color: var(--blue);
                    font-weight: 500;
                    margin-top: 10px;
                }
            `;
            document.head.appendChild(style);
        }
    }
}
customElements.define('web-styles', WebStyles);
</script>
    )";
}