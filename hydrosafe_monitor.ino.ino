#include <U8g2lib.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
// A biblioteca Wire já está incluída
#endif

// === CONFIGURAÇÕES ===
#define FUSO_HORARIO_UTC -3 // Fuso horário UTC-3 (Brasília)
#define PINO_TRIG 3         // Pino Trigger do sensor ultrassônico
#define PINO_ECHO 2         // Pino Echo do sensor ultrassônico

// --- Pinos --- 
const int PINO_LED_VERDE = 12;   // LED indicador de estado Normal
const int PINO_LED_AMARELO = 9;  // LED indicador de estado Alerta
const int PINO_LED_VERMELHO = 11; // LED indicador de estado Crítico
const int PINO_BUZZER = 10;      // Pino do Buzzer
const int PINO_BOTAO_RESET_EEPROM = 4; // Pino do botão para limpar a EEPROM
const int PINO_BOTAO_MODO = 7;         // Pino do botão para alternar modo de exibição

// --- Parâmetros do Bueiro --- 
const float ALTURA_BUEIRO_METROS = 1.50; // Altura máxima do bueiro em metros

// --- Limites dos Estados (metros) ---
const float NIVEL_ALERTA_METROS = 0.75;
const float NIVEL_CRITICO_METROS = 1.10;

// OLED SH1107 128x128 I2C (Hardware I2C)
U8G2_SH1107_128X128_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
RTC_DS1307 rtc; // Renomeado para minúsculo por convenção

// Configuração da EEPROM
const int MAX_REGISTROS_EEPROM = 100; // Ajustar se necessário (depende do tamanho da EEPROM)
const int TAMANHO_REGISTRO_EEPROM = 6; // 4 bytes para timestamp (unix), 2 bytes para nível (em mm)
int enderecoAtualEEPROM = 0;
float ultimoNivelCriticoSalvo = -1.0; // Para evitar salvar repetidamente o mesmo nível crítico

// --- Variáveis Globais --- 
enum EstadoNivel { NORMAL, ALERTA, CRITICO };
EstadoNivel estadoAtual = NORMAL;
const char* estadoAtualStr = "Normal";

bool modoExibicaoAnimacao = true; // true = Animação, false = Texto
unsigned long tempoUltimoAcionamentoBotao = 0;
const unsigned long ATRASO_DEBOUNCE_MS = 50; // Tempo (ms) para debounce dos botões

// --- Parâmetros da Animação --- 
const int TANQUE_X = 10; // Posição X do tanque no display
const int TANQUE_Y = 0;  // Posição Y do tanque no display
const int TANQUE_LARGURA = 108; // Largura do tanque no display
const int TANQUE_ALTURA = 128;  // Altura do tanque no display

const int NIVEL_MAX_AGUA_PIXELS = TANQUE_ALTURA - 2; // Nível máximo em pixels dentro da borda
const int NIVEL_MIN_AGUA_PIXELS = 2;  // Nível mínimo em pixels dentro da borda

float faseOnda = 0.0;
float velocidadeOnda = 0.15;
float amplitudeOnda = 1.0;

struct Gota {
  float x, y;
  float vx, vy;
  bool ativa;
};
const int MAX_GOTAS = 20;
Gota gotas[MAX_GOTAS];
float taxaGeracaoGotas = 0.08;
float faseFluxo = 0.0;
float velocidadeFluxo = 0.08;

// --- Variáveis para controle do Buzzer --- 
unsigned long tempoUltimoBeep = 0;
bool estadoBuzzer = false; // Controla se o buzzer está ligado ou desligado no ciclo atual

