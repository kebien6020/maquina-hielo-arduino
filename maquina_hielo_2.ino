#include <Nextion.h>
#include <stdlib.h>
#include <EEPROM.h>
#include <max6675.h>
#include <ModbusMaster.h>

#include "src/NexDualButton.h"
#include "src/NexNumber.h"

// Entradas
constexpr int PIN_FLOTADOR = 2;

constexpr int PIN_FLT_TK_ALTO = 3;
constexpr int PIN_FLT_TK_BAJO = 4;
// Salidas
constexpr int PIN_CONTACTOR_PRINCIPAL = 11;
constexpr int PIN_DEFROST = 10;
constexpr int PIN_FAN = 9;
constexpr int PIN_BOMBA = 8;
constexpr int PIN_SOLENOIDE_TK_ALMACENAMIENTO = 7;
constexpr int PIN_BOMBA_TK_ALMACENAMIENTO = 5;
constexpr int PIN_LLENADO = 6;
constexpr int PIN_MOTOR = A1;
constexpr int PIN_DIRECCION_RS485_1 = 22;
constexpr int PIN_DIRECCION_RS485_2 = 24;

// Tiempos
constexpr auto TIEMPO_MODO_INICIO_CICLO = 60000ul;
constexpr auto TIEMPO_BOMBA_INICIO = 5000ul;
constexpr auto TIEMPO_FINAL_DE_CICLO = 4ul * 60ul * 1000ul;
constexpr auto TIEMPO_DEFROST = 6ul * 60ul * 1000ul + 20ul * 1000ul;
constexpr auto TIEMPO_MUESTRA_CONTROL_FRIO = 50ul;
constexpr auto TIEMPO_ARRANQUE_CONTACTOR = 10000ul;

constexpr auto TIEMPO_ALARMA_LLENADO = 5ul * 60ul * 1000ul;
constexpr auto TIEMPO_ALARMA_LLENADO_TK_ALAMCENAMIENTO = 10ul * 60ul * 1000ul;
constexpr auto TIEMPO_ALARMA_DOS_CICLOS = 20ul * 60ul * 1000ul;

constexpr auto TIEMPO_SOLENOIDE_TK_ALMACENAMIENTO = 10000ul;

// Configuraciones de serial
constexpr auto MONITOR_SERIAL = true;
constexpr auto MENSAJES_ADICIONALES = false;

// Direcciones EEPROM
constexpr auto CONFIG_PRE_DEFROST_DIRECCION = 1;
constexpr auto CONFIG_PRE_DEFROST_OFFSET = 20;

constexpr auto CONFIG_DEFROST_DIRECCION = 2;
constexpr auto CONFIG_DEFROST_OFFSET = 20;

constexpr auto CONFIG_INICIO_DIRECCION = 3;

constexpr auto CONFIG_TLLENADO_DIRECCION = 4;

// Componentes de la interfaz
auto bReset0 = NexButton(0, 4, "b2");
NexButton* todosReset[] = {&bReset0};

auto bInicio = NexButton(0, 2, "b0");
auto bProcesoActual = NexButton(0, 6, "b4");

auto tTArranque = NexText(4, 3, "txtTArranque");
auto tBombaArr = NexText(4, 8, "txtPump");
auto tFanArr = NexText(4, 9, "txtFan");
auto tLlenadoArr = NexText(4, 10, "txtFill");
auto tFltArr = NexText(4, 12, "txtFlt");

auto tTInicioCiclo = NexText(5, 3, "txtTInicio");
auto tBombaInicio = NexText(5, 8, "txtPump");
auto tFanInicio = NexText(5, 9, "txtFan");
auto tLlenadoInicio = NexText(5, 10, "txtFill");
auto tFltInicio = NexText(5, 12, "txtFlt");

auto tBombaCrusero = NexText(3, 10, "txtPump");
auto tFanCrusero = NexText(3, 11, "txtFan");
auto tLlenadoCrusero = NexText(3, 12, "txtFill");
auto tFltCrusero = NexText(3, 3, "txtFlt");
auto tTemperaturaCrusero = NexText(3, 9, "txtTmp");
auto tTLlenadoCrusero = NexText(3, 14, "txtTFill");

auto tMotorPDefrost = NexText(6, 10, "txtMotor");
auto tTemperaturaPDefrost = NexText(6, 9, "txtTmp");
auto tDefrostPDefrost = NexText(6, 9, "txtDefrost");
auto tFanPDefrost = NexText(6, 11, "txtFan");
auto tLlenadoPDefrost = NexText(6, 12, "txtFill");
auto tTPreDefrost = NexText(6, 14, "txtTPreDefrost");

auto tMotorDefrost = NexText(7, 10, "txtMotor");
auto tTemperaturaDefrost = NexText(7, 9, "txtTmp");
auto tDefrostDefrost = NexText(7, 9, "txtDefrost");
auto tFanDefrost = NexText(7, 11, "txtFan");
auto tLlenadoDefrost = NexText(7, 12, "txtFill");
auto tTDefrost = NexText(7, 14, "txtTDefrost");

auto bAgua = NexDualButton(1, 3, "bAgua");
auto bAguaC = NexDualButton(1, 5, "bAguaC");
auto bLlenado = NexDualButton(1, 4, "bLlenado");
auto bLlenadoC = NexDualButton(1, 6, "bLlenadoC");
auto bDefrost = NexDualButton(1, 7, "bDefrost");

