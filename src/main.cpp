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
const int PIN_BAJA      = 12;                    // boton de bajada
const int PIN_SUBE      = A4;                    // boton de subida
const int BUZZER        = 13;                    // buzzer que indica el cambio de velocidad.
const int RELES[4]      = {A3, A2, A1, A0};      // 4 reles / 4 velocidades diferentes.
int cols[5]             = {7, 8, 9, 10, 11};     // display de 5x5 filas/columnas
int filas[5]            = {6, 5, 4, 3, 2};       //  "
int velocidades[5]      = {0, 150, 90, 40, 15};  // OFF, 1, 2, 3, 4
int mux                 = 1;
volatile int ReleNumero = 0;  // comienza apagado

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
    IrReceiver.resume();
    Serial.print("Comando IR: ");
    Serial.println(command);
    delay(100);
    if (IrReceiver.decodedIRData.address == 0)
    {
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
}

void SoundTopUp()
{
    tone(BUZZER, 523, 77);
    delay(150);
    tone(BUZZER, 784, 77);
    delay(150);
    tone(BUZZER, 698, 99);
}

void SoundTopDown()
{
    tone(BUZZER, 784, 77);
    delay(150);
    tone(BUZZER, 659, 77);
    delay(150);
    tone(BUZZER, 523, 99);
}

void SoundUp()
{
    // for (size_t i = 600; i < 6000; i += 4) tone(BUZZER, i, 10);
    tone(BUZZER, 2640, 100);
    delay(100);
}

void SoundDown()
{
    // for (size_t i = 600; i < 6000; i += 4) tone(BUZZER, 6000 - i, 10);
    tone(BUZZER, 2200, 100);
    delay(100);
}

void SoundApagar()
{
    for (size_t i = 200; i < 6000; i += 4) tone(BUZZER, 6000 - i, 10);
}

void BotonSube()
{
    ReleNumero++;
    if (ReleNumero > 4)
    {
        ReleNumero = 4;
        SoundTopUp();
    }

    // encender led
    digitalWrite(PIN_SUBE, LOW);  // prende led del boton
    pinMode(PIN_SUBE, OUTPUT);    // "

    Serial.print("RELE ");
    Serial.println(ReleNumero);
    AccionarReles();

    SoundUp();
    pinMode(PIN_SUBE, INPUT_PULLUP);  // apaga led del boton
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
        SoundTopDown();
    }

    digitalWrite(PIN_BAJA, LOW);  // prende led del boton
    pinMode(PIN_BAJA, OUTPUT);    //  "

    Serial.print("RELE ");
    Serial.println(ReleNumero);
    AccionarReles();

    SoundDown();
    pinMode(PIN_BAJA, INPUT_PULLUP);  // apaga led del boton
}

void BotonParar()
{
    ReleNumero = 0;
    AccionarReles();
    SoundApagar();
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
    Serial.println("Control de ventilador");
    Serial.println("ISET 57 año 2024");

    IrReceiver.begin(SENSOR_IR, DISABLE_LED_FEEDBACK);

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