// --- Setup: Configuração Inicial ---
void setup() {
  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);
  pinMode(PINO_LED_VERDE, OUTPUT);
  pinMode(PINO_LED_AMARELO, OUTPUT);
  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);
  pinMode(PINO_BOTAO_RESET_EEPROM, INPUT_PULLUP);
  pinMode(PINO_BOTAO_MODO, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Iniciando o sistema..."); // Mensagem de inicialização

  Wire.begin();
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf); // Define uma fonte padrão pequena
  u8g2.setFontMode(0); // Modo de fonte transparente (background não é apagado)

  rtc.begin();
  if (!rtc.isrunning()) {
    Serial.println("RTC nao esta funcionando, ajustando hora!");
    // Ajusta o RTC para a data e hora em que o código foi compilado
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Inicializa as gotas da animação como inativas
  for (int i = 0; i < MAX_GOTAS; i++) {
    gotas[i].ativa = false;
  }
  randomSeed(analogRead(0)); // Inicializa o gerador de números aleatórios

  enderecoAtualEEPROM = encontrarProximoEnderecoEEPROM();
  Serial.print("Proximo endereco livre na EEPROM: ");
  Serial.println(enderecoAtualEEPROM);

  // Garante que os LEDs e o buzzer comecem desligados (exceto o verde)
  digitalWrite(PINO_LED_VERDE, HIGH); // Estado normal inicial
  digitalWrite(PINO_LED_AMARELO, LOW);
  digitalWrite(PINO_LED_VERMELHO, LOW);
  noTone(PINO_BUZZER);
}

// --- Leitura do Sensor Ultrassônico ---
float medirDistancia() {
  // Gera um pulso no pino TRIG
  digitalWrite(PINO_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PINO_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PINO_TRIG, LOW);

  // Mede a duração do pulso no pino ECHO
  // Timeout ajustado para ~1.7m (distancia max + margem)
  // 1.7m * 2 / 343 m/s = 0.0099 s = 9900 us. Usar 15000us como timeout.
  long duracaoPulso = pulseIn(PINO_ECHO, HIGH, 15000);

  // Se a duração for 0, houve timeout (sem eco retornado a tempo)
  if (duracaoPulso == 0) {
      // Serial.println("Sensor timeout"); // Mensagem removida conforme solicitado
      return ALTURA_BUEIRO_METROS; // Retorna distância máxima (implica nível 0)
  }

  // Calcula a distância em metros
  // Distancia (cm) = duracaoPulso / 58.3
  float distanciaMetros = (duracaoPulso / 58.3) / 100.0;

  // Garante que a distância não seja maior que a altura do bueiro ou negativa
  if (distanciaMetros > ALTURA_BUEIRO_METROS) distanciaMetros = ALTURA_BUEIRO_METROS;
  if (distanciaMetros < 0) distanciaMetros = 0;

  return distanciaMetros;
}