auto slPreDefrost = NexSlider(2, 3, "slPreDefrost");
auto nPreDefrost = NexNumber(2, 4, "nPreDefrost");

auto slDefrost = NexSlider(2, 7, "slDefrost");
auto nDefrost = NexNumber(2, 8, "nDefrost");

auto slInicio = NexSlider(2, 11, "slInicio");
auto nInicio = NexNumber(2, 12, "nInicio");

auto slTLlenado = NexSlider(2, 15, "slTLlenado");
auto nTLlenado = NexNumber(2, 16, "nTLlenado");
auto tTemperaturaConfig = NexText(2, 19, "tTemp");

auto tFalla = NexText(8, 2, "tFalla");

// Componentes que generan eventos
NexTouch* nex_listen_list[] = {
  &bReset0,
  &bInicio,
  &bAgua,
  &bAguaC,
  &bLlenado,
  &bLlenadoC,
  &bProcesoActual,
  &bDefrost,
  &slPreDefrost,
  &slDefrost,
  &slInicio,
  &slTLlenado,
  NULL
};

void resetCallback(void *);
void inicioCallback(void *);
void manualBombaCallback(void *);
void irProcesoActualCallback(void *);
void modoDefrostCallback(void *);
void manualLlenadoCallback(void *);
void manualBombaControlCallback(void *);
void manualLlenadoControlCallback(void *);
void configPreDefrostCallback(void *);
void configDefrostCallback(void *);
void configInicioCallback(void *);
void configTLlenadoCallback(void *);

// Funciones de salida
bool g_contactor = false;
void contactor(bool state) { g_contactor = state; }
bool contactor() { return digitalRead(PIN_CONTACTOR_PRINCIPAL) == LOW; }

bool g_bomba = false;
void bomba(bool state) { g_bomba = state; }
bool bomba() { return digitalRead(PIN_BOMBA) == LOW; }

bool g_fan = false;
void fan(bool state) { g_fan = state; }
bool fan() { return digitalRead(PIN_FAN) == LOW; }

bool g_llenado = false;
void llenado(bool state) { g_llenado = state; }
bool llenado() { return digitalRead(PIN_LLENADO) == HIGH; }

bool g_defrost = false;
void defrost(bool state) { g_defrost = state; }
bool defrost() { return digitalRead(PIN_DEFROST) == LOW; }

bool g_motor = false;
void motor(bool state) { g_motor = state; }
bool motor() { return digitalRead(PIN_MOTOR) == LOW; }

bool g_solenoide_tk = false;
void solenoide_tk(bool state) { g_solenoide_tk = state; }
bool solenoide_tk() { return digitalRead(PIN_SOLENOIDE_TK_ALMACENAMIENTO) == HIGH; }

bool g_bomba_tk = false;
void bomba_tk(bool state) { g_bomba_tk = state; }
bool bomba_tk() { return digitalRead(PIN_BOMBA_TK_ALMACENAMIENTO) == LOW; }

// Globales configurables en UI
auto g_config_temperatura_pre_defrost = 0;
auto g_config_temperatura_defrost = 0;
auto g_config_temperatura_inicio = 60;
auto g_config_tiempo_llenado = 30;

auto g_timestamp_solenoide_tk_on = 0ul;

// Comunicacion Modbus
ModbusMaster modbus;

// Callbacks direccion 485 para libreria modbus
void preTransmission()
{
  // Configurar 485 para enviar
  digitalWrite(PIN_DIRECCION_RS485_1, HIGH);
  digitalWrite(PIN_DIRECCION_RS485_2, HIGH);
}

