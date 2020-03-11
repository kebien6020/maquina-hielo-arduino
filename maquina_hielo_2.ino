#include <OneWire.h>
#include <DallasTemperature.h>
#include <Nextion.h>
#include <stdlib.h>
#include <EEPROM.h>

#include "src/NexDualButton.h"
#include "src/NexNumber.h"

// Entradas
constexpr int PIN_FLOTADOR = 4;
constexpr int PIN_SENSOR_TEMPERATURA = A0;
// Salidas
constexpr int PIN_CONTACTOR_PRINCIPAL = 11;
constexpr int PIN_DEFROST = 10;
constexpr int PIN_FAN = 9;
constexpr int PIN_BOMBA = 8;
// constexpr int PIN_LLENADO_TK_ALMACENAMIENTO = 7;
constexpr int PIN_LLENADO = 6;
constexpr int PIN_MOTOR = 11;

// Tiempos
constexpr auto TIEMPO_MODO_INICIO_CICLO = 60000ul;
constexpr auto TIEMPO_BOMBA_INICIO = 5000ul;
constexpr auto TIEMPO_PREVIO_LLENADO_CRUSERO = 30ul * 1000ul;
constexpr auto TIEMPO_FINAL_DE_CICLO = 4ul * 60ul * 1000ul;
constexpr auto TIEMPO_DEFROST = 6ul * 60ul * 1000ul + 20ul * 1000ul;
constexpr auto TIEMPO_MUESTRA_CONTROL_FRIO = 50ul;
constexpr auto TIEMPO_ARRANQUE_CONTACTOR = 10000ul;

// Temperaturas
constexpr auto TEMPERATURA_FINAL_CICLO = 0.0f;

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

OneWire wire{PIN_SENSOR_TEMPERATURA};
DallasTemperature sensor{&wire};
DeviceAddress deviceAddress;

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