// --- Atualiza Estado (Normal, Alerta, Crítico) e Indicadores (LEDs, Buzzer) ---
void atualizarEstado(float nivelMetros) {
  EstadoNivel estadoAnterior = estadoAtual;
  unsigned long tempoAtual = millis();

  // Determina o estado atual com base no nível
  if (nivelMetros >= NIVEL_CRITICO_METROS) {
    estadoAtual = CRITICO;
    estadoAtualStr = "Critico";
    digitalWrite(PINO_LED_VERMELHO, HIGH); // Apenas LED vermelho ligado
    digitalWrite(PINO_LED_AMARELO, LOW);
    digitalWrite(PINO_LED_VERDE, LOW);

    // Buzzer: padrão rápido "pipipipipipi"
    // Ciclo de 150ms: 75ms ligado, 75ms desligado
    if (tempoAtual - tempoUltimoBeep >= 75) {
        tempoUltimoBeep = tempoAtual;
        if (estadoBuzzer) {
            noTone(PINO_BUZZER);
            estadoBuzzer = false;
        } else {
            tone(PINO_BUZZER, 1200); // Tom um pouco mais agudo para crítico
            estadoBuzzer = true;
        }
    }

  } else if (nivelMetros >= NIVEL_ALERTA_METROS) {
    estadoAtual = ALERTA;
    estadoAtualStr = "Alerta";
    digitalWrite(PINO_LED_VERMELHO, LOW);
    digitalWrite(PINO_LED_AMARELO, HIGH); // Apenas LED amarelo ligado
    digitalWrite(PINO_LED_VERDE, LOW);

    // Buzzer: padrão "pi-pi-pi"
    // Ciclo de 600ms: 150ms ligado, 450ms desligado
    if (tempoAtual - tempoUltimoBeep >= (estadoBuzzer ? 150 : 450)) {
        tempoUltimoBeep = tempoAtual;
        if (estadoBuzzer) {
            noTone(PINO_BUZZER);
            estadoBuzzer = false;
        } else {
            tone(PINO_BUZZER, 1000); // Tom padrão para alerta
            estadoBuzzer = true;
        }
    }

  } else {
    estadoAtual = NORMAL;
    estadoAtualStr = "Normal";
    digitalWrite(PINO_LED_VERMELHO, LOW);
    digitalWrite(PINO_LED_AMARELO, LOW);
    digitalWrite(PINO_LED_VERDE, HIGH); // Apenas LED verde ligado
    noTone(PINO_BUZZER); // Garante que o buzzer está desligado
    estadoBuzzer = false; // Reseta estado do buzzer
  }

  // --- Lógica para salvar na EEPROM --- 
  // Salva na EEPROM *apenas* quando entra no estado CRÍTICO
  // e se o nível mudou significativamente desde o último registro crítico.
  if (estadoAtual == CRITICO && estadoAnterior != CRITICO) {
     // Verifica se o nível mudou pelo menos 5cm desde o último registro
     // ou se é o primeiro registro crítico desde que saiu desse estado.
     if (ultimoNivelCriticoSalvo < 0 || abs(nivelMetros - ultimoNivelCriticoSalvo) >= 0.05) {
        unsigned long timestampAtual = rtc.now().unixtime();
        salvarDadosEEPROM(timestampAtual, nivelMetros);
        imprimirRegistroLog(timestampAtual, nivelMetros);
        ultimoNivelCriticoSalvo = nivelMetros; // Atualiza o último nível salvo
     }
  } else if (estadoAtual != CRITICO) {
      // Se saiu do estado crítico, reseta a variável para permitir novo registro
      // na próxima vez que entrar em estado crítico.
      ultimoNivelCriticoSalvo = -1.0;
  }
}