void postTransmission()
{
  // Configurar 485 para recibir
  digitalWrite(PIN_DIRECCION_RS485_1, LOW);
  digitalWrite(PIN_DIRECCION_RS485_2, LOW);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_FLOTADOR, INPUT_PULLUP);
  pinMode(PIN_FLT_TK_ALTO, INPUT_PULLUP);
  pinMode(PIN_FLT_TK_BAJO, INPUT_PULLUP);
  pinMode(PIN_CONTACTOR_PRINCIPAL, OUTPUT);         contactor(false);
  pinMode(PIN_LLENADO, OUTPUT);                     llenado(false);
  pinMode(PIN_FAN, OUTPUT);                         fan(false);
  pinMode(PIN_BOMBA, OUTPUT);                       bomba(false);
  pinMode(PIN_DEFROST, OUTPUT);                     defrost(false);
  pinMode(LED_BUILTIN, OUTPUT);                     digitalWrite(LED_BUILTIN, LOW);
  pinMode(PIN_SOLENOIDE_TK_ALMACENAMIENTO, OUTPUT); solenoide_tk(false);
  pinMode(PIN_BOMBA_TK_ALMACENAMIENTO, OUTPUT);     bomba_tk(false);
  pinMode(PIN_DIRECCION_RS485_1, OUTPUT);
  pinMode(PIN_DIRECCION_RS485_2, OUTPUT);

  // Pantalla
  {
    
    nexSerial.begin(115200);
    sendCommand("");
    sendCommand("bkcmd=1");
    recvRetCommandFinished();
    sendCommand("page 0");
    recvRetCommandFinished();
  }

  // Modbus RTU - RS485
  {
    Serial3.begin(9600);

    modbus.begin(2, Serial3); // Stop bit 1, parity none
    // Callbacks para mover los pines de direccion de RS485
    modbus.preTransmission(preTransmission);
    modbus.postTransmission(postTransmission);
  }

  // Eventos
  for (NexButton* bReset : todosReset) {
    bReset->attachPop(resetCallback);
  }
  bInicio.attachPop(inicioCallback);
  bAgua.attachPop(manualBombaCallback);
  bAguaC.attachPop(manualBombaControlCallback);
  bLlenado.attachPop(manualLlenadoCallback);
  bLlenadoC.attachPop(manualLlenadoControlCallback);
  bProcesoActual.attachPop(irProcesoActualCallback);
  bDefrost.attachPop(modoDefrostCallback);

  slPreDefrost.attachPop(configPreDefrostCallback);
  slDefrost.attachPop(configDefrostCallback);
  slInicio.attachPop(configInicioCallback);
  slTLlenado.attachPop(configTLlenadoCallback);

  // Load from EEPROM
  g_config_temperatura_pre_defrost = static_cast<int8_t>(EEPROM.read(CONFIG_PRE_DEFROST_DIRECCION));
  g_config_temperatura_defrost = static_cast<int8_t>(EEPROM.read(CONFIG_DEFROST_DIRECCION));
  g_config_temperatura_inicio = static_cast<int8_t>(EEPROM.read(CONFIG_INICIO_DIRECCION));
  g_config_tiempo_llenado = static_cast<int8_t>(EEPROM.read(CONFIG_TLLENADO_DIRECCION));

  // Empezar a contar tiempo solenoide si al arrancar
  // el tk almacenamiento esta vacio
  const auto flotador_tk_bajo = digitalRead(PIN_FLT_TK_BAJO) == LOW;
  if (flotador_tk_bajo == HIGH) {
    g_timestamp_solenoide_tk_on = millis();
    solenoide_tk(true);
  }
}

enum class Modo {
  DETENIDO,
  ARRANQUE,
  INICIO_CICLO,
  CRUSERO,
  FINAL_CICLO,
  DEFROST,
};

using Modo = Modo; // Blame Arduino IDE, https://arduino.stackexchange.com/questions/28133/cant-use-enum-as-function-argument
const char* modoATexto(Modo m) {
  switch (m) {
    case Modo::DETENIDO:        return "DETENIDO    ";
    case Modo::ARRANQUE:        return "ARRANQUE    ";
    case Modo::INICIO_CICLO:    return "INICIO_CICLO";
    case Modo::CRUSERO:         return "CRUSERO     ";
    case Modo::FINAL_CICLO:     return "FINAL_CICLO ";
    case Modo::DEFROST:         return "DEFROST     ";
  }
  return                               "DESCONOCIDO ";
}

auto g_modo_antes = Modo::DETENIDO;
auto g_modo = Modo::DETENIDO;
auto g_modo_siguiente = Modo::DETENIDO;
auto g_temp_debounce_inicio = 0ul;
auto g_temp_bomba_inicio = 0ul;
auto g_temp_serial = 0ul;
auto g_temp_inicio_ciclo = 0ul;
auto g_temp_crusero = 0ul;
auto g_temp_final_ciclo = 0ul;
auto g_temp_defrost = 0ul;
auto g_temp_muestras_control_frio = 0ul;
auto g_temp_contactor = 0ul;

auto g_bomba_control_manual = false;
auto g_bomba_manual = false;
auto g_llenado_control_manual = false;
auto g_llenado_manual = false;

auto g_temperatura = 25.0;

auto ahora = 0ul;

// Callbacks interfaz
void resetCallback(void *) {
  g_modo_siguiente = Modo::DETENIDO;
}

void inicioCallback(void *) {
  Serial.println("Boton inicio");
  g_modo_siguiente = Modo::ARRANQUE;
  
  sendCommand("page 4");
  recvRetCommandFinished();
}

void manualBombaControlCallback(void *) {
  Serial.println("Cambio de modo manual para bomba");
  uint32_t aguaC = -1;

  sendCommand("bkcmd=0");
  recvRetCommandFinished();
  sendCommand("get bAguaC.val");
  recvRetNumber(&aguaC);
  sendCommand("bkcmd=1");
  recvRetCommandFinished();

  g_bomba_control_manual = aguaC == 1;
}

void manualLlenadoControlCallback(void *) {
  Serial.println("Cambio de modo manual para llenado");
  uint32_t llenadoC = -1;
  
  sendCommand("bkcmd=0");
  recvRetCommandFinished();
  sendCommand("get bLlenadoC.val");
  recvRetNumber(&llenadoC);
  sendCommand("bkcmd=1");
  recvRetCommandFinished();

  g_llenado_control_manual = llenadoC == 1;
}

void manualBombaCallback(void *) {
  uint32_t agua = -1;

  sendCommand("bkcmd=0");
  recvRetCommandFinished();
  sendCommand("get bAgua.val");
  recvRetNumber(&agua);
  sendCommand("bkcmd=1");
  recvRetCommandFinished();

  g_bomba_manual = agua == 1;
}

