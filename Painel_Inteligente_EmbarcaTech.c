//******* Incluindo Display OLED 

//Incluindo bibliotecas
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include "pico/binary_info.h"    // Biblioteca para informações binárias
#include "inc/ssd1306.h"         // Biblioteca para controlar o display OLED SSD1306
#include "hardware/i2c.h"        // Biblioteca para trabalhar com I2C

//Definicação de pinos e variáveis
#define LED_RED 13
#define LED_BLUE 12
#define LED_GREEN 11
#define BUZZER_PIN 10
#define SET 1
#define RESET 0

#define NOTE_C5 523               // Frequência da nota C5 em Hz - Buzzer
#define WIFI_SSID "Zandoni"      // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "Pipoca@2108" // Substitua pela senha da sua rede Wi-Fi

// Definindo os pinos do I2C
const uint I2C_SDA = 14;  // Pino SDA para comunicação I2C
const uint I2C_SCL = 15;  // Pino SCL para comunicação I2C

//***Declaração de variáveis globais
// Buffer para resposta HTTP
char http_response[1024];
//Exibir última mensagem e categoria
char ultima_mensagem[256] = "";
char ultima_categoria[50] = "Nenhuma";

// Função para iniciar a comunicação I2C
void iniciar_comunicacao_i2c() 
{
	i2c_init(i2c1, ssd1306_i2c_clock * 1000);  // Inicializa o I2C com a frequência definida em ssd1306_i2c_clock
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configura o pino SDA para a função I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configura o pino SCL para a função I2C
    gpio_pull_up(I2C_SDA);  // Habilita o resistor pull-up para o pino SDA
    gpio_pull_up(I2C_SCL);  // Habilita o resistor pull-up para o pino SCL
}

// Função para iniciar o display OLED SSD1306
void iniciar_display() 
{
    ssd1306_init();  // Inicializa o display OLED SSD1306
}


// Função para atualizar o display OLED com a mensagem e a categoria
void atualizar_display(const char* mensagem, const char* categoria) {

    // Configura a área de renderização do display
    struct render_area frame_area = {
        start_column : 0,                 // Coluna inicial
        end_column : ssd1306_width - 1,  // Coluna final (largura da tela)
        start_page : 0,                 // Página inicial (linha inicial)
        end_page : ssd1306_n_pages - 1 // Página final (total de páginas para a altura da tela)
    };
    // Calcula o comprimento do buffer de renderização
    calculate_render_area_buffer_length(&frame_area);  

    // Cria e limpa o buffer do display
    uint8_t ssd[ssd1306_buffer_length];  // Cria um buffer de memória para o display
    memset(ssd, 0, ssd1306_buffer_length);  // Preenche o buffer com zeros

    // Desenha a mensagem e a categoria no display
    ssd1306_draw_string(ssd, 0, 0, mensagem);
    ssd1306_draw_string(ssd, 0, 8, categoria);
    render_on_display(ssd, &frame_area);  // Renderiza o buffer no display
}