// --- Desenho da Animação do Nível da Água ---
void desenharAnimacao(float nivelAtualMetros) {
  u8g2.setDrawColor(1); // Cor branca para desenho
  // Desenha a borda do tanque
  u8g2.drawFrame(TANQUE_X, TANQUE_Y, TANQUE_LARGURA, TANQUE_ALTURA);

  // Calcula o nível da água em pixels, mapeando de metros para a altura do tanque
  int nivelAguaInt = map(constrain(nivelAtualMetros, 0.0, ALTURA_BUEIRO_METROS) * 1000, // Usa milímetros para melhor resolução no map
                        0, ALTURA_BUEIRO_METROS * 1000,
                        NIVEL_MIN_AGUA_PIXELS, NIVEL_MAX_AGUA_PIXELS);

  // Garante que o nível em pixels esteja dentro dos limites mínimo e máximo
  if (nivelAguaInt < NIVEL_MIN_AGUA_PIXELS) nivelAguaInt = NIVEL_MIN_AGUA_PIXELS;
  if (nivelAguaInt > NIVEL_MAX_AGUA_PIXELS) nivelAguaInt = NIVEL_MAX_AGUA_PIXELS;

  // Calcula a coordenada Y da superfície da água e do fundo
  int superficieY = TANQUE_Y + TANQUE_ALTURA - 1 - nivelAguaInt;
  int fundoY = TANQUE_Y + TANQUE_ALTURA - 1;

  // Desenha o corpo da água (retângulo branco preenchido)
  if (nivelAguaInt >= NIVEL_MIN_AGUA_PIXELS) { // Só desenha se houver água visível
      u8g2.drawBox(TANQUE_X + 1, superficieY + 1, TANQUE_LARGURA - 2, nivelAguaInt);

      // Desenha as Ondas na superfície
      for (int x = TANQUE_X + 1; x < TANQUE_X + TANQUE_LARGURA - 1; x++) {
        // Calcula o deslocamento vertical da onda usando seno
        float deslocamentoOnda = sin(faseOnda + (float)(x - TANQUE_X) * 0.15) * amplitudeOnda;
        int ondaY = round(superficieY + deslocamentoOnda);

        if (deslocamentoOnda < 0) { // Crista da onda (acima da linha d'água)
           u8g2.setDrawColor(1); // Desenha em branco
           u8g2.drawVLine(x, ondaY + 1, superficieY - ondaY); // Linha vertical da crista
        } else if (deslocamentoOnda > 0) { // Vale da onda (abaixo da linha d'água)
           u8g2.setDrawColor(0); // Desenha em preto para "apagar" o branco
           int alturaVale = ondaY - superficieY;
           // Garante que não apague abaixo do corpo d'água
           if (superficieY + 1 + alturaVale <= fundoY) {
               u8g2.drawVLine(x, superficieY + 1, alturaVale);
           }
        }
      }
      u8g2.setDrawColor(1); // Restaura a cor para branco

      // Desenha Linhas de Fluxo dentro da água
      if (nivelAguaInt > 10) { // Só desenha se o nível for um pouco maior
          int numLinhasFluxo = nivelAguaInt / 15;
          for (int i = 0; i < numLinhasFluxo; i++) {
            // Calcula a posição X da linha usando cosseno para movimento lateral
            float fluxoX = TANQUE_X + TANQUE_LARGURA / 2.0 + cos(faseFluxo + i * 0.8) * (TANQUE_LARGURA * 0.25);
            // Distribui as linhas verticalmente dentro da água
            float fluxoY = superficieY + 6 + (float)i * (nivelAguaInt - 12) / max(1, numLinhasFluxo);
            // Desenha apenas se estiver visivelmente dentro da água
            if (fluxoY < fundoY - 2 && fluxoY > superficieY + 2) {
               // Desenha pequenos traços ou pixels para simular o fluxo
               u8g2.drawPixel(round(fluxoX - 1), round(fluxoY));
               u8g2.drawPixel(round(fluxoX), round(fluxoY));
               u8g2.drawPixel(round(fluxoX + 1), round(fluxoY));
            }
          }
      }
  }

  // Desenha as Gotas (por último, para ficarem sobre a água)
  u8g2.setDrawColor(1);
  for (int i = 0; i < MAX_GOTAS; i++) {
    if (gotas[i].ativa) {
      u8g2.drawPixel(round(gotas[i].x), round(gotas[i].y));
    }
  }
}

