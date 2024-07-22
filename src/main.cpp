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

const int SENSOR_IR     = A5;                    // sensor IR
const int PIN_BAJA      = 13;                    // boton de bajada
const int PIN_SUBE      = A4;                    // boton de subida
const int BUZZER        = 12;                    // buzzer que indica el cambio de velocidad.
const int RELES[4]      = {A3, A2, A1, A0};      // 4 reles / 4 velocidades diferentes.
int cols[5]             = {7, 8, 9, 10, 11};     // display de 5x5 filas/columnas
int filas[5]            = {6, 5, 4, 3, 2};       //  "
int velocidades[5]      = {0, 111, 80, 50, 20};  // OFF, 1, 2, 3, 4
int mux                 = 1;
volatile int ReleNumero = 1;

// forward.
void BotonParar();
void EncenderEsteLed(int f, int c);
void BotonBaja();
void AccionarReles();
void BotonSube();
void LeerInfrarrojo();


void LeerInfrarrojo()
{
    int command = IrReceiver.decodedIRData.command;
    Serial.println(command);
    delay(100);
    IrReceiver.resume();

    // detecto diferentes botones de varios remotos...
    if (command == 21 || command == 176)
    {
        BotonSube();
    }
    if (command == 7 || command == 178)
    {
        BotonBaja();
    }
    if (command == 9 || command == 133)
    {
        BotonParar();
    }
}

void BotonSube()
{
    ReleNumero++;
    if (ReleNumero > 4)
    {
        ReleNumero = 4;
        digitalWrite(BUZZER, 1);
        delay(40);
        digitalWrite(BUZZER, 0);
        delay(60);
    }

    // encender led
    digitalWrite(PIN_SUBE, LOW);
    pinMode(PIN_SUBE, OUTPUT);
    Serial.println(ReleNumero);
    digitalWrite(BUZZER, 1);
    delay(200);
    digitalWrite(BUZZER, 0);
    AccionarReles();
    pinMode(PIN_SUBE, INPUT_PULLUP);
}

void AccionarReles()
{
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

void BotonBaja()
{
    ReleNumero--;
    if (ReleNumero < 1)
    {
        ReleNumero = 1;
        digitalWrite(BUZZER, 1);
        delay(40);
        digitalWrite(BUZZER, 0);
        delay(60);
    }

    digitalWrite(PIN_BAJA, LOW);  // prende led
    pinMode(PIN_BAJA, OUTPUT);    // "
    Serial.println(ReleNumero);
    digitalWrite(BUZZER, 1);
    delay(200);
    digitalWrite(BUZZER, 0);
    AccionarReles();
    pinMode(PIN_BAJA, INPUT_PULLUP);  // apaga led
}

void BotonParar()
{
    digitalWrite(BUZZER, 1);
    delay(100);
    digitalWrite(BUZZER, 0);
    delay(100);
    digitalWrite(BUZZER, 1);
    delay(100);
    digitalWrite(BUZZER, 0);
    delay(100);
    digitalWrite(BUZZER, 1);
    delay(100);
    digitalWrite(BUZZER, 0);
    ReleNumero = 0;
    AccionarReles();
}

// solo un PUNTO se verá encendido, pero como es mucha la velocidad del multiplexado,
// parece que está encendida una linea completa, horizontal, vertical o diagonalmente.
void EncenderEsteLed(int f, int c)
{
    if (IrReceiver.decode())
    {
        // llegó un codigo del remoto. Lo leo y actúo si es necesario.
        LeerInfrarrojo();
        delay(400);
    }

    digitalWrite(filas[f], HIGH);
    digitalWrite(cols[c], LOW);
    delay(mux);
    digitalWrite(filas[f], LOW);
    digitalWrite(cols[c], HIGH);

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

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println("Control de ventilador");
    Serial.println("ISET 57 año 2024");

    IrReceiver.begin(SENSOR_IR, BUZZER);

    // desplazamiento array de leds
    for (int a = 0; a < 5; a++)
    {
        pinMode(filas[a], OUTPUT);
        pinMode(cols[a], OUTPUT);
    }
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, 0);
    pinMode(PIN_SUBE, INPUT_PULLUP);
    pinMode(PIN_BAJA, INPUT_PULLUP);
    pinMode(RELES[0], OUTPUT);
    pinMode(RELES[1], OUTPUT);
    pinMode(RELES[2], OUTPUT);
    pinMode(RELES[3], OUTPUT);
}

void loop()
{
    // velocidad del ventilador: lo tomo del rele actualmente seleccionado.
    int velocidad = velocidades[ReleNumero];
    if (velocidad == 0)
    {
        // muestro que está apagado

        EncenderEsteLed(2, 2);
    }
    else
    {
        // gira dependiendo de la velocidad:

        for (int x = 0; x < velocidad; x++)
        {
            for (int a = 0; a < 5; a++) EncenderEsteLed(4 - a, a);
        }
        for (int x = 0; x < velocidad; x++)
        {
            for (int a = 0; a < 5; a++) EncenderEsteLed(4 - a, 2);
        }
        for (int x = 0; x < velocidad; x++)
        {
            for (int a = 0; a < 5; a++) EncenderEsteLed(4 - a, 4 - a);
        }
        for (int x = 0; x < velocidad; x++)
        {
            for (int a = 0; a < 5; a++) EncenderEsteLed(2, a);
        }
    }
}