// Função para criar servidor de mensagens HTTP
void create_http_response() {
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
        "<!DOCTYPE html>"
        "<html lang=\"pt\">"
        "<head>"
        "  <meta charset=\"UTF-8\">"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "  <title>Painel de Notificações - EmbarcaTech</title>"
        "  <style>"
        "    body { font-family: Arial, sans-serif; text-align: center; padding: 20px; background-color: #f4f4f4; }"
        "    .container { max-width: 400px; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); margin: auto; }"
        "    h1 { color: #333; }"
        "    input, button { width: 100%%; padding: 10px; margin: 10px 0; border-radius: 5px; border: 1px solid #ddd; }"
        "    button { background-color: #28a745; color: white; font-size: 16px; border: none; cursor: pointer; }"
        "    button:hover { background-color: #218838; }"
        "  </style>"
        "</head>"
        "<body>"
        "  <div class=\"container\">"
        "    <h1>Denúncias Anônimas</h1>"
        "    <p>Status: %s - %s</p>"
        "    <input type=\"text\" id=\"mensagem\" placeholder=\"Digite sua mensagem...\">"
        "    <button onclick=\"enviarMensagem()\">Enviar</button>"
        "  </div>"
        "  <script>"
        "    function enviarMensagem() {"
        "      var msg = document.getElementById('mensagem').value;"
        "      if(msg.trim() !== '') {"
        "        fetch('/enviar?msg=' + encodeURIComponent(msg))"
        "          .then(response => {"
        "            if (response.ok) {"
        "              return response.text();"
        "            } else {"
        "              throw new Error('Erro na resposta do servidor');"
        "            }"
        "          })"
        "          .then(data => {"
        "            console.log('Resposta do servidor:', data);"
        "            alert('Mensagem enviada!');"
        "            document.getElementById('mensagem').value = '';"
        "          })"
        "          .catch(error => {"
        "            console.error('Erro ao enviar a mensagem:', error);"
        "            alert('Erro ao enviar a mensagem!');"
        "          });"
        "      } else {"
        "        alert('Por favor, digite uma mensagem!');"
        "      }"
        "    }"
        "  </script></body></html>\r\n",
        ultima_mensagem, ultima_categoria);
}
                              
             

// Função para tentar conectar ao Wi-Fi com tentativas
int connect_wifi() {
    int retries = 5;
    while (retries > 0) {
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
            printf("Falha ao conectar ao Wi-Fi. Tentando novamente...\n");
            retries--;
            sleep_ms(5000);
        } else {
            printf("Wi-Fi conectado!\n");
            return 0; // Sucesso
        }
    }
    return 1; // Falhou após várias tentativas
}

//* Função para configurar os LEDs
void init_leds() {
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, RESET);  // LED vermelho apagado inicialmente

    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    gpio_put(LED_BLUE, RESET);  // LED azul apagado inicialmente

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, RESET);  // LED verde apagado inicialmente
}

//* Função para piscar o LED vermelho
void piscar_led_vermelho() {
    gpio_put(LED_RED, SET);
    sleep_ms(500);
    gpio_put(LED_RED, RESET);
    sleep_ms(500);
}

//* Função para piscar o LED azul
void piscar_led_azul() {
    gpio_put(LED_BLUE, SET);
    sleep_ms(100);
    gpio_put(LED_BLUE, RESET);
    sleep_ms(100);
}

//* Função para piscar o LED verde
void piscar_led_verde() {
    gpio_put(LED_GREEN, SET);
    sleep_ms(100);
    gpio_put(LED_GREEN, RESET);
    sleep_ms(100);
}

//* Função para iniciar o buzzer
void init_buzzer() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);
}

// Função para converter caracteres acentuados para equivalentes sem acento
char* remover_acentos(char* str) {
    static char buffer[256];
    int i, j = 0;
    char *original = "áàãâäéèêëíìîïóòõôöúùûüçñÁÀÃÂÄÉÈÊËÍÌÎÏÓÒÕÔÖÚÙÛÜÇÑ";
    char *sem_acento = "aaaaaeeeeiiiiooooouuuucnAAAAAEEEEIIIIOOOOOUUUUCN";

    for (i = 0; str[i] != '\0' && j < 255; i++) {
        char *pos = strchr(original, str[i]);
        if (pos) {
            buffer[j++] = sem_acento[pos - original];
        } else {
            buffer[j++] = str[i];
        }
    }
    buffer[j] = '\0';
    return buffer;
}

// Função para converter string para minúsculas
void to_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

// Função para normalizar texto removendo acentos e convertendo para minúsculas
void normalizar_texto(char* str) {
    char temp[256];
    strcpy(temp, remover_acentos(str));
    to_lowercase(temp);
    strcpy(str, temp);
}

