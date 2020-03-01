// Entradas
constexpr int PIN_FLOTADOR = 30;
constexpr int PIN_CONTROL_DE_FRIO = 32;
// Salidas
constexpr int PIN_DEFROST = 35;
constexpr int PIN_LLENADO = 37;
constexpr int PIN_FAN = 39;
constexpr int PIN_BOMBA = 33;

// Tiempos
constexpr auto TIEMPO_MODO_INICIO_CICLO = 20000ul;
constexpr auto TIEMPO_BOMBA_INICIO = 5000ul;
constexpr auto TIEMPO_PREVIO_LLENADO_CRUSERO = 30ul * 1000ul;
constexpr auto TIEMPO_FINAL_DE_CICLO = 4ul * 60ul * 1000ul;
constexpr auto TIEMPO_DEFROST = 6ul * 60ul * 1000ul + 20ul * 1000ul;
constexpr auto TIEMPO_MUESTRA_CONTROL_FRIO = 50ul;

// Calculo de temperatura
constexpr auto TEMPERATURA_LECTURA_1 = 10;
constexpr auto TEMPERATURA_CELSIUS_1 = double{0.0};

constexpr auto TEMPERATURA_LECTURA_2 = 100;
constexpr auto TEMPERATURA_CELSIUS_2 = double{50.0};

// Otros
constexpr auto MUESTRAS_CONTROL_FRIO = 100;

// Configuraciones de serial
constexpr auto MONITOR_SERIAL = true;
constexpr auto MENSAJES_ADICIONALES = false;

template<typename T, int SIZE, typename A>
class Promedio {
  T data[SIZE];
  T* current;
public:
  Promedio() : current(data) {}
  
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

Promedio<int, MUESTRAS_CONTROL_FRIO, long long> control_frio_promedio;

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_FLOTADOR, INPUT_PULLUP);
  pinMode(PIN_CONTROL_DE_FRIO, INPUT);
  pinMode(PIN_DEFROST, OUTPUT); digitalWrite(PIN_DEFROST, HIGH);
  pinMode(PIN_LLENADO, OUTPUT); digitalWrite(PIN_LLENADO, HIGH);
  pinMode(PIN_FAN, OUTPUT);     digitalWrite(PIN_FAN, HIGH);
  pinMode(PIN_BOMBA, OUTPUT);   digitalWrite(PIN_BOMBA, HIGH);
  pinMode(LED_BUILTIN, OUTPUT); digitalWrite(LED_BUILTIN, LOW);

  for (auto i {0}; i < MUESTRAS_CONTROL_FRIO; ++i) {
    control_frio_promedio.add_val(128);
  }
}

enum class Modo {
  IR_INICIO_CICLO,
  INICIO_CICLO,
  CRUSERO,
  FINAL_CICLO,
  DEFROST,
};

using Modo = Modo; // Blame Arduino IDE, https://arduino.stackexchange.com/questions/28133/cant-use-enum-as-function-argument
const char* modoATexto(Modo m) {
  switch (m) {
    case Modo::IR_INICIO_CICLO: return "IR_INICIO_CICLO";
    case Modo::INICIO_CICLO:    return "INICIO_CICLO";
    case Modo::CRUSERO:         return "CRUSERO     ";
    case Modo::FINAL_CICLO:     return "FINAL_CICLO ";
    case Modo::DEFROST:         return "DEFROST     ";
  }
  return                               "DESCONOCIDO ";
}

auto g_modo = Modo::IR_INICIO_CICLO;
auto g_temp_debounce_inicio = 0ul;
auto g_temp_bomba_inicio = 0ul;
auto g_temp_serial = 0ul;
auto g_temp_inicio_ciclo = 0ul;
auto g_temp_crusero = 0ul;
auto g_temp_final_ciclo = 0ul;
auto g_temp_defrost = 0ul;
auto g_temp_muestras_control_frio = 0ul;

auto ahora = 0ul;

auto flotador_antes = HIGH;

void patronParpadeoLed();
void inicializarContadores();
void muestraControlFrio();
void informacionSerial();

