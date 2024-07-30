/*
    Control remoto para ventilador de techo.
    Con arduino UNO / Nano.

    - Utiliza un remoto IR para controlar la velocidad.
    - Usa 2 botones para subir/bajar la velocidad.
    - Tiene un display de matriz de puntos para indicar si está encendido,
      y hace un juego de luces como si las paletas del ventilados estarían girando,
      en cada velocidad tiene una velocidad de giro diferente.
    - Se reemplazó el autotransformador y la llave selectora mecánica por este aparato.


    ISET 57
    Alumnos de tercer año.
    Año 2024.
*/

#include "IRremote/IRremote.hpp"
#include "RemotosUsados.h"  // aca estan los codigos usados

const int SENSOR_IR     = A5;                    // sensor IR
const int PIN_BAJA      = 12;                    // boton de bajada
const int PIN_SUBE      = A4;                    // boton de subida
const int BUZZER        = 13;                    // buzzer que indica el cambio de velocidad.
const int RELES[4]      = {A3, A2, A1, A0};      // 4 reles / 4 velocidades diferentes.
int cols[5]             = {7, 8, 9, 10, 11};     // display de 5x5 filas/columnas
int filas[5]            = {6, 5, 4, 3, 2};       //  "
int velocidades[5]      = {0, 150, 90, 40, 15};  // OFF, 1, 2, 3, 4
int mux                 = 1;
volatile int ReleNumero = 0;  // comienza apagado

// ENCIENDE O APAGA EL BUZZER Y EL LED DEL BOTON.
// estado!=0 enciende el buzzer.
// estado=0 ambos leds OFF, =1 led SUBE, =2 led BAJA, =3 ambos leds ON.
void BuzzerYLed(int estado)
{
    digitalWrite(BUZZER, estado != 0);
    if (estado == 0)
    {
        pinMode(PIN_SUBE, INPUT_PULLUP);  // apaga led del boton ^
        pinMode(PIN_BAJA, INPUT_PULLUP);  // apaga led del boton v
    }
    if ((estado & 1) != 0)
    {
        digitalWrite(PIN_SUBE, LOW);  // prende led del boton ^
        pinMode(PIN_SUBE, OUTPUT);    //  "
    }
    if ((estado & 2) != 0)
    {
        digitalWrite(PIN_BAJA, LOW);  // prende led del boton v
        pinMode(PIN_BAJA, OUTPUT);    //  "
    }
}

void AccionarReles()
{
    Serial.print("RELE ");
    Serial.println(ReleNumero);

    if (ReleNumero == 1)
    {
        digitalWrite(RELES[0], 1);
        digitalWrite(RELES[1], 0);
        digitalWrite(RELES[2], 0);
        digitalWrite(RELES[3], 0);
    }
    if (ReleNumero == 2)
    {
        digitalWrite(RELES[0], 0);
        digitalWrite(RELES[1], 1);
        digitalWrite(RELES[2], 0);
        digitalWrite(RELES[3], 0);
    }
    if (ReleNumero == 3)
    {
        digitalWrite(RELES[0], 0);
        digitalWrite(RELES[1], 0);
        digitalWrite(RELES[2], 1);
        digitalWrite(RELES[3], 0);
    }
    if (ReleNumero == 4)
    {
        digitalWrite(RELES[0], 0);
        digitalWrite(RELES[1], 0);
        digitalWrite(RELES[2], 0);
        digitalWrite(RELES[3], 1);
    }
    if (ReleNumero == 0)
    {
        digitalWrite(RELES[0], 0);
        digitalWrite(RELES[1], 0);
        digitalWrite(RELES[2], 0);
        digitalWrite(RELES[3], 0);
    }
}

void BotonSube()
{
    if (ReleNumero >= 4)
    {
        BuzzerYLed(1);  // llego al tope.
        delay(250);
        BuzzerYLed(0);
        return;
    }

    ReleNumero++;  // proxima velocidad.
    AccionarReles();

    BuzzerYLed(1);  // indicadores
    delay(50);
    BuzzerYLed(0);
}