// Função para analisar a categoria da mensagem
const char* analisar_categoria(const char* mensagem) {
    static char mensagem_normalizada[256];
    strcpy(mensagem_normalizada, mensagem);
    normalizar_texto(mensagem_normalizada);

    // Palavras-chave para cada categoria
    // URGENTE (Alerta Vermelho)
    const char* prioridade_vermelha[] = {
        "emergencia", "socorro", "acidente", "morte", "colapso", "parada cardiaca",
        "perigo", "queimadura", "grave", "hemorragia", "desmaio", "fratura", "asfixia",
        "infarto", "dano cerebral", "trauma", "afogamento", "homicidio", "suicidio", "tortura",
        "respiratorio", "respiratoria", "panico", "vitima", "colisao",
        "tiro", "arma", "desespero", "envenenamento"
    };

    // ALERTA (Alerta Amarelo)
    const char* prioridade_amarela[] = {
        "risco", "queda de energia", "queda de arvore", "chuvas fortes", "tempestade", "alagamento",
        "interrupcao de servicos", "bloqueio de estrada", "acidente de transito", "inundacao",
        "ameaca de tempestade", "vazamento de gas", "desabamento", "vazamento de agua",
        "falta de iluminacao", "risco de incendio", "queda de estrutura", "perigo de contaminacao"
    };

    // NOTIFICAÇÃO (Alerta Verde)
    const char* prioridade_verde[] = {
        "informacao", "aviso de problema", "atualizacao", "relato", "confirmacao", "feedback",
        "verificacao", "monitoramento", "acompanhamento", "denuncia", "relatorio", 
        "solicitacao de informacao", "observacao", "analise", "notificacao",
        "registro", "comunicacao de falha", "acompanhamento", "revisao"
    };

    // Verifica palavras-chave da categoria "URGENTE"
    for (int i = 0; i < sizeof(prioridade_vermelha) / sizeof(prioridade_vermelha[0]); i++) {
        if (strstr(mensagem_normalizada, prioridade_vermelha[i]) != NULL) {
            for (int j = 0; j < 3; j++) {
                piscar_led_vermelho();  // Pisca o LED vermelho 3x
            }
            gpio_put(BUZZER_PIN, SET);   // Ativa o buzzer
            sleep_ms(3000);              // Mantém por 3 segundos
            gpio_put(BUZZER_PIN, RESET); // Desliga o buzzer
            return "URGENTE (Alerta Vermelho)";
        }
    }

    // Verifica palavras-chave da categoria "ALERTA"
    for (int i = 0; i < sizeof(prioridade_amarela) / sizeof(prioridade_amarela[0]); i++) {
        if (strstr(mensagem_normalizada, prioridade_amarela[i]) != NULL) {
            for (int j = 0; j < 3; j++) {
                piscar_led_azul();  // Pisca o LED azul 3x
                piscar_led_vermelho();  // Pisca o LED azul 3x
            }
            return "ALERTA (Alerta Amarelo)";
        }
    }

    // Verifica palavras-chave da categoria "NOTIFICAÇÃO"
    for (int i = 0; i < sizeof(prioridade_verde) / sizeof(prioridade_verde[0]); i++) {
        if (strstr(mensagem_normalizada, prioridade_verde[i]) != NULL) {
            for (int j = 0; j < 3; j++) {
                piscar_led_verde();  // Pisca o LED verde 3x
            }
            return "NOTIFICAÇÃO (Alerta Verde)";
        }
    }

    return "SEM CATEGORIA";
}