void loop() {
  const auto flotador = digitalRead(PIN_FLOTADOR) == HIGH ? LOW : HIGH;
  
  ahora = millis();

  patronParpadeoLed();
  inicializarContadores();
  muestraControlFrio();
  
  const auto control_frio_actual = static_cast<int>(control_frio_promedio.get_val());
  const auto temperatura = calcularTemperatura(control_frio_actual);

  // Cambio de modo inicio ciclo
  if (g_modo == Modo::IR_INICIO_CICLO) {
    g_modo = Modo::INICIO_CICLO;
    g_temp_inicio_ciclo = ahora + TIEMPO_MODO_INICIO_CICLO;
  }

  if (g_modo == Modo::INICIO_CICLO) {
    digitalWrite(PIN_FAN, LOW);
    digitalWrite(PIN_DEFROST, HIGH);
    if (g_temp_debounce_inicio <= ahora) {
      g_temp_debounce_inicio = ahora + 1000; // programar siguiente actualizacion para dentro de 1 segundo
      if (flotador == HIGH) { // menos de lleno
        digitalWrite(PIN_LLENADO, LOW);
      } else { // lleno
        digitalWrite(PIN_LLENADO, HIGH);
      }
    }

    if (flotador == LOW) { // lleno
      digitalWrite(PIN_BOMBA, LOW); // prender la bomba
      g_temp_bomba_inicio = ahora + TIEMPO_BOMBA_INICIO; // programar su apagado
      if (MENSAJES_ADICIONALES && g_temp_serial <= ahora) {
        Serial.println("Programando apagado de la bomba para dentro de 5s");
      }
    }

    if (g_temp_bomba_inicio <= ahora) {
      digitalWrite(PIN_BOMBA, HIGH);
    }

    if (g_temp_inicio_ciclo <= ahora) {
      g_modo = Modo::CRUSERO;
    }
  }

  if (g_modo == Modo::CRUSERO) {
    digitalWrite(PIN_FAN, LOW);
    digitalWrite(PIN_DEFROST, HIGH);
    digitalWrite(PIN_BOMBA, LOW);

    if (flotador == HIGH && flotador_antes == LOW) { // menos de lleno y justo cambio
      g_temp_crusero = ahora + TIEMPO_PREVIO_LLENADO_CRUSERO; // poner a correr tiempo
      if (MENSAJES_ADICIONALES) {
        Serial.println("Programando el llenado");
      }
    }
    
    if (g_temp_crusero <= ahora && flotador == HIGH) {
      digitalWrite(PIN_LLENADO, LOW);
      if (MENSAJES_ADICIONALES && g_temp_serial <= ahora) {
        Serial.println("Finalizado tiempo de espera para llenado, llenando");
      }
    }

    if (flotador == LOW) { // lleno
      digitalWrite(PIN_LLENADO, HIGH);
      if (MENSAJES_ADICIONALES && g_temp_serial <= ahora) {
        Serial.println("Lleno, apagando llenado");
      }
    }

    if (!control_frio_actual) {
      g_modo = Modo::FINAL_CICLO;
      g_temp_final_ciclo = ahora + TIEMPO_FINAL_DE_CICLO;
    }
  }

  if (g_modo == Modo::FINAL_CICLO) {
    digitalWrite(PIN_BOMBA, LOW);
    digitalWrite(PIN_LLENADO, HIGH);
    digitalWrite(PIN_FAN, LOW);
    digitalWrite(PIN_DEFROST, HIGH);
    
    if (g_temp_final_ciclo <= ahora) {
      g_modo = Modo::DEFROST;
      g_temp_defrost = ahora + TIEMPO_DEFROST;
    }
  }

  if (g_modo == Modo::DEFROST) {
    digitalWrite(PIN_FAN, HIGH);
    digitalWrite(PIN_DEFROST, LOW);
    digitalWrite(PIN_BOMBA, HIGH);
    digitalWrite(PIN_LLENADO, HIGH);
    
    if (g_temp_defrost <= ahora) {
      g_modo = Modo::IR_INICIO_CICLO;
    }
  }

  informacionSerial(flotador, control_frio_actual);

  flotador_antes = flotador;
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
}

void muestraControlFrio() {
  if (g_temp_muestras_control_frio <= ahora) {
    g_temp_muestras_control_frio = ahora + TIEMPO_MUESTRA_CONTROL_FRIO;

    const auto control_frio = analogRead(PIN_CONTROL_DE_FRIO);
    control_frio_promedio.add_val(control_frio);
  }
}

double calcularTemperatura(int valorSensor) {
  constexpr auto x1 = TEMPERATURA_LECTURA_1;
  constexpr auto x2 = TEMPERATURA_LECTURA_2;
  constexpr auto y1 = TEMPERATURA_CELSIUS_1;
  constexpr auto y2 = TEMPERATURA_CELSIUS_2;
  constexpr auto m = (y2 - y1) / (x2 - x1);
  constexpr auto b = y1 - m * x1;

  const auto x = valorSensor;

  return m * x + b;
}

void informacionSerial(int flotador, int lectura_frio) {
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
    if (digitalRead(PIN_LLENADO) == LOW) {
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

    Serial.print(" - LEC_TEMP: ");
    Serial.print(lectura_frio);

    Serial.print(", - TEMP_CALC: ");
    Serial.print(calcularTemperatura(lectura_frio));

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