// --- Atualização do Estado da Animação --- 
void atualizarAnimacao(float nivelAtualMetros) {
  // Atualiza a fase da onda para movimento contínuo
  faseOnda += velocidadeOnda;
  // A amplitude da onda aumenta com o nível da água
  amplitudeOnda = 0.5 + (constrain(nivelAtualMetros, 0.0, ALTURA_BUEIRO_METROS) / ALTURA_BUEIRO_METROS) * 2.0;
  // Atualiza a fase do fluxo interno
  faseFluxo += velocidadeFluxo;

  // --- Atualiza Posição e Estado das Gotas --- 
  float gravidade = 0.18;
  // Recalcula o nível e a superfície em pixels para colisões
  int nivelAguaInt = map(constrain(nivelAtualMetros, 0.0, ALTURA_BUEIRO_METROS) * 1000, 0, ALTURA_BUEIRO_METROS * 1000, NIVEL_MIN_AGUA_PIXELS, NIVEL_MAX_AGUA_PIXELS);
  if (nivelAguaInt < NIVEL_MIN_AGUA_PIXELS) nivelAguaInt = NIVEL_MIN_AGUA_PIXELS;
  int superficieY = TANQUE_Y + TANQUE_ALTURA - 1 - nivelAguaInt;

  for (int i = 0; i < MAX_GOTAS; i++) {
    if (gotas[i].ativa) {
      // Atualiza posição com base na velocidade
      gotas[i].x += gotas[i].vx;
      gotas[i].y += gotas[i].vy;
      // Aplica gravidade à velocidade vertical
      gotas[i].vy += gravidade;

      // Condições para desativar a gota
      bool caiuNaAgua = gotas[i].x > TANQUE_X && gotas[i].x < TANQUE_X + TANQUE_LARGURA - 1 && gotas[i].y >= superficieY;
      bool bateuNoFundo = gotas[i].y >= TANQUE_Y + TANQUE_ALTURA - 1;
      bool bateuNaLateral = gotas[i].x <= TANQUE_X || gotas[i].x >= TANQUE_X + TANQUE_LARGURA - 1;
      bool saiuPorCima = gotas[i].y < TANQUE_Y;

      if (caiuNaAgua || bateuNoFundo || bateuNaLateral || saiuPorCima) {
         gotas[i].ativa = false;
      }
    }
  }

  // --- Gera Novas Gotas --- 
  // A chance de gerar gotas aumenta com o nível da água
  float chanceGerarGota = taxaGeracaoGotas * (0.1 + constrain(nivelAtualMetros, 0.0, ALTURA_BUEIRO_METROS) / ALTURA_BUEIRO_METROS);
  // Gera uma gota aleatoriamente com base na chance calculada
  if (random(0, 1000) / 1000.0 < chanceGerarGota && nivelAtualMetros > 0.05) { // Só gera se tiver > 5cm de água
    // Procura um slot de gota inativo para reutilizar
    for (int i = 0; i < MAX_GOTAS; i++) {
      if (!gotas[i].ativa) {
        gotas[i].ativa = true;
        // Define a posição inicial (lateral esquerda ou direita, perto da superfície)
        bool nasceNaEsquerda = random(0, 2) == 0;
        gotas[i].x = nasceNaEsquerda ? (TANQUE_X + 1.0 + random(0,2)) : (TANQUE_X + TANQUE_LARGURA - 2.0 - random(0,2));
        gotas[i].y = superficieY + random(-1, 4);
        // Define a velocidade inicial (horizontal para fora, vertical para cima)
        gotas[i].vx = (nasceNaEsquerda ? 1.0 : -1.0) * (random(50, 150) / 100.0);
        gotas[i].vy = -1.0 * (random(100, 200) / 100.0); // Saltos um pouco menores
        break; // Gera apenas uma gota por vez
      }
    }
  }
}

// --- Desenho do Texto de Status no Display ---
void desenharTextoStatus(float nivelAguaMetros, const char* estadoStr) {
    u8g2.setFont(u8g2_font_ncenB10_tr); // Usa uma fonte um pouco maior para o status
    u8g2.setCursor(5, 30); // Posição do texto do nível
    u8g2.print("Nivel: ");
    u8g2.print(nivelAguaMetros, 2); // Exibe com duas casas decimais
    u8g2.print(" m");

    u8g2.setCursor(5, 70); // Posição do texto do estado
    u8g2.print("Estado: ");
    u8g2.print(estadoStr);

    u8g2.setFont(u8g2_font_6x10_tf); // Restaura a fonte padrão pequena
}

// --- Funções da EEPROM --- 