// Função de callback para processar requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        // Cliente fechou a conexão
        tcp_close(tpcb);
        return ERR_OK;
    }
    // Exibir a requisição recebida
    printf("Requisição recebida:\n%s\n", (char *)p->payload);
    char *request = (char *)p->payload; // Processa a requisição HTTP - Processamento categoria da mensagem

    // Se a requisição contém "/enviar?msg="
    if (strstr(request, "GET /enviar?msg=") != NULL) {
        char *start = strstr(request, "msg=") + 4;
        char *end = strstr(start, " ");
      
        if (end){
            *end = '\0';
       }

       // Verifica se a mensagem foi extraída corretamente
       printf("Mensagem recebida (HTTP): %s\n", start);

        strncpy(ultima_mensagem, start, sizeof(ultima_mensagem) - 1);
        ultima_mensagem[sizeof(ultima_mensagem) - 1] = '\0';

        // Classificar a mensagem e imprime categoria
        strncpy(ultima_categoria, analisar_categoria(ultima_mensagem), sizeof(ultima_categoria) - 1);
        ultima_categoria[sizeof(ultima_categoria) - 1] = '\0';
        printf("Categoria atribuída (HTTP): %s\n", ultima_categoria);

        // Atualiza o display com a mensagem recebida e sua categoria
        atualizar_display(ultima_mensagem, ultima_categoria);
    }

    //atualiza status da página
    create_http_response(); 

    // Envia a resposta HTTP
    tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);
    pbuf_free(p);     // Libera o buffer recebido

    return ERR_OK;
}

// Callback de conexão: associa o http_callback à conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);  // Associa o callback HTTP
    return ERR_OK;
}

// Função de setup do servidor TCP
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);  // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback);  // Associa o callback de conexão
    printf("Servidor HTTP rodando na porta 80...\n");
}

// Loop principal
int main()
{
    stdio_init_all();
    iniciar_comunicacao_i2c();
    iniciar_display();
    init_leds();
    init_buzzer();
    sleep_ms(10000);
    printf("Iniciando servidor HTTP\n");

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);


    // Exibir mensagem de boas-vindas no display
    char *text[] = {
        "Sistema de Notificacao",
        "Inteligente",
        "Bem-vindo!"
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);

    
	sleep_ms(3000);  // Aguarda 3 segundos

    // Inicializa o Wi-Fi
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (connect_wifi()) {
        printf("Falha ao conectar ao Wi-Fi após várias tentativas.\n");
        return 1;
    } else {
        printf("Wi-Fi conectado!\n");

         // Apresenta o endereço de IP do SERVIDOR
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
       
        // Inicia o servidor HTTP
        start_http_server();
        printf("Servidor HTTP iniciado com sucesso!\n");
    }
    while (true) {
        cyw43_arch_poll();  // Necessário para manter o Wi-Fi ativo
        sleep_ms(100);

        char mensagem[256];    
        printf("Aguardando mensagem...\n");
        fgets(mensagem, sizeof(mensagem), stdin);
        mensagem[strcspn(mensagem, "\n")] = 0; // Remover a quebra de linha
        
        printf("Mensagem recebida: %s\n", mensagem);
        printf("Normalizando texto (removendo acentos e convertendo para minúsculas)...\n");
        normalizar_texto(mensagem);
        printf("Texto normalizado: %s\n", mensagem);

        printf("Analisando categoria da mensagem...\n");
        const char* categoria = analisar_categoria(mensagem);
        printf("Categoria indentificada: %s\n", categoria);

        // Atualiza o display com a mensagem e sua categoria
        atualizar_display(mensagem, categoria);
        
        if (strstr(categoria, "URGENTE") != NULL) {
            printf("Categoria URGENTE detectada!\n");
            for (int i = 0; i < 3; i++) {
                piscar_led_vermelho();
            }
            gpio_put(BUZZER_PIN, SET);
            sleep_ms(3000);
            gpio_put(BUZZER_PIN, RESET);
        }
        else if (strstr(categoria, "ALERTA") != NULL) {
                printf("Categoria ALERTA detectada!\n");
                piscar_led_vermelho();
                piscar_led_verde();
            
        }
        else if (strstr(categoria, "NOTIFICAÇÃO") != NULL) {
                printf("Categoria NOTIFICAÇÃO detectada!\n");
                piscar_led_verde();
        }
    }
    return 0;
  }