void manualLlenadoCallback(void *) {
  uint32_t llenado = -1;

  sendCommand("bkcmd=0");
  recvRetCommandFinished();
  sendCommand("get bLlenado.val");
  recvRetNumber(&llenado);
  sendCommand("bkcmd=1");
  recvRetCommandFinished();

  g_llenado_manual = llenado == 1;
}

void modoDefrostCallback(void *) {
  Serial.println("Boton manual defrost");

  g_modo_siguiente = Modo::DEFROST;

  sendCommand("page 7");
  recvRetCommandFinished();
}

void irProcesoActualCallback(void *) {
  switch (g_modo) {
    case Modo::ARRANQUE: sendCommand("page 4"); break;
    case Modo::INICIO_CICLO: sendCommand("page 5"); break;
    case Modo::CRUSERO: sendCommand("page 3"); break;
    case Modo::FINAL_CICLO: sendCommand("page 6"); break;
    case Modo::DEFROST: sendCommand("page 7"); break;
    default: sendCommand("page 0"); break;
  }

  recvRetCommandFinished();
}

void configPreDefrostCallback(void *) {
  uint32_t value;
  slPreDefrost.getValue(&value);

  const auto temp = static_cast<int8_t>(value) - CONFIG_PRE_DEFROST_OFFSET;

  Serial.print("Guardando en EEPROM en direccion ");
  Serial.print(CONFIG_PRE_DEFROST_DIRECCION);
  Serial.print(" el valor ");
  Serial.println(static_cast<uint8_t>(temp));

  EEPROM.write(CONFIG_PRE_DEFROST_DIRECCION, static_cast<uint8_t>(temp));
  g_config_temperatura_pre_defrost = temp;
}

void configDefrostCallback(void *) {
  uint32_t value;
  slDefrost.getValue(&value);

  const auto temp = static_cast<int8_t>(value) - CONFIG_DEFROST_OFFSET;

  Serial.print("Guardando en EEPROM en direccion ");
  Serial.print(CONFIG_DEFROST_DIRECCION);
  Serial.print(" el valor ");
  Serial.println(static_cast<uint8_t>(temp));

  EEPROM.write(CONFIG_DEFROST_DIRECCION, static_cast<uint8_t>(temp));
  g_config_temperatura_defrost = temp;
}

void configInicioCallback(void *) {
  uint32_t value;
  slInicio.getValue(&value);

  const auto temp = static_cast<int8_t>(value);

  Serial.print("Guardando en EEPROM en direccion ");
  Serial.print(CONFIG_INICIO_DIRECCION);
  Serial.print(" el valor ");
  Serial.println(static_cast<uint8_t>(temp));

  EEPROM.write(CONFIG_INICIO_DIRECCION, static_cast<uint8_t>(temp));
  g_config_temperatura_inicio = temp;
}
void configTLlenadoCallback(void *) {
  uint32_t value;
  slTLlenado.getValue(&value);

  const auto tiempo = static_cast<int8_t>(value);

  Serial.print("Guardando en EEPROM en direccion ");
  Serial.print(CONFIG_TLLENADO_DIRECCION);
  Serial.print(" el valor ");
  Serial.println(static_cast<uint8_t>(tiempo));

  EEPROM.write(CONFIG_TLLENADO_DIRECCION, static_cast<uint8_t>(tiempo));
  g_config_tiempo_llenado = tiempo;
}

void patronParpadeoLed();
void inicializarContadores();
void muestraControlFrio();
void informacionSerial();
void leerTemperatura();
void actualizarPantalla(bool flotador);
void actualizarSalidas();
void cambiosDeModo();
void alarmas();

auto flotador = false; // true -> vacio
auto flotador_antes = false;

auto flotador_tk_alto = false; // true -> nivel por encima de high
auto flotador_tk_alto_antes = false;
auto flotador_tk_bajo = false; // true -> nivel por encima de low
auto flotador_tk_bajo_antes = false;

