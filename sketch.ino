#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define led_verde 41 // Pino utilizado para controle do led verda
#define led_vermelho 40 // Pino utilizado para controle do led vermelho
#define led_amarelo 9 // Pino utilizado para controle do led azul

const int pinoBotao = 18;  // Pino do botão push
int estadoBotao = 0;  // Variável que armazena o estado atual do botão
int ultimoEstadoBotao = 0; // Variável que armazena o último estado do botão 
int vezesBotaoPressionado = 0; // Quantas vezes o botão foi pressionado
long ultimoPeriodoDebounce = 0;  // Tempo desde a última vez que o botão foi clicado
long periodoDebounce = 50;    // Período de debounce

const int pinoLdr = 4;  // Pino do sensor de luminosidade (ldr)
int threshold=600; // Claridade mínima para ligar o led

void requisicao_http() {
  if(WiFi.status() == WL_CONNECTED){ // Se o ESP32 estiver conectado à Internet
    HTTPClient http;

    String caminhoServidor = "http://www.google.com.br/"; // Endpoint da requisição HTTP

    http.begin(caminhoServidor.c_str()); // Define a rota e a URL da requisição

    int httpCodigoResposta = http.GET(); // Código do Resultado da Requisição HTTP

    // Caso tenha recebido código de resposta, requisição foi um sucesso e pega o payload
    if (httpCodigoResposta>0) {
      Serial.print("Código de resposta HTTP: ");
      Serial.println(httpCodigoResposta);
      String payload = http.getString();
      Serial.println(payload);
    } else { // Caso não receba o código de resposta, mostre o código de erro
      Serial.print("Código de erro: ");
      Serial.println(httpCodigoResposta);
    }

    http.end(); // Encerra a requisição e a comunicação por HTTP

  } else {
    Serial.println("WiFi desconectado");
  }
}

// Função para ler o valor do botão com debounce
bool ler_valor_botao() {
  estadoBotao = digitalRead(pinoBotao);

  // Cria um delay com millis(), visando evitar a leitura incorreta do estado do botão
  if ((millis() - ultimoPeriodoDebounce) > periodoDebounce) {
    delay(1);
  }

  return estadoBotao;
}

void setup() {
  // Configuração inicial dos pinos para controle dos leds como OUTPUTs (saídas) do ESP32
  pinMode(led_amarelo,OUTPUT);
  pinMode(led_verde,OUTPUT);
  pinMode(led_vermelho,OUTPUT);

  // Inicialização do botão
  pinMode(pinoBotao, INPUT_PULLUP); // Inicializa o botão como input

  // Inicialização do sensor de luminosidade  
  pinMode(pinoLdr, INPUT);

  // Desliga todos os leds
  digitalWrite(led_amarelo, LOW);
  digitalWrite(led_verde, LOW);
  digitalWrite(led_vermelho, LOW);

  Serial.begin(9600); // Configuração para debug por interface serial entre ESP e computador com baud rate de 9600

  WiFi.begin("Wokwi-GUEST", ""); // Conexão à rede WiFi aberta com SSID Wokwi-GUEST

  while (WiFi.status() != WL_CONNECT_FAILED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("Conectado ao WiFi com sucesso!"); // Considerando que saiu do loop acima, o ESP32 agora está conectado ao WiFi (outra opção é colocar este comando dentro do if abaixo)

  // Verifica estado do botão
  estadoBotao = ler_valor_botao();
  if (estadoBotao == 1) {
    Serial.println("Botão pressionado!");
  } else {
    Serial.println("Botão não pressionado!");
  }
}

// Função para fazer o delay com millis
void delay_com_millis(long periodo) {
  long periodoAnterior = 0;

  if (millis() >= periodoAnterior + periodo) {
    periodoAnterior = millis(); 
  }
}

// Função para o semáforo no modo noturno
void semaforo_noturno() {
  // Desliga todos os leds
  digitalWrite(led_verde, LOW);
  digitalWrite(led_amarelo, LOW);
  digitalWrite(led_vermelho, LOW);

  digitalWrite(led_amarelo, HIGH); // liga o led amarelo
  delay(1000); // espera um segundo
  digitalWrite(led_amarelo, LOW); // desliga o led amarelo
  delay(1000); // espera um segundo
}

// Função para o semáforo convencional
void semaforo_convencional() {
  // Liga o led verde e desliga os outros
  digitalWrite(led_verde, HIGH);
  digitalWrite(led_amarelo, LOW);
  digitalWrite(led_vermelho, LOW);

  delay(3000); // espera 3 segundos

  // Liga o led amarelo e desliga os outros
  digitalWrite(led_verde, LOW);
  digitalWrite(led_amarelo, HIGH);
  digitalWrite(led_vermelho, LOW);

  delay(2000); // espera dois segundos


  // Liga o led vermelho e desliga os outros
  digitalWrite(led_verde, LOW);
  digitalWrite(led_amarelo, LOW);
  digitalWrite(led_vermelho, HIGH);

  vezesBotaoPressionado = 0;
  estadoBotao = ler_valor_botao();

  unsigned long periodoAnterior = 0;
  unsigned long periodo = 50;

  // A cada 50 milisegundos, verifica o estado do botão
  if (millis() >= periodoAnterior + periodo) {
    periodoAnterior = millis();
    estadoBotao = ler_valor_botao();

    // Contabiliza quantas vezes o botão foi pressionado caso ele tenha sido pressionado
    if (estadoBotao == 1) {
      vezesBotaoPressionado += 1;
    }
  }

  // Se o botão for pressionado 3 vezes, faz a requisição http
  if (vezesBotaoPressionado == 3) {
    requisicao_http();
  }

  // Se o botão tiver sido pressionado uma vez, pula o led vermelho
  if (vezesBotaoPressionado == 1) {
    goto cancelarLedVermelho;
  }

  delay(5000); // espera 5 segundos
  cancelarLedVermelho: // Caso o botão tenha sido clicado uma vez
    delay(1000); // espera 1 segundo antes de voltar ao led verde
}

void loop() {
  // Lê o valor do sensor de luminosidade
  int ldrstatus = analogRead(pinoLdr);

  // caso o valor lido seja menor ou igual ao limite de luminosidade
  if(ldrstatus<=threshold){
    Serial.print("Está escuro, ligue o led!");
    Serial.println(ldrstatus);

    // enquanto o ldr ainda detecta que está escuro, mantém o semáforo no modo noturno
    while (ldrstatus<=threshold) {
      semaforo_noturno();
    }
  }else{ // Caso o valor lido seja maior ou igual ao limite de luminosidade
    Serial.print("Está claro, desligue o led!");
    Serial.println(ldrstatus);

    // enquanto o ldr ainda detecta que está claro, mantém o semáforo no modo convencional
    while (ldrstatus >= threshold) {
      semaforo_convencional();
    }
  }
}