auto g_config_temperatura_pre_defrost = 0;
auto g_config_temperatura_defrost = 0;
auto g_config_temperatura_inicio = 60;
auto g_config_tiempo_llenado = 30;

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_FLOTADOR, INPUT_PULLUP);
  pinMode(PIN_SENSOR_TEMPERATURA, INPUT);
  pinMode(PIN_CONTACTOR_PRINCIPAL, OUTPUT); contactor(false);
  pinMode(PIN_LLENADO, OUTPUT);             llenado(false);
  pinMode(PIN_FAN, OUTPUT);                 fan(false);
  pinMode(PIN_BOMBA, OUTPUT);               bomba(false);
  pinMode(PIN_DEFROST, OUTPUT);             defrost(false);
  pinMode(LED_BUILTIN, OUTPUT);             digitalWrite(LED_BUILTIN, LOW);

  // Sensor de temperatura
  sensor.begin();
  sensor.setWaitForConversion(false);
	sensor.getAddress(deviceAddress, 0);

  // Pantalla
  {
    
    nexSerial.begin(115200);
    sendCommand("");
    sendCommand("bkcmd=1");
    recvRetCommandFinished();
    sendCommand("page 0");
    recvRetCommandFinished();
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

auto g_temperatura = 25.0f;

auto ahora = 0ul;

auto flotador_antes = HIGH;

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

auto flotador = false; // true -> lleno

void loop() {
  g_modo_antes = g_modo;
  g_modo = g_modo_siguiente;

  flotador_antes = flotador;
  flotador = digitalRead(PIN_FLOTADOR) == LOW;
  
  ahora = millis();

  patronParpadeoLed();
  inicializarContadores();
  leerTemperatura();
  cambiosDeModo();

  if (g_modo == Modo::DETENIDO) {
    contactor(false);
    defrost(false);
    bomba(false);
    fan(false);
    llenado(false);
  }

  if (g_modo == Modo::INICIO_CICLO || g_modo == Modo::ARRANQUE) {
    if (g_modo == Modo::ARRANQUE) {
      if (g_temp_contactor <= ahora) {
        contactor(true);
        g_modo_siguiente = Modo::INICIO_CICLO;
      }
    } else {
      contactor(true);
    }
    fan(true);
    defrost(false);
    if (g_temp_debounce_inicio <= ahora) {
      g_temp_debounce_inicio = ahora + 1000; // programar siguiente actualizacion para dentro de 1 segundo
      if (flotador == HIGH) { // menos de lleno
        llenado(true);
      } else { // lleno
        llenado(false);
      }
    }

    if (flotador == LOW) { // lleno
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

    if (flotador == HIGH && flotador_antes == LOW) { // menos de lleno y justo cambio
      g_temp_crusero = ahora + TIEMPO_PREVIO_LLENADO_CRUSERO; // poner a correr tiempo
      if (MENSAJES_ADICIONALES) {
        Serial.println("Programando el llenado");
      }
    }
    
    if (g_temp_crusero <= ahora && flotador == HIGH) {
      llenado(true);
      if (MENSAJES_ADICIONALES && g_temp_serial <= ahora) {
        Serial.println("Finalizado tiempo de espera para llenado, llenando");
      }
    }

    if (flotador == LOW) { // lleno
      llenado(false);
      if (MENSAJES_ADICIONALES && g_temp_serial <= ahora) {
        Serial.println("Lleno, apagando llenado");
      }
    }

    if (g_temperatura > DEVICE_DISCONNECTED_C && g_temperatura < TEMPERATURA_FINAL_CICLO) {
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
  }

  if (g_modo == Modo::DEFROST) {
    fan(false);
    defrost(true);
    bomba(false);
    llenado(false);
    
    if (g_temp_defrost <= ahora) {
      g_modo_siguiente = Modo::INICIO_CICLO;
    }
  }

  if (g_bomba_control_manual && g_modo != Modo::DEFROST) {
    bomba(g_bomba_manual);
  }
  if (g_llenado_control_manual && g_modo != Modo::DEFROST) {
    llenado(g_llenado_manual);
  }

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
}

void cambiosDeModo() {
  if (g_modo == Modo::ARRANQUE && g_modo_antes != Modo::ARRANQUE) {
    Serial.println("Programando tiempo modo arranque");
    g_temp_contactor = ahora + TIEMPO_ARRANQUE_CONTACTOR;
    g_temp_inicio_ciclo = ahora + TIEMPO_MODO_INICIO_CICLO;
  }

  if (g_modo == Modo::INICIO_CICLO && g_modo_antes != Modo::INICIO_CICLO) {
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
    g_temp_contactor = ahora + TIEMPO_MODO_INICIO_CICLO;
  }
  if (g_temp_contactor == 0) {
    g_temp_contactor = ahora + TIEMPO_ARRANQUE_CONTACTOR;
  }
}

void leerTemperatura() {
  sensor.requestTemperatures();
  const auto temp = sensor.getTempCByIndex(0);
  if (temp > DEVICE_DISCONNECTED_C) g_temperatura = temp;
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

    Serial.print(" - Salidas: ");
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
  
  if (g_temp_serial <= ahora) {
    // tFlt.setText(flotador ? "Lleno" : "Vacio");
    // char buffer_temperatura[32];
    // dtostrf(g_temperatura, 6, 1, buffer_temperatura);
    // tTmp.setText(buffer_temperatura);
    // tContact.setText(contactor() ? "ON" : "OFF");
    // tMode.setText(modoATexto(g_modo));
    // tDefrost.setText(defrost() ? "ON" : "OFF");


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
    
    char buffer[32];

    if (g_modo == Modo::ARRANQUE) {
      itoa((g_temp_contactor - ahora) / 1000ul, buffer, 10);
      tTArranque.setText(buffer);
      tBombaArr.setText(bomba() ? "ON" : "OFF");
      tFanArr.setText(fan() ? "ON" : "OFF");
      tLlenadoArr.setText(llenado() ? "ON" : "OFF");
      tFltArr.setText(flotador ? "ON" : "OFF");
    }

    if (g_modo == Modo::INICIO_CICLO) {
      itoa((g_temp_inicio_ciclo - ahora) / 1000ul, buffer, 10);
      tTInicioCiclo.setText(buffer);
      tBombaInicio.setText(bomba() ? "ON" : "OFF");
      tFanInicio.setText(fan() ? "ON" : "OFF");
      tLlenadoInicio.setText(llenado() ? "ON" : "OFF");
      tFltInicio.setText(flotador ? "ON" : "OFF");
    }

    if (g_modo == Modo::CRUSERO) {
      tBombaCrusero.setText(bomba() ? "ON" : "OFF");
      tFanCrusero.setText(fan() ? "ON" : "OFF");
      tLlenadoCrusero.setText(llenado() ? "ON": "OFF");
      tFltCrusero.setText(flotador ? "ON": "OFF");
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