void loop() {
  g_modo_antes = g_modo;
  g_modo = g_modo_siguiente;

  flotador_antes = flotador;
  flotador = digitalRead(PIN_FLOTADOR) == HIGH;

  flotador_tk_alto_antes = flotador_tk_alto;
  flotador_tk_alto = digitalRead(PIN_FLT_TK_ALTO) == HIGH;
  flotador_tk_bajo_antes = flotador_tk_bajo;
  flotador_tk_bajo = digitalRead(PIN_FLT_TK_BAJO) == HIGH;
  
  ahora = millis();

  patronParpadeoLed();
  inicializarContadores();
  leerTemperatura();
  cambiosDeModo();
  logicaTanque();

  if (g_modo == Modo::DETENIDO) {
    contactor(false);
    defrost(false);
    bomba(false);
    fan(false);
    llenado(false);
    solenoide_tk(false);
    bomba_tk(false);
  }

  if (g_modo == Modo::INICIO_CICLO || g_modo == Modo::ARRANQUE) {
    if (g_modo == Modo::ARRANQUE) {
      if (g_temp_contactor <= ahora) {
        g_modo_siguiente = Modo::INICIO_CICLO;
      }
    } else {
      contactor(true);
    }
    fan(true);
    defrost(false);
    if (g_temp_debounce_inicio <= ahora) {
      g_temp_debounce_inicio = ahora + 1000; // programar siguiente actualizacion para dentro de 1 segundo
      if (flotador) { // menos de lleno
        llenado(true);
      } else { // lleno
        llenado(false);
      }
    }

    if (!flotador) { // lleno
      bomba(true); // prender la bomba
      g_temp_bomba_inicio = ahora + TIEMPO_BOMBA_INICIO; // programar su apagado
      if (MENSAJES_ADICIONALES && g_temp_serial <= ahora) {
        Serial.println("Programando apagado de la bomba para dentro de 5s");
      }
    }

    if (g_temp_bomba_inicio <= ahora) {
      bomba(false);
    }

    if (g_temp_inicio_ciclo <= ahora) {
      Serial.println("Pasando a modo crusero");
      g_modo_siguiente = Modo::CRUSERO;
    }
  }

  if (g_modo == Modo::CRUSERO) {
    fan(true);
    defrost(false);
    bomba(true);

    if (flotador && !flotador_antes) { // menos de lleno y justo cambio
      g_temp_crusero = ahora + (g_config_tiempo_llenado * 1000ul); // poner a correr tiempo
      if (MENSAJES_ADICIONALES) {
        Serial.println("Programando el llenado");
      }
    }
    
    if (g_temp_crusero <= ahora && flotador) {
      llenado(true);
      if (MENSAJES_ADICIONALES && g_temp_serial <= ahora) {
        Serial.println("Finalizado tiempo de espera para llenado, llenando");
      }
    }

    if (!flotador) { // lleno
      llenado(false);
      if (MENSAJES_ADICIONALES && g_temp_serial <= ahora) {
        Serial.println("Lleno, apagando llenado");
      }
    }

    if (g_temperatura < g_config_temperatura_pre_defrost) {
      g_modo_siguiente = Modo::FINAL_CICLO;
      g_temp_final_ciclo = ahora + TIEMPO_FINAL_DE_CICLO;
    }
  }

  if (g_modo == Modo::FINAL_CICLO) {
    bomba(true);
    llenado(false);
    fan(true);
    defrost(false);
    
    if (g_temp_final_ciclo <= ahora) {
      g_modo_siguiente = Modo::DEFROST;
    }

    if (g_temperatura < g_config_temperatura_defrost) {
      g_modo_siguiente = Modo::DEFROST;
    }
  }

  if (g_modo == Modo::DEFROST) {
    fan(false);
    defrost(true);
    bomba(false);
    llenado(false);
    
    if (g_temp_defrost <= ahora) {
      g_modo_siguiente = Modo::INICIO_CICLO;
    }

    if (g_temperatura > g_config_temperatura_inicio) {
      g_modo_siguiente = Modo::INICIO_CICLO;
    }
  }

  if (g_bomba_control_manual && g_modo != Modo::DEFROST) {
    bomba(g_bomba_manual);
  }
  if (g_llenado_control_manual && g_modo != Modo::DEFROST) {
    llenado(g_llenado_manual);
  }

  alarmas();
  actualizarSalidas();
  nexLoop(nex_listen_list);
  actualizarPantalla(flotador);
  informacionSerial(flotador);
}

/***************
 * End of Loop *
 ***************/

void actualizarSalidas() {
  digitalWrite(PIN_CONTACTOR_PRINCIPAL, g_contactor ? LOW : HIGH);
  digitalWrite(PIN_BOMBA, g_bomba ? LOW : HIGH);
  digitalWrite(PIN_FAN, g_fan ? LOW : HIGH);
  digitalWrite(PIN_MOTOR, g_motor ? LOW : HIGH);
  digitalWrite(PIN_LLENADO, g_llenado ? HIGH : LOW);
  digitalWrite(PIN_DEFROST, g_defrost ? LOW : HIGH);
  digitalWrite(PIN_SOLENOIDE_TK_ALMACENAMIENTO, g_solenoide_tk ? HIGH : LOW);
  digitalWrite(PIN_BOMBA_TK_ALMACENAMIENTO, g_bomba_tk ? LOW : HIGH);
}

void cambiosDeModo() {
  if (g_modo == Modo::ARRANQUE && g_modo_antes != Modo::ARRANQUE) {
    Serial.println("Programando tiempo modo arranque");
    g_temp_contactor = ahora + TIEMPO_ARRANQUE_CONTACTOR;
    // Requerido porque arranque e inicio comparten la logica
    g_temp_inicio_ciclo = ahora + TIEMPO_MODO_INICIO_CICLO;
  }

  if (g_modo == Modo::INICIO_CICLO && g_modo_antes != Modo::INICIO_CICLO) {
    contactor(true);
    Serial.println("Programando tiempo modo inicio ciclo");
    g_temp_inicio_ciclo = ahora + TIEMPO_MODO_INICIO_CICLO;
  }

  if (g_modo == Modo::DEFROST && g_modo_antes != g_modo) {
    Serial.println("Programando tiempo defrost");
    g_temp_defrost = ahora + TIEMPO_DEFROST;
    Serial.println("Desactivando modos manuales");
    g_bomba_control_manual = false;
    g_llenado_control_manual = false;
  }
}

