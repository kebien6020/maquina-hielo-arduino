// Entradas
constexpr int PIN_FLOTADOR = 30;
constexpr int PIN_CONTROL_DE_FRIO = 32;
// Salidas
constexpr int PIN_DEFROST = 35;
constexpr int PIN_LLENADO = 37;
constexpr int PIN_FAN = 39;
constexpr int PIN_BOMBA = 33;

// Tiempos
constexpr auto TIEMPO_MODO_INICIO_CICLO = 10000ul;
constexpr auto TIEMPO_BOMBA_INICIO = 5000ul;
constexpr auto TIEMPO_PREVIO_LLENADO_CRUSERO = 2ul * 60ul * 1000ul + 30ul * 1000ul;
constexpr auto TIEMPO_FINAL_DE_CICLO = 3ul * 60ul * 1000ul;
constexpr auto TIEMPO_DEFROST = 6ul * 60ul * 1000ul;

// Configuraciones de serial
constexpr auto MONITOR_SERIAL = false;
constexpr auto MENSAJES_ADICIONALES = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_FLOTADOR, INPUT_PULLUP);
  pinMode(PIN_CONTROL_DE_FRIO, INPUT_PULLUP);
  pinMode(PIN_DEFROST, OUTPUT); digitalWrite(PIN_DEFROST, HIGH);
  pinMode(PIN_LLENADO, OUTPUT); digitalWrite(PIN_LLENADO, HIGH);
  pinMode(PIN_FAN, OUTPUT);     digitalWrite(PIN_FAN, HIGH);
  pinMode(PIN_BOMBA, OUTPUT);   digitalWrite(PIN_BOMBA, HIGH);
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

auto flotador_antes = HIGH;

void loop() {
  const auto flotador = digitalRead(PIN_FLOTADOR) == HIGH ? LOW : HIGH;
  const auto control_frio = digitalRead(PIN_CONTROL_DE_FRIO) == HIGH ? LOW : HIGH;
  const auto ahora = millis();

  // Inicializar contadores
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
        Serial.println("Programando el llenado para dentro de 2:30");
      }
    }
    
    if (g_temp_crusero <= ahora) {
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

    if (control_frio == LOW) {
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

  // Informacion por serial
  if (MONITOR_SERIAL && g_temp_serial <= ahora) {
    Serial.print("Modo: ");
    Serial.print(modoATexto(g_modo));

    Serial.print(" - Entradas: ");
    if (flotador == HIGH) {
      Serial.print("FLT ");
    } else {
      Serial.print("    ");
    }
    if (control_frio == HIGH) {
      Serial.print("CTL ");
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

  flotador_antes = flotador;
}
