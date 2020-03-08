#include <OneWire.h>
#include <DallasTemperature.h>
#include <Nextion.h>
#include <stdlib.h>

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

// Componentes de la interfaz
auto bReset0 = NexButton(0, 4, "b2");
auto bReset1 = NexButton(1, 4, "b2");
auto bReset2 = NexButton(2, 2, "b2");
auto bReset3 = NexButton(3, 2, "b2");
auto bReset4 = NexButton(4, 2, "b2");
NexButton* todosReset[] = {&bReset0, &bReset1, &bReset2, &bReset3, &bReset4};

auto bInicio = NexButton(0, 2, "b0");

auto tTArranque = NexText(4, 3, "txtTArranque");
auto tBombaArr = NexText(4, 8, "txtPump");
auto tFanArr = NexText(4, 9, "txtFan");
auto tLlenadoArr = NexText(4, 10, "txtFill");

// Componentes que generan eventos
NexTouch* nex_listen_list[] = {
  &bReset0,
  &bReset1,
  &bReset2,
  &bReset3,
  &bReset4,
  &bInicio,
  NULL
};

void resetCallback(void *);
void inicioCallback(void *);

OneWire wire{PIN_SENSOR_TEMPERATURA};
DallasTemperature sensor{&wire};
DeviceAddress deviceAddress;

void contactor(bool state) { digitalWrite(PIN_CONTACTOR_PRINCIPAL, state ? LOW : HIGH); }
bool contactor() { return digitalRead(PIN_CONTACTOR_PRINCIPAL) == LOW; }

void bomba(bool state) { digitalWrite(PIN_BOMBA, state ? LOW : HIGH); }
bool bomba() { return digitalRead(PIN_BOMBA) == LOW; }

void fan(bool state) { digitalWrite(PIN_FAN, state ? LOW : HIGH); }
bool fan() { return digitalRead(PIN_FAN) == LOW; }

void llenado(bool state) { digitalWrite(PIN_LLENADO, state ? HIGH : LOW); }
bool llenado() { return digitalRead(PIN_LLENADO) == HIGH; }

void defrost(bool state) { digitalWrite(PIN_DEFROST, state ? LOW : HIGH); }
bool defrost() { return digitalRead(PIN_DEFROST) == LOW; }

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
    auto success = nexInit();
    while(nexSerial.available()) { Serial.print((uint8_t)nexSerial.read()); }
    if (success)
      Serial.print("Nex succesfully initialized");
  }

  // Eventos
  for (NexButton* bReset : todosReset) {
    bReset->attachPop(resetCallback);
  }
  bInicio.attachPop(inicioCallback);
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
auto g_temp_debounce_inicio = 0ul;
auto g_temp_bomba_inicio = 0ul;
auto g_temp_serial = 0ul;
auto g_temp_inicio_ciclo = 0ul;
auto g_temp_crusero = 0ul;
auto g_temp_final_ciclo = 0ul;
auto g_temp_defrost = 0ul;
auto g_temp_muestras_control_frio = 0ul;
auto g_temp_contactor = 0ul;

auto g_temperatura = 25.0f;

auto ahora = 0ul;

auto flotador_antes = HIGH;

// Callbacks interfaz
void resetCallback(void *) {
  g_modo = Modo::DETENIDO;
}

void inicioCallback(void *) {
  Serial.println("Boton inicio");
  g_modo = Modo::ARRANQUE;
  
  sendCommand("page 4");
}

void patronParpadeoLed();
void inicializarContadores();
void muestraControlFrio();
void informacionSerial();
void leerTemperatura();
void actualizarPantalla(bool flotador);

auto flotador = false; // true -> lleno

void loop() {
  g_modo_antes = g_modo;
  flotador_antes = flotador;

  flotador = digitalRead(PIN_FLOTADOR) == LOW;
  
  ahora = millis();

  patronParpadeoLed();
  inicializarContadores();
  leerTemperatura();
  nexLoop(nex_listen_list);

  if (g_modo == Modo::ARRANQUE && g_modo_antes != Modo::ARRANQUE) {
    Serial.println("Programando tiempo modo arranque");
    g_temp_contactor = ahora + TIEMPO_ARRANQUE_CONTACTOR;
    g_temp_inicio_ciclo = ahora + TIEMPO_MODO_INICIO_CICLO;
  }

  if (g_modo == Modo::INICIO_CICLO && g_modo_antes != Modo::INICIO_CICLO) {
    Serial.println("Programando tiempo modo inicio ciclo");
    g_temp_inicio_ciclo = ahora + TIEMPO_MODO_INICIO_CICLO;
  }

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
        g_modo = Modo::INICIO_CICLO;
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
      g_modo = Modo::CRUSERO;
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
      g_modo = Modo::FINAL_CICLO;
      g_temp_final_ciclo = ahora + TIEMPO_FINAL_DE_CICLO;
    }
  }

  if (g_modo == Modo::FINAL_CICLO) {
    bomba(true);
    llenado(false);
    fan(true);
    defrost(false);
    
    if (g_temp_final_ciclo <= ahora) {
      g_modo = Modo::DEFROST;
      g_temp_defrost = ahora + TIEMPO_DEFROST;
    }
  }

  if (g_modo == Modo::DEFROST) {
    fan(false);
    defrost(true);
    bomba(false);
    llenado(false);
    
    if (g_temp_defrost <= ahora) {
      g_modo = Modo::INICIO_CICLO;
    }
  }

  actualizarPantalla(flotador);
  informacionSerial(flotador);
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
    if (digitalRead(PIN_BOMBA) == LOW) {
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

void actualizarPantalla(bool flotador) {
  if (g_temp_serial <= ahora) {
    // tFlt.setText(flotador ? "Lleno" : "Vacio");
    // char buffer_temperatura[32];
    // dtostrf(g_temperatura, 6, 1, buffer_temperatura);
    // tTmp.setText(buffer_temperatura);
    // tContact.setText(contactor() ? "ON" : "OFF");
    // tMode.setText(modoATexto(g_modo));
    // tDefrost.setText(defrost() ? "ON" : "OFF");

    
    char buffer[32];

    if (g_modo == Modo::ARRANQUE) {
      itoa((g_temp_contactor - ahora) / 1000ul, buffer, 10);
      tTArranque.setText(buffer);
      tBombaArr.setText(bomba() ? "ON" : "OFF");
      tFanArr.setText(fan() ? "ON" : "OFF");
      tLlenadoArr.setText(llenado() ? "ON" : "OFF");
    }
  }

  if (g_modo == Modo::INICIO_CICLO && g_modo_antes != Modo::INICIO_CICLO) {
    sendCommand("page 5");
  }

  if (g_modo == Modo::CRUSERO && g_modo_antes != Modo::CRUSERO) {
    sendCommand("page 3");
  }
}