// Salva um registro (timestamp e nível) na EEPROM
void salvarDadosEEPROM(unsigned long timestamp, float nivelMetros) {
  // Verifica se há espaço na EEPROM
  if (enderecoAtualEEPROM + TAMANHO_REGISTRO_EEPROM > EEPROM.length()) {
      Serial.println("EEPROM cheia! Nao foi possivel salvar.");
      // Poderia implementar lógica de sobrescrever o mais antigo (buffer circular)
      return; // Simplesmente não salva se estiver cheia
  }

  // Salva o timestamp (4 bytes)
  EEPROM.put(enderecoAtualEEPROM, timestamp);

  // Converte o nível de metros para milímetros (uint16_t) para melhor precisão
  uint16_t nivel_mm = constrain(nivelMetros * 1000, 0, 65535);
  // Salva o nível em mm (2 bytes)
  EEPROM.put(enderecoAtualEEPROM + 4, nivel_mm);

  Serial.print("Salvando Nivel CRITICO na EEPROM - Endereco: ");
  Serial.print(enderecoAtualEEPROM);
  Serial.print(" | Timestamp: ");
  Serial.print(timestamp);
  Serial.print(" | Nivel (m): ");
  Serial.println(nivelMetros, 2);

  // Avança para o próximo endereço disponível
  enderecoAtualEEPROM += TAMANHO_REGISTRO_EEPROM;
}

// Imprime um registro de log no Monitor Serial
void imprimirRegistroLog(unsigned long timestamp, float nivelMetros) {
  // Converte o timestamp UTC para data/hora local (considerando o fuso)
  DateTime dt = DateTime(timestamp);
  char bufferDataHora[25];
  // Formata a data e hora
  sprintf(bufferDataHora, "%02d/%02d/%04d %02d:%02d:%02d",
          dt.day(), dt.month(), dt.year(),
          dt.hour(), dt.minute(), dt.second());

  Serial.print(bufferDataHora);
  Serial.print(" - Nivel Critico Registrado: ");
  Serial.print(nivelMetros, 2);
  Serial.println(" m");
}

// Encontra o próximo endereço livre na EEPROM ao iniciar
int encontrarProximoEnderecoEEPROM() {
  // Procura pelo primeiro registro cujo timestamp seja 0xFFFFFFFF (valor padrão de bytes não escritos)
  for (int addr = 0; addr < EEPROM.length(); addr += TAMANHO_REGISTRO_EEPROM) {
    unsigned long timestampSalvo;
    EEPROM.get(addr, timestampSalvo);
    // Se encontrar um timestamp inválido (0xFFFFFFFF), significa que este slot está livre
    if (timestampSalvo == 0xFFFFFFFF) {
      return addr; // Retorna o endereço do slot livre
    }
  }
  // Se percorreu toda a EEPROM e não achou slot livre
  Serial.println("EEPROM cheia ou nao inicializada. Iniciando do endereco 0.");
  return 0; // Retorna 0 (pode sobrescrever ou indicar que está cheia)
}

// Limpa (reseta) toda a EEPROM, preenchendo com 0xFF
void resetarEEPROM() {
  Serial.println("Limpando a EEPROM...");
  for (int i = 0; i < EEPROM.length(); i++) {
    // Escreve 0xFF apenas se o valor atual for diferente, para economizar ciclos de escrita
    if (EEPROM.read(i) != 0xFF) {
        EEPROM.write(i, 0xFF);
    }
  }
  enderecoAtualEEPROM = 0; // Reseta o ponteiro de endereço
  ultimoNivelCriticoSalvo = -1.0; // Reseta o controle de último nível salvo
  Serial.println("Limpeza da EEPROM concluida.");
}