void logicaTanque() {
  const auto evento_bajo_nivel = 
    flotador_tk_bajo != flotador_tk_bajo_antes && // hubo cambio en sensor de nivel bajo, y...
    flotador_tk_bajo == false;                    // cambió a off
  
  if (evento_bajo_nivel) {
    solenoide_tk(true);
    g_timestamp_solenoide_tk_on = ahora;
  }

  const auto tiempo_solenoide_encendida = ahora - g_timestamp_solenoide_tk_on;
  const auto finalizo_temporizador_solenoide =
    solenoide_tk() && // la solenoide esta encendida, y...
    tiempo_solenoide_encendida >= TIEMPO_SOLENOIDE_TK_ALMACENAMIENTO; // ya paso el tiempo de solenoide
  
  if (finalizo_temporizador_solenoide) {
    bomba_tk(true);
  }

  const auto evento_alto_nivel =
    flotador_tk_alto != flotador_tk_alto_antes && // hubo cambio en el sensor de nivel alto, y...
    flotador_tk_alto == true;                     // cambió a on

  if (evento_alto_nivel) {
    bomba_tk(false);
    solenoide_tk(false);
  }
}

void patronParpadeoLed() {
  if (g_modo == Modo::INICIO_CICLO) {
    if ((ahora % 1000) < 500) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
  } else if (g_modo == Modo::CRUSERO) {
    if ((ahora % 2000) < 1000) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
  } else if (g_modo == Modo::FINAL_CICLO) {
    if ((ahora % 500) < 250) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
  } else if (g_modo == Modo::DEFROST) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void inicializarContadores() {
  if (g_temp_debounce_inicio == 0) {
    g_temp_debounce_inicio = ahora;
  }
  if (g_temp_bomba_inicio == 0) {
    g_temp_bomba_inicio = ahora;
  }
  if (g_temp_serial == 0) {
    g_temp_serial = ahora;
  }
  if (g_temp_serial == 0) {
    g_temp_serial = ahora;
  }
  if (g_temp_inicio_ciclo == 0) {
    g_temp_inicio_ciclo = ahora;
  }
  if (g_temp_contactor == 0) {
    g_temp_contactor = ahora;
  }
}

template<typename T, int SIZE, typename A>
class Average {
  T data[SIZE];
  T* current;
public:
  Average() : current(data) {}
  
  void add_val(T val)
  {
    *current = val;
    ++current;
    if (current >= (data + SIZE)) current = data;
  }

  A get_val()
  {
    A sum = 0;
    for (auto val : data) {
      sum += val;
    }
    return sum / SIZE;
  }
};

auto g_timestamp_ultima_lectura_temperatura = 0ul;

void leerTemperatura() {
  const auto tiempo_transcurrido = ahora - g_timestamp_ultima_lectura_temperatura;

  if (tiempo_transcurrido >= 1000) {
    g_timestamp_ultima_lectura_temperatura = ahora;

    const auto result = modbus.readInputRegisters(0x03E8, 1);

    if (result == modbus.ku8MBSuccess) {
      const auto value = modbus.getResponseBuffer(0);

      const auto degreesTimesTen = static_cast<int16_t>(value);
      g_temperatura = degreesTimesTen / 10.0;
    } else {
      Serial.print("Lectura no exitosa de la temperatura, codigo libreria modbus: ");
      Serial.println(result, 16);
    }

  }
}

void pantallaFalla(const char* mensaje) {
  sendCommand("page 8"); // fallas
  recvRetCommandFinished();

  tFalla.setText(mensaje);
}

auto g_timestamp_inicio_llenado = 0ul;
auto g_timestamp_inicio_llenado_tk = 0ul;
auto g_timestamp_inicio_ciclo = 0ul;

auto g_tiempo_ciclo_prev = 0ul; // anterior anterior
auto g_tiempo_ciclo = 0ul;      // anterior

void alarmas() {
  // Timestamps
  if (g_llenado && g_llenado != llenado()) {
    g_timestamp_inicio_llenado = ahora;
  }

  if (g_bomba_tk && g_bomba_tk != bomba_tk()) {
    g_timestamp_inicio_llenado_tk = ahora;
  }

  if (g_modo == Modo::INICIO_CICLO && g_modo != g_modo_antes) {
    g_timestamp_inicio_ciclo = ahora;
  }

  // Alarmas
  char buffer[64];

  // Alarma tiempo llenado
  auto tiempo_llenando = ahora - g_timestamp_inicio_llenado;
  if (llenado() && tiempo_llenando > TIEMPO_ALARMA_LLENADO) {
    g_modo_siguiente = Modo::DETENIDO;

    sprintf(buffer, "Tiempo de llenado super\xF3 %ds", (int)(TIEMPO_ALARMA_LLENADO / 1000ul));
    pantallaFalla(buffer);
  }

  // Alarma sensor bajo dañado
  if (!flotador_tk_bajo && flotador_tk_alto) {
    pantallaFalla("Falla en sensor de nivel bajo");
  }

  // Alarma tiempo llenado tk almacenamiento
  auto tiempo_llenando_tk = ahora - g_timestamp_inicio_llenado_tk;
  if (bomba_tk() && tiempo_llenando_tk > TIEMPO_ALARMA_LLENADO_TK_ALAMCENAMIENTO) {
    g_modo_siguiente = Modo::DETENIDO;
    solenoide_tk(false);
    bomba_tk(false);
    g_timestamp_inicio_llenado_tk = ahora; // resetear contador de alarma

    sprintf(buffer, "Tiempo de llenado tk alamcenamiento super\xF3 %ds", (int)(TIEMPO_ALARMA_LLENADO_TK_ALAMCENAMIENTO / 1000ul));
    pantallaFalla(buffer);
  }

  // Alarma tiempo ciclo muy corto
  if (g_modo == Modo::INICIO_CICLO && g_modo != g_modo_antes) {
    if (g_timestamp_inicio_ciclo != 0ul) {
      const auto duracion_ciclo = ahora - g_timestamp_inicio_ciclo;
      g_tiempo_ciclo_prev = g_tiempo_ciclo;
      g_tiempo_ciclo = duracion_ciclo;
    }

    if (g_tiempo_ciclo > 0ul && g_tiempo_ciclo_prev > 0ul) {
      const auto sumatoria_tiempos_ciclo = g_tiempo_ciclo + g_tiempo_ciclo_prev;
      if (sumatoria_tiempos_ciclo > TIEMPO_ALARMA_DOS_CICLOS) {
        g_modo_siguiente = Modo::DETENIDO;

        sprintf(buffer, "3 Inicios de ciclo en los ultimos %ds", (int)(TIEMPO_ALARMA_DOS_CICLOS / 1000ul));
        pantallaFalla(buffer);
      }
    }
  }

}

void informacionSerial(int flotador) {
  if (g_modo != g_modo_antes) {
    Serial.print(modoATexto(g_modo_antes));
    Serial.print(" -> ");
    Serial.println(modoATexto(g_modo));
  }

  if (MONITOR_SERIAL && g_temp_serial <= ahora) {
    if (g_bomba_control_manual) {
      Serial.print("Bomba control manual: ");
      Serial.println(g_bomba_manual);
    }

    if (g_llenado_control_manual) {
      Serial.print("Llenado control manual: ");
      Serial.println(g_llenado_manual);
    }

    Serial.print("Modo: ");
    Serial.print(modoATexto(g_modo));

    Serial.print(" - Entradas: ");
    if (flotador == HIGH) {
      Serial.print("FLT ");
    } else {
      Serial.print("    ");
    }
    if (flotador_tk_bajo) {
      Serial.print("FLB ");
    } else {
      Serial.print("    ");
    }
    if (flotador_tk_alto) {
      Serial.print("FLA ");
    } else {
      Serial.print("    ");
    }

    Serial.print(" - Salidas: ");
    if (digitalRead(PIN_CONTACTOR_PRINCIPAL) == LOW) {
      Serial.print("CTR ");
    } else {
      Serial.print("    ");
    }
    if (digitalRead(PIN_DEFROST) == LOW) {
      Serial.print("DEF ");
    } else {
      Serial.print("    ");
    }
    if (digitalRead(PIN_LLENADO) == HIGH) {
      Serial.print("LLE ");
    } else {
      Serial.print("    ");
    }
    if (digitalRead(PIN_FAN) == LOW) {
      Serial.print("FAN ");
    } else {
      Serial.print("    ");
    }
    if (bomba()) {
      Serial.print("BMB ");
    } else {
      Serial.print("    ");
    }
    if (solenoide_tk()) {
      Serial.print("STK ");
    } else {
      Serial.print("    ");
    }
    if (bomba_tk()) {
      Serial.print("BTK ");
    } else {
      Serial.print("    ");
    }

    Serial.print(" - TEMPERATURA: ");
    Serial.print(g_temperatura);

    if (g_modo == Modo::INICIO_CICLO) {
      Serial.print(" - T_INICIO: ");
      Serial.print((g_temp_inicio_ciclo - ahora)/ 1000ul);
      Serial.print(", T_BOMBA: ");
      Serial.print((g_temp_bomba_inicio - ahora)/ 1000ul);
      Serial.print(", T_DEBOUNCE: ");
      Serial.print((g_temp_debounce_inicio - ahora)/ 1000ul);
    }

    if (g_modo == Modo::CRUSERO) {
      if (g_temp_crusero >= ahora) {
        Serial.print(" - T_ESPERA_LLENADO: ");
        Serial.print((g_temp_crusero - ahora)/ 1000ul);
      }
    }

    if (g_modo == Modo::FINAL_CICLO) {
      Serial.print(" - T_FINAL: ");
      Serial.print((g_temp_final_ciclo - ahora)/ 1000ul);
    }

    if (g_modo == Modo::DEFROST) {
      Serial.print(" - T_DEFROST: ");
      Serial.print((g_temp_defrost - ahora)/ 1000ul);
    }

    Serial.println();
    g_temp_serial += 1000;
  }
}

auto pagina_anterior = 0;
uint32_t pagina = -1;

void actualizarPantalla(bool flotador) {

  char buffer[32];
  
  if (g_temp_serial <= ahora) {

    pagina_anterior = pagina;
    sendCommand("get dp");
    recvRetNumber(&pagina);

    if (pagina == 2 && pagina != pagina_anterior) { // configuraciones
      slPreDefrost.setValue(g_config_temperatura_pre_defrost + CONFIG_PRE_DEFROST_OFFSET);
      nPreDefrost.setValue(g_config_temperatura_pre_defrost);

      slDefrost.setValue(g_config_temperatura_defrost + CONFIG_DEFROST_OFFSET);
      nDefrost.setValue(g_config_temperatura_defrost);

      slInicio.setValue(g_config_temperatura_inicio);
      nInicio.setValue(g_config_temperatura_inicio);

      slTLlenado.setValue(g_config_tiempo_llenado);
      nTLlenado.setValue(g_config_tiempo_llenado);

      dtostrf(g_temperatura, 6, 1, buffer);
      tTemperaturaConfig.setText(buffer);
    }

    if (pagina == 1) { // manual control

      if (!g_bomba_control_manual) { // auto
        // update the button state
        bAgua.setVal(bomba() ? 1 : 0);
      } else {
        bAgua.setVal(g_bomba_manual ? 1 : 0);
      }

      if (!g_llenado_control_manual) { // auto
        // update the button state
        bLlenado.setVal(llenado() ? 1 : 0);
      } else {
        bLlenado.setVal(g_llenado_manual ? 1 : 0);
      }

      bAguaC.setVal(g_bomba_control_manual ? 1 : 0);
      bAguaC.setText(g_bomba_control_manual ? "Manual" : "Auto");
      bLlenadoC.setVal(g_llenado_control_manual ? 1 : 0);
      bLlenadoC.setText(g_llenado_control_manual ? "Manual" : "Auto");

    }

    if (g_modo == Modo::ARRANQUE) {
      itoa((g_temp_contactor - ahora) / 1000ul, buffer, 10);
      tTArranque.setText(buffer);
      tBombaArr.setText(bomba() ? "ON" : "OFF");
      tFanArr.setText(fan() ? "ON" : "OFF");
      tLlenadoArr.setText(llenado() ? "ON" : "OFF");
      tFltArr.setText(flotador ? "VACIO" : "LLENO");
    }

    if (g_modo == Modo::INICIO_CICLO) {
      itoa((g_temp_inicio_ciclo - ahora) / 1000ul, buffer, 10);
      tTInicioCiclo.setText(buffer);
      tBombaInicio.setText(bomba() ? "ON" : "OFF");
      tFanInicio.setText(fan() ? "ON" : "OFF");
      tLlenadoInicio.setText(llenado() ? "ON" : "OFF");
      tFltInicio.setText(flotador ? "VACIO" : "LLENO");
    }

    if (g_modo == Modo::CRUSERO) {
      tBombaCrusero.setText(bomba() ? "ON" : "OFF");
      tFanCrusero.setText(fan() ? "ON" : "OFF");
      tLlenadoCrusero.setText(llenado() ? "ON": "OFF");
      tFltCrusero.setText(flotador ? "VACIO": "LLENO");
      dtostrf(g_temperatura, 6, 1, buffer);
      tTemperaturaCrusero.setText(buffer);
      
      if (g_temp_crusero >= ahora) {
        itoa((g_temp_crusero - ahora) / 1000ul, buffer, 10);
        tTLlenadoCrusero.setText(buffer);
      } else {
        tTLlenadoCrusero.setText("N/A");
      }
    }

    if (g_modo == Modo::FINAL_CICLO) {
      tMotorPDefrost.setText(motor() ? "ON" : "OFF");
      dtostrf(g_temperatura, 6, 1, buffer);
      tTemperaturaPDefrost.setText(buffer);
      tDefrostPDefrost.setText(defrost() ? "ON" : "OFF");
      tFanPDefrost.setText(fan() ? "ON" : "OFF");
      tLlenadoPDefrost.setText(llenado() ? "ON": "OFF");
      if (g_temp_final_ciclo >= ahora) {
        itoa((g_temp_final_ciclo - ahora) / 1000ul, buffer, 10);
        tTPreDefrost.setText(buffer);
      } else {
        tTPreDefrost.setText("N/A");
      }
    }

    if (g_modo == Modo::DEFROST) {
      tMotorDefrost.setText(motor() ? "ON" : "OFF");
      dtostrf(g_temperatura, 6, 1, buffer);
      tTemperaturaDefrost.setText(buffer);
      tDefrostDefrost.setText(defrost() ? "ON" : "OFF");
      tFanDefrost.setText(fan() ? "ON" : "OFF");
      tLlenadoDefrost.setText(llenado() ? "ON": "OFF");
      if (g_temp_defrost >= ahora) {
        itoa((g_temp_defrost - ahora) / 1000ul, buffer, 10);
        tTDefrost.setText(buffer);
      } else {
        tTDefrost.setText("N/A");
      }
    }
  }

  if (g_modo == Modo::INICIO_CICLO && g_modo_antes != Modo::INICIO_CICLO) {
    sendCommand("page 5");
    recvRetCommandFinished();
  }

  if (g_modo == Modo::CRUSERO && g_modo_antes != Modo::CRUSERO) {
    sendCommand("page 3");
    recvRetCommandFinished();
  }

  if (g_modo == Modo::FINAL_CICLO && g_modo_antes != Modo::FINAL_CICLO) {
    sendCommand("page 6");
    recvRetCommandFinished();
  }

  if (g_modo == Modo::DEFROST && g_modo_antes != Modo::DEFROST) {
    sendCommand("page 7");
    recvRetCommandFinished();
  }
}