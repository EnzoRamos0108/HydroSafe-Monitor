# HydroSafe - Sistema Inteligente de Monitoramento de Enchentes

[](https://#)[](LICENSE)
[![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)

<p align="center">
  <img src="hydrosafe_youtube_background.jpg" alt="HydroSafe Logo" width="cover"/>
</p>

**HydroSafe** é um sistema de monitoramento em tempo real focado na prevenção de enchentes urbanas através da medição contínua dos níveis de água em bueiros, galerias pluviais e rios. Utilizando sensores acessíveis e tecnologia IoT (potencial), o HydroSafe visa fornecer alertas antecipados para autoridades e comunidades, ajudando a mitigar danos materiais e, principalmente, a salvar vidas.

---

### 🌧️ Contextualização: O Problema das Enchentes no Brasil

> _"As enchentes no Brasil não são eventos inesperados — são tragédias anunciadas."_  

![Gif de enchente ilustrativo](link_para_o_gif_aqui)

As enchentes representam um dos desastres naturais mais recorrentes e devastadores no Brasil. Entre 1991 e 2022, o país registrou **mais de 21 mil inundações**, afetando cerca de **110 milhões de pessoas**. Apenas em 2023, **mais de 3,3 milhões** de brasileiros foram impactados por eventos hidrológicos extremos, resultando em **pelo menos 142 mortes**. Em 2024, o estado do **Rio Grande do Sul** viveu uma das piores tragédias da história recente, com cidades inteiras como Canoas e Roca Sales permanecendo submersas por semanas.

As causas das enchentes são múltiplas, mas a principal não é natural: trata-se da **falta de planejamento urbano, drenagem insuficiente e ausência de políticas públicas eficazes de prevenção**. Segundo o IBGE, **mais de 8 milhões de brasileiros vivem em áreas de risco**, sendo que **mais da metade dessas regiões não possuem sistemas básicos de escoamento de águas pluviais**.

Além do impacto humano, os prejuízos econômicos são expressivos. Estima-se que as enchentes causam **mais de R$ 8 bilhões em perdas anuais** no Brasil, valor que salta em eventos extremos — como os mais de **R$ 20 bilhões de prejuízos projetados só no RS em 2024**, segundo a CNI.

Nesse cenário, **tecnologias de monitoramento e alerta precoce** como o _HydroSafe_ se tornam essenciais para **antecipar riscos, salvar vidas e mitigar danos**. Sensores conectados, IoT e soluções locais baseadas em Arduino são alternativas acessíveis e eficazes para combater um problema estrutural e recorrente.


## ⚠️ Aviso Importante

- Este software/hardware é um protótipo destinado a auxiliar no monitoramento de níveis de água para prevenção de enchentes e deve ser utilizado para fins educacionais e de desenvolvimento.

- A precisão dos dados depende da correta instalação, calibração e manutenção dos sensores e do sistema.

- Fatores ambientais (detritos, obstruções, condições climáticas extremas) podem afetar o desempenho dos sensores.

- O HydroSafe utiliza componentes eletrônicos que requerem instalação cuidadosa, especialmente em ambientes úmidos ou externos. Garanta a devida proteção contra água e intempéries.

---

## ✨ Funcionalidades Principais

- 🌊 **Monitoramento Contínuo:** Medição em tempo real do nível da água usando sensor ultrassônico.

- 🚦 **Indicadores Visuais:** LEDs de status (Verde: Normal, Amarelo: Alerta, Vermelho: Perigo) para rápida identificação do risco.

- 🔊 **Alerta Sonoro:** Buzzer integrado para emitir alarmes audíveis em níveis críticos.

- 📟 **Display Local:** Tela OLED SH1107 para visualização direta do nível da água.

## 🛠️ Tecnologias Utilizadas

- **Hardware:**
  - Microcontrolador: Arduino Uno (ou compatível)
  - Sensor: Ultrassônico HC-SR04
  - Indicadores: LEDs (Verde, Amarelo, Vermelho)
  - Alerta: Buzzer Passivo
  - Display: OLED SH1107

- **Software:**
  - Linguagem: C++
  - Simulação: Wokwi

- **Bibliotecas Arduino:**
  - `LiquidCrystal_I2C`
  - `U8g2`

---

## 🏗️ Estrutura do Projeto

O projeto é composto principalmente por:

1. **Nó Sensor (Hardware):** O dispositivo físico montado com Arduino, sensores e atuadores, responsável pela coleta de dados no local.

1. **Firmware (Software):** O código `hydrosafe_monitor.ino` que roda no Arduino, controlando os sensores, processando os dados e ativando os alertas.

1. **Simulação (Wokwi):** Arquivos `diagram.json` e o código `.ino` para simular o hardware e software em ambiente virtual.

---

## ⚙️ Instalação

### 1. Hardware (Montagem do Nó Sensor)

- **Conexões:** Siga o esquemático definido no `diagram.json` (ou crie um baseado nos pinos definidos no `.ino`) para conectar o sensor HC-SR04, os LEDs (com resistores apropriados), o buzzer e o display OLED SH1107 ao Arduino Uno.
  - Sensor HC-SR04: VCC, GND, Trig, Echo
  - LEDs: Anodo (+) ao pino digital via resistor, Catodo (-) ao GND.
  - Buzzer: Pino positivo (+) ao pino digital, Pino negativo (-) ao GND.
  - OLED SH1107: VCC, GND, SDA, SCL.

- **Alimentação:** Forneça alimentação adequada ao Arduino (via USB ou fonte externa).

- **Proteção:** Aloje os componentes em uma caixa protetora à prova d'água, garantindo que o sensor ultrassônico tenha uma visão clara da superfície da água a ser medida, mas esteja protegido de submersão direta e detritos.

- **Fixação:** Instale a caixa de forma segura no local de monitoramento (bueiro, margem do rio), considerando a altura máxima esperada da água.

### 2. Software (Firmware Arduino)

- **IDE:** Instale a [Arduino IDE](https://www.arduino.cc/en/software).

- **Bibliotecas:** Instale as bibliotecas `LiquidCrystal_I2C` e `U8g2`através do Gerenciador de Bibliotecas da IDE.

- **Código:** Abra o arquivo `hydrosafe_monitor.ino` na Arduino IDE.

- **Configuração:** Ajuste as constantes no início do código (pinos dos componentes, altura do sensor, limiares de alerta) se sua montagem for diferente da original.

- **Upload:** Conecte o Arduino ao computador via USB, selecione a placa e a porta corretas na IDE e faça o upload do código.

### 3. Simulação (Wokwi - Opcional)

- Acesse [Wokwi.com](https://wokwi.com/).

- Crie um novo projeto Arduino.

- Copie o conteúdo do `diagram.json` para a aba `diagram.json` no Wokwi.

- Copie o conteúdo do `hydrosafe_monitor.ino` para a aba do sketch (`.ino`).

- Instale as bibliotecas `LiquidCrystal_I2C` e `U8g2.`

- Inicie a simulação. Você pode interagir com o sensor ultrassônico virtual para testar os diferentes níveis e alertas.

---

## 🚀 Como Usar

1. **Energizar:** Ligue o nó sensor HydroSafe.

1. **Monitoramento Local:**

- Observe o **Display OLED SH1107**: Ele mostrará a distância medida pelo sensor de acordo com a calibração feita (a atual é com altura máxima de 1.5m) e o nível de risco atual (Normal, Alerta, Perigo).

- Observe os **LEDs**: O LED correspondente ao nível de risco atual acenderá.

- Escute o **Buzzer**: Ele soará continuamente se o nível de Perigo for atingido.

1. **Ajuste de Limiares:** Se necessário, modifique os valores `LIMITE_ALERTA` e `LIMITE_PERIGO` no código `hydrosafe_monitor.ino` e faça o upload novamente para ajustar a sensibilidade dos alertas.

---

## 🎬 Demonstração

**Simulação no Wokwi:**


<p align="center">
  <img src="hydrosafe-print.png" alt="Print da simulação" width="cover"/>
</p>


*Imagem da nossa simulação*

🔗 *[https://wokwi.com/projects/432755988180161537]*

**Vídeo de Funcionamento:**

```
[LINK PARA VÍDEO NO YOUTUBE/VIMEO AQUI]
```

*Link para o vídeo demonstrando o monitor HydroSafe em ação.*

---



## 📄 Licença

Este projeto é de uso **educacional e acadêmico**. Sinta-se livre para estudar, adaptar e se inspirar! 

---

> Desenvolvido com dedicação, criatividade e muitas horas de café por Enzo Ramos, Felipe Cerazi e Gustavo Peaguda. 💻🍷



---