// --- Loop Principal --- 
void loop() {
  unsigned long tempoAtualMs = millis(); // Obtém o tempo atual em milissegundos

  // 1. Leitura do Sensor
  float distanciaSensor = medirDistancia();
  // Calcula o nível da água (invertendo a distância e limitando à altura do bueiro)
  float nivelAguaAtual = ALTURA_BUEIRO_METROS - distanciaSensor;
  if (nivelAguaAtual < 0) nivelAguaAtual = 0;
  if (nivelAguaAtual > ALTURA_BUEIRO_METROS) nivelAguaAtual = ALTURA_BUEIRO_METROS;

  // 2. Atualização do Estado e Indicadores
  // (Controla LEDs, Buzzer, variável de estado e salva na EEPROM se necessário)
  atualizarEstado(nivelAguaAtual);

  // 3. Verificação do Botão de Modo de Exibição (Pino 7)
  bool estadoBotaoModo = digitalRead(PINO_BOTAO_MODO);
  static bool ultimoEstadoBotaoModo = HIGH;
  static unsigned long tempoInicioPressaoBotaoModo = 0;

  // Detecta a borda de descida (botão pressionado)
  if (estadoBotaoModo == LOW && ultimoEstadoBotaoModo == HIGH) {
      tempoInicioPressaoBotaoModo = tempoAtualMs;
  }

  // Detecta a borda de subida (botão liberado)
  if (estadoBotaoModo == HIGH && ultimoEstadoBotaoModo == LOW) {
      // Verifica se o tempo pressionado foi maior que o debounce
      if ((tempoAtualMs - tempoInicioPressaoBotaoModo) > ATRASO_DEBOUNCE_MS) {
          modoExibicaoAnimacao = !modoExibicaoAnimacao; // Inverte o modo de exibição
          tempoUltimoAcionamentoBotao = tempoAtualMs; // Atualiza o tempo do último acionamento válido
          Serial.print("Modo de Exibicao Alterado: ");
          Serial.println(modoExibicaoAnimacao ? "Animacao" : "Texto");
      }
  }
  ultimoEstadoBotaoModo = estadoBotaoModo; // Guarda o estado atual para a próxima iteração

  // 4. Atualização do Estado da Animação (Calcula próximo frame se estiver no modo animação)
  if (modoExibicaoAnimacao) {
      atualizarAnimacao(nivelAguaAtual);
  }

  // 5. Desenho no Display OLED
  u8g2.firstPage(); // Inicia o ciclo de desenho do U8g2
  do {
    // Desenha a animação ou o texto de status, dependendo do modo
    if (modoExibicaoAnimacao) {
      desenharAnimacao(nivelAguaAtual);
    } else {
      desenharTextoStatus(nivelAguaAtual, estadoAtualStr);
    }
  } while (u8g2.nextPage()); // Continua desenhando até completar o buffer do display

  // 6. Verificação do Botão de Reset da EEPROM (Pino 4)
  bool estadoBotaoReset = digitalRead(PINO_BOTAO_RESET_EEPROM);
  static bool ultimoEstadoBotaoReset = HIGH;
  static unsigned long tempoInicioPressaoBotaoReset = 0;

  // Detecta a borda de descida (botão pressionado)
   if (estadoBotaoReset == LOW && ultimoEstadoBotaoReset == HIGH) {
       tempoInicioPressaoBotaoReset = tempoAtualMs;
   }

   // Detecta a borda de subida (botão liberado)
   if (estadoBotaoReset == HIGH && ultimoEstadoBotaoReset == LOW) {
       // Verifica se o tempo pressionado foi maior que o debounce
       if ((tempoAtualMs - tempoInicioPressaoBotaoReset) > ATRASO_DEBOUNCE_MS) {
            resetarEEPROM(); // Chama a função para limpar a EEPROM
            // Exibe mensagem de confirmação no OLED
            u8g2.firstPage();
            do {
              u8g2.setFont(u8g2_font_ncenB08_tr); // Usa uma fonte um pouco maior para a mensagem
              u8g2.drawStr(10, 60, "EEPROM Limpa!");
            } while (u8g2.nextPage());
            u8g2.setFont(u8g2_font_6x10_tf); // Restaura a fonte padrão
            delay(2000); // Pausa para exibir a mensagem
            tempoUltimoAcionamentoBotao = tempoAtualMs; // Atualiza o tempo do último acionamento válido
       }
   }
   ultimoEstadoBotaoReset = estadoBotaoReset; // Guarda o estado atual

  // 7. Pequeno Atraso no Loop
  // Controla a taxa de atualização geral (leitura do sensor, atualização do display, etc.)
  delay(40); // Aproximadamente 25 frames por segundo / leituras por segundo
}

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