void BotonBaja()
{
    if (ReleNumero <= 1)
    {
        BuzzerYLed(2);  // llego al tope.
        delay(250);
        BuzzerYLed(0);
        return;
    }

    ReleNumero--;  // proxima velocidad.
    AccionarReles();

    BuzzerYLed(2);  // indicadores
    delay(50);
    BuzzerYLed(0);
}

void BotonParar()
{
    ReleNumero = 0;  // STOP
    AccionarReles();

    // sonido que indica STOP:
    for (int i = 0; i < 3; i++)
    {
        BuzzerYLed(3);
        delay(50);
        BuzzerYLed(0);
        delay(100);
    }
}

void LeerInfrarrojo()
{
    //----------------------------------------------------------------------------
    // Muestra el valor del sensor IR si falla algo.
    //
    if (IrReceiver.decodedIRData.protocol == UNKNOWN)
    {
        // (tambien puede ser que el codigo sea muy nuevo y no esté en esta libreria)
        Serial.println(F("Ruido o codigo desconocido"));
        IrReceiver.printIRResultRawFormatted(&Serial, true);
    }
    IrReceiver.resume();

    uint16_t command = IrReceiver.decodedIRData.command;
    Serial.print("Codigo recibido: ");
    Serial.println(command);

    //----------------------------------------------------------------------------
    // Ahora analizo si el codigo me sirve:
    //

    // no quiero codigos repetidos dentro de este tiempo...
    // si transcurrieron menos de 500 ms y el comando es el mismo, no hago nada.
    static uint32_t UT = 0;   // ultimo tiempo
    static uint16_t UC = -1;  // ultimo comando
    if (millis() - UT < 500 && command == UC) return;
    UT = millis();
    UC = command;

    //----------------------------------------------------------------------------
    // detecto diferentes botones de varios remotos...
    //
    if (command == PROYECTOR_EPSON_UP || command == PROYECTOR_EPSON_LEFT || command == IR_ARDUINO_21)
    {
        BotonSube();
    }
    else if (command == PROYECTOR_EPSON_DOWN || command == PROYECTOR_EPSON_RIGHT || command == IR_ARDUINO_7)
    {
        BotonBaja();
    }
    else if (command == PROYECTOR_EPSON_ENTER || command == IR_ARDUINO_9)
    {
        BotonParar();
    }
    else
    {
        Serial.println(F("Comando desconocido"));
    }
}

// solo un PUNTO se verá encendido, pero como es mucha la velocidad del multiplexado,
// parece que está encendida una linea completa, horizontal, vertical o diagonalmente.
void EncenderEsteLed(int f, int c)
{
    if (IrReceiver.decode())
    {
        // llegó un codigo del remoto. Lo leo y actúo si es necesario.
        LeerInfrarrojo();
    }

    if (f < 0)
    {
        // asi apago el led central, solo ocurre cuando está apagado el ventilador.
        digitalWrite(filas[2], LOW);
        digitalWrite(cols[2], LOW);
    }
    else
    {
        static int lastf = 2, lastc = 2;
        digitalWrite(filas[lastf], LOW);  // apago el led anterior,
        digitalWrite(cols[lastc], HIGH);
        digitalWrite(filas[f], HIGH);  // prendo punto actual,
        digitalWrite(cols[c], LOW);

        // controlo el brillo cuando esta apagado.
        // (lo apago antes y le meto un delay)
        if (ReleNumero == 0)
        {
            delayMicroseconds(200);  // este es el valor de brillo
            digitalWrite(filas[f], LOW);
        }

        delay(mux);  // espero el tiempo de multiplexado,
        lastf = f;
        lastc = c;
    }

    // leer pulsadores:

    // guardo valores anteriores para ver si existe un cambio. (staticos)
    static int p1, p2, p1ant = 1, p2ant = 1, cont = 0;
    p1 = digitalRead(PIN_SUBE);
    p2 = digitalRead(PIN_BAJA);

    if (p1 != p1ant && p1 == 0)
    {
        BotonSube();
        cont = 0;  // controlo si se pulsa mas de 3 segundos
    }
    if (p2 != p2ant && p2 == 0)
    {
        BotonBaja();
        cont = 0;  // controlo si se pulsa mas de 3 segundos
    }
    p1ant = p1;  // guardo valores anteriores
    p2ant = p2;

    // si esta pulsado 3 seg algun boton, SE PARA EL VENTILADOR:
    if ((p2 == p2ant && p2 == 0) || (p1 == p1ant && p1 == 0))
    {
        cont++;
        if (cont == 1000)
        {
            BotonParar();
        }
    }
}

bool Vertical(int v, size_t tiempo)
{
    for (size_t t = 0; t < tiempo; t++)
    {
        for (int x = 0; x < 5; x++) EncenderEsteLed(v, x);
        if (ReleNumero != 0) return true;  // encendieron el ventilador
    }
    return false;
}

bool Horizontal(int h, size_t tiempo)
{
    for (size_t t = 0; t < tiempo; t++)
    {
        for (int x = 0; x < 5; x++) EncenderEsteLed(x, h);
        if (ReleNumero != 0) return true;  // encendieron el ventilador
    }
    return false;
}

void Cuadrado1(int tiempo)
{
    for (int b = 0; b < tiempo; b++)
    {
        for (int a = 0; a < 5; a++) EncenderEsteLed(a, 0);
        EncenderEsteLed(0, 1);
        EncenderEsteLed(4, 1);
        EncenderEsteLed(0, 2);
        EncenderEsteLed(4, 2);
        EncenderEsteLed(0, 3);
        EncenderEsteLed(4, 3);
        for (int a = 0; a < 5; a++) EncenderEsteLed(a, 4);
        if (ReleNumero != 0) return;  // encendieron el ventilador
    }
}

void Cuadrado2(int tiempo)
{
    for (int b = 0; b < tiempo; b++)
    {
        for (int a = 1; a < 4; a++) EncenderEsteLed(a, 1);
        EncenderEsteLed(1, 2);
        EncenderEsteLed(3, 2);
        EncenderEsteLed(1, 3);
        EncenderEsteLed(3, 3);
        for (int a = 1; a < 4; a++) EncenderEsteLed(a, 3);
        if (ReleNumero != 0) return;  // encendieron el ventilador
    }
}

// en realidad es el punto central
void Cuadrado3(int tiempo)
{
    for (int b = 0; b < tiempo; b++)
    {
        EncenderEsteLed(2, 2);  // punto central
        delay(5);
        if (ReleNumero != 0) return;  // encendieron el ventilador
    }
}

void Espiral()
{
    static uint8_t puntos[50] = {
        0, 0, 0, 1, 0, 2, 0, 3, 0, 4,  //
        1, 4, 2, 4, 3, 4, 4, 4,        //
        4, 3, 4, 2, 4, 1, 4, 0,        //
        3, 0, 2, 0, 1, 0,              //
        1, 1, 1, 2, 1, 3,              //
        2, 3, 3, 3,                    //
        3, 2, 3, 1,                    //
        2, 1, 2, 2,
    };
    for (int i = 0; i < 50; i += 2)
    {
        EncenderEsteLed(puntos[i], puntos[i + 1]);
        delay(11 + i / 3);
    }
    for (int i = 48; i >= 0; i -= 2)
    {
        EncenderEsteLed(puntos[i], puntos[i + 1]);
        delay(55 - i / 3);
    }
}

void LuzDeApagado()
{
    Serial.println("Vert");
    for (size_t r = 0; r < 22; r++)
    {
        for (int x = 0; x < 4; x++)
        {
            if (Vertical(x, 10)) return;
            delay(10);
        }
        for (int x = 0; x < 4; x++)
        {
            if (Vertical(4 - x, 10)) return;
            delay(10);
        }
    }
    Serial.println("Horiz");
    for (size_t r = 0; r < 22; r++)
    {
        for (int y = 0; y < 4; y++)
        {
            if (Horizontal(y, 10)) return;
            delay(10);
        }
        for (int y = 0; y < 4; y++)
        {
            if (Horizontal(4 - y, 10)) return;
            delay(10);
        }
    }
    Serial.println("Puntitos vert");
    for (size_t i = 0; i < 22; i++)
    {
        for (int b = 0; b < 5; b++)
        {
            for (int a = 0; a < 5; a++)
            {
                EncenderEsteLed(a, b);
                delay(30);
                if (ReleNumero != 0) return;  // encendieron el ventilador
            }
        }
    }
    Serial.println("Puntitos horiz");
    for (size_t i = 0; i < 22; i++)
    {
        for (int b = 0; b < 5; b++)
        {
            for (int a = 0; a < 5; a++)
            {
                EncenderEsteLed(b, a);
                delay(30);
                if (ReleNumero != 0) return;  // encendieron el ventilador
            }
        }
    }
    Serial.println("cuadrados");
    for (size_t i = 0; i < 22; i++)
    {
        Cuadrado1(8);
        Cuadrado2(15);
        Cuadrado3(10);
        EncenderEsteLed(-1, -1);  // apago un cachito
        delay(333);
        Cuadrado3(5);
        Cuadrado2(15);
    }
    Serial.println("espiral");
    for (size_t i = 0; i < 22; i++) Espiral();
}

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println("\n\n*******************************************************");
    Serial.println("  Control de ventilador");
    Serial.println("  ISET 57 año 2024");
    Serial.println("  https://github.com/MicroIset57/ventiladorControlado");
    Serial.println("*******************************************************\n");

    /*
        Solo puedo usar el "tone" antes de configurar el IR,
        porque ambos usan la interrupcion de TIMER.
    */
    for (size_t i = 600; i < 6000; i += 4) tone(BUZZER, i, 10);

    IrReceiver.begin(SENSOR_IR, DISABLE_LED_FEEDBACK);

    // Serial.print(F("Ready to receive IR signals of protocols: "));
    // printActiveIRProtocols(&Serial);
    // Serial.println("at pin " + String(SENSOR_IR));

    // desplazamiento array de leds
    for (int a = 0; a < 5; a++)
    {
        pinMode(filas[a], OUTPUT);
        pinMode(cols[a], OUTPUT);
        digitalWrite(filas[a], 0);  // apago todos los leds
        digitalWrite(cols[a], 1);
    }
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, 0);  // apago el buzzer
    pinMode(PIN_SUBE, INPUT_PULLUP);
    pinMode(PIN_BAJA, INPUT_PULLUP);
    for (size_t i = 0; i < 4; i++)
    {
        pinMode(RELES[i], OUTPUT);
        digitalWrite(RELES[i], 0);  // apago todos los reles
    }
}

void loop()
{
    // velocidad del ventilador: lo tomo del rele actualmente seleccionado.

    if (velocidades[ReleNumero] == 0)
    {
        // muestro que está apagado:
        LuzDeApagado();
    }
    else
    {
        // gira dependiendo de la velocidad:

        for (int x = 0; x < velocidades[ReleNumero]; x++)
        {
            for (int a = 0; a < 5; a++) EncenderEsteLed(4 - a, a);
        }
        for (int x = 0; x < velocidades[ReleNumero]; x++)
        {
            for (int a = 0; a < 5; a++) EncenderEsteLed(4 - a, 2);
        }
        for (int x = 0; x < velocidades[ReleNumero]; x++)
        {
            for (int a = 0; a < 5; a++) EncenderEsteLed(4 - a, 4 - a);
        }
        for (int x = 0; x < velocidades[ReleNumero]; x++)
        {
            for (int a = 0; a < 5; a++) EncenderEsteLed(2, a);
        }
    }
}
