
/*
PATILLAJE UTILIZADO DEL ARDUINO.
====================================================
D0	- RX	(Conexión con PC por USB) (no utilizar)
D1	- TX	(Conexión con PC por USB) (no utilizar)
D2	- (Sin conectar)
D3	- Tono (Conectada al altavoz)
D4	- Led rojo.
D5	- Led amarillo.
D6	- Led verde.
D7	- Led azul.
D8	- (Sin conectar)
D9	- (Sin conectar)
D10	- (Sin conectar)
D11	- (Sin conectar)
D12	- (Sin conectar)
D13	- (Sin conectar)
A0	- Pulsador azul.		 "Empezar/continuar partida". Utilizada como interrupción.
A1	- Pulsador verde.		 "Modo de juego ".			  Utilizada como interrupción.
A2	- Pulsador amarillo.	 "Dos jugadores".			  Utilizada como interrupción.
A3	- Pulsador rojo.		 "Un jugador".				  Utilizada como interrupción.
A4	- SDA	Utilizado por las comunicaciones I2C. Se conecta a la misma patilla de los dispositivos I2C conectados
A5	- SCL	Utilizado por las comunicaciones I2C. Se conecta a la misma patilla de los dispositivos I2C conectados
*/

#include <Arduino.h>

// Funciones

void animacionLeds();
void Configurar();
void Error();
byte LeePulsador();
void leeSecuencia();
void muestraSecuencia();
void muestraValor(byte);
void visualizaConfiguracion();

// Fin funciones

const byte pulsadorColor[] = { A3, A2, A1, A0 };

// Definimos los pin que controlan los leds de colores.
const byte LedColor[] = { 4, 5, 6, 7 };

// Pin donde se conecta el altavoz.
const byte pinAltavoz = 3;

const unsigned int tonos[] = { 523, 494, 440, 392 };	// Tonos asociados a cada uno de los valores.

const byte apagado = HIGH;				// Valor para apager un led.
const byte encendido = LOW;				// Valor para encender un led

const int tiempoMuestra = 1000;			// Tiempo duarnte el cual se enciende el led y suena la nota asociada.
const int tiempoPausa = 100;			// Pausa entre dos leds/notas consecutivos.
const int tiempoRespuesta = 2000;		// Tiempo máximo para responder (por valor).

const byte intA3 = (1 << PCINT11);		// Máscara para detectar pulsación en A3.
const byte intA2 = (1 << PCINT10);		// Máscara para detectar pulsación en A2.
const byte intA1 = (1 << PCINT9);		// Máscara para detectar pulsación en A1.
const byte intA0 = (1 << PCINT8);		// Máscara para detectar pulsación en A0.
const byte maximoJugadas = 50;			// Número máximo de jugadas.

// ####################	VARIABLES DE TRABAJO #######################

bool configurando = true;				// Almacena si estamos en modo configuración o juego.
bool player1 = true;					// Controla si el jugador 1 sigue activo.
bool player2 = false;					// Controla si el jugador 2 sigue activo.
bool modoAlterno = false;				// Modo alterno.
bool modoInverso = false;				// Modo inverso.
bool ok = true;							// El jugador ha contestado correctamente.

byte contador[] = { 0, 0 };				// Número de jugada.
byte jugada = 0;						// Valor de la jugada actual.
byte jugadas[maximoJugadas][2];			// Jugadas.
byte jugadores = 1;						// Número de jugadores.
byte turno = 0;							// Turno del jugador.

char format[17];						// Impresión en LCD con formato.

unsigned long lapso;					// Variable temporal que se utiliza para el cálculo del tiempo empleado por el jugador.
unsigned long resultado[2][2];			// Guarda los resultados de cada jugador (respuestas acertadas y tiempo empleado)

volatile bool Pulsado = false;			// true si el usuario a pulsado alguno de los 4 pulsadores.
volatile byte pulsador = 0;				// Número del pulsador accionado.

void setup() {
 	// Configuramos los pins de los leds y pulsadores.
	for (byte q = 0; q < 4; q++) {
		pinMode(pulsadorColor[q], INPUT);
		pinMode(LedColor[q], OUTPUT);
		digitalWrite(LedColor[q], apagado);
	}


	// Activar las interrupciones en pines A0 - A3
	// PCINT11 = A3,PCINT10 = A2,PCINT9 = A1,PCINT8 = A0,
	// Con las siguientes intrucciones ponemos a 1 (habilitado) los pines que queremos utilizar (A3-A0)
	// *digitalPinToPCMSK(pin) devuelve el puntero de la máscara encargada de las interrupciones de "pin".
	// *digitalPinToPCMSK(A5) es lo mismo que PCMASK1 ya que es esta la máscara que se ocupa de las interrupciones de A5.
	// (1 << PCINTXX) pone a 1 el bit correspondiente a la patilla asociada a esa interrupción)
	*digitalPinToPCMSK(A3) |= (1 << PCINT11);
	*digitalPinToPCMSK(A2) |= (1 << PCINT10);
	*digitalPinToPCMSK(A1) |= (1 << PCINT9);
	*digitalPinToPCMSK(A0) |= (1 << PCINT8);

	// Se pone a "1" el bit especificado. bit(X) es equivalente a (1 << X).
	PCIFR |= bit(PCIF1);	// borrar cualquier interrupción pendiente. (todas las patilla A0-A5 utilizan la misma interrupción.
	PCICR |= bit(PCIE1);	// Activar la interrupción que controla los pin A0-A5.

	// Entramos en el modo configuración.
	visualizaConfiguracion();
	Configurar();
}

void loop() {
  	if (ok) {
		// Generamos un nuevo número aleatorio y lo añadimos a la secuencia.
		// Ponemos una semilla para generar un número aleatorio.
		randomSeed(millis());
		jugadas[contador[turno - 1]++][turno - 1] = (random(1, 1001) % 4) + 1;		// Número aleatorio entre 1 y 4

		// Mostramos la secuencia al jugador.
		muestraSecuencia();

		// Leemos la secuencia del jugador.
		leeSecuencia();

		// Si el modo alterno está activado y los dos jugadores están activos, cambiamos de turno.
		if (modoAlterno && player1 && player2) {
			if (++turno > jugadores) { turno = 1; }
		}

	}
	else {
		// Si nos hemos equivocado finaliza el turno del jugador.
		// Tenemos que controlar si la partida se a acabado o continuamos.
		if (!(player1 || player2))
		{
			// Partida finalizada. Estamos a la espera hasta que pulsen el pulsador azul.
			// Estamos en "pausa" hasta que se pulse el pulsador azul.
			while (pulsador != 4) {}

			// Borramos la pulsación.
			Pulsado = false;
			pulsador = 0;

			// Entramos en el modo configuración para una nueva partida.
			configurando = true;
			visualizaConfiguracion();
			Configurar();
			ok = true;
		}
		else {
			// La partida continua. Cambiamos de turno y salimos del bucle.
			if (++turno > jugadores) { turno = 1; }
			ok = true;
			// Lanzamos la animación de leds para "separar" visualmente el fin de un jugador y el comienzo del siguiente.
			animacionLeds();
		}
	}
}

// Configura el modo de juego mediante los pulsadores.
void Configurar() {

	// Número de jugadores.
	while (configurando)
	{
		if (Pulsado) {
			switch (LeePulsador())
			{
			case 1:			// Rojo pulsado.
				jugadores = 1;
				break;
			case 2:			// Amarillo pulsado.
				jugadores = 2;
				break;
			case 3:			// Verde pulsado.
				// Si no están activados ninguno de los dos modos activamos el modo inverso.
				if (!modoInverso && !modoAlterno) {
					modoInverso = true;
					break;
				}
				// Si inverso activo y alterno no. Activamos alterno, desactivamos inverso.
				if (modoInverso && !modoAlterno) {
					modoInverso = false;
					modoAlterno = true;
					break;
				}
				// Si inverso desactivado y alterno activo. Activamos los dos.
				if (!modoInverso && modoAlterno) {
					modoInverso = true;
					break;
				}
				// Si los dos estan activoms, desactivamos los dos.
				if (modoInverso && modoAlterno) {
					modoInverso = false;
					modoAlterno = false;
					break;
				}
			case 4:			// Azul pulsado.
				// Finalizamos la configuración.
				configurando = false;
				// Empieza el jugador 1. 
				turno = 1;

				// Apagamos todos los led.
				digitalWrite(LedColor[0], apagado);
				digitalWrite(LedColor[1], apagado);
				digitalWrite(LedColor[2], apagado);
				digitalWrite(LedColor[3], apagado);
				// Ponemos "en blanco" las jugadas de ambos jugadores y a cero los contadores.
				jugada = 0;
				contador[0] = 0;
				contador[1] = 0;
				for (byte q = 0; q < maximoJugadas; q++) {
					for (byte w = 0; w < 2; w++) {
						jugadas[q][w] = 0;
					}
				}
				// Hacemos una pausa de un segundo para que el jugador tenga tiempo de prepararse.
				delay(1000);
			default:
				break;
			}
			if (configurando) {
				visualizaConfiguracion();
			}
		}
	}
}

// Función utilizada para saber si se ha accionado algún pulsador y leer el valor.
// Al leerlo restablece los valores para que no haya repeticiones.
byte LeePulsador() {
	byte valor;
	Pulsado = false;
	valor = pulsador;
	pulsador = 0;
	return valor;
}

// Muestra la secuencia de sonidos y colores actual.
void muestraSecuencia() {
	for (byte q = 0; q < contador[turno - 1]; q++)
	{
		muestraValor(jugadas[q][turno - 1]);
		// Pausa entre notas.
		delay(tiempoPausa);
	}
}

// Lee la secuencia de pulsaciones del judaor y comprueba si coincide con la grabada.
void leeSecuencia() {
	bool comprobando = true;			// Mientras este a true el programa espera una lectura.
	byte pulsacion = 0;					// Controla la pulsación por la que va el usuario.
	byte valor = 0;						// Controla la posición de la secuencia que debemos comparar en función del modo de juego.
	byte valorElegido = 0;				// Almacena el valor pulsado por el usuario.
	unsigned long tiempo = millis();	// Para controlar el tiempo.
	lapso = millis();

	while (comprobando)
	{
		// Comprobamos si se ha excedido el tiempo de espera.
		if (millis() - tiempoRespuesta > tiempo) {
			// Tiempo excedido. Fallo del usuario
			//Serial.println("Tiempo excedido.");
			comprobando = false;
			ok = false;
			// Sonido de fallo.
			Error();
			// Mostramos el valor correcto
			delay(200);
			muestraValor(jugadas[valor][turno - 1]);
			delay(200);

		}
		// Si el jugador ha pulsado comprobamos.
		if (Pulsado) {
			valorElegido = LeePulsador();
			if (modoInverso) {
				valor = contador[turno - 1] - pulsacion - 1;
			}
			else
			{
				valor = pulsacion;
			}
			// Mostramos el valor pulsado.
			muestraValor(valorElegido);

			// Comprobamos si es correcto.
			if (valorElegido != jugadas[valor][turno - 1]) {
				ok = false;
				comprobando = false;
				// Sonido de fallo.
				Error();
				// Mostramos el valor correcto
				delay(200);
				muestraValor(jugadas[valor][turno - 1]);
				delay(200);
			}
			// Si hemos contestado bien y ya hemos pulsado la secuencia completa salimos del bucle.
			if (ok && (++pulsacion == contador[turno - 1])) {
				resultado[turno - 1][0] = pulsacion;				// Pulsaciones correctas hasta ahora.
				resultado[turno - 1][1] += millis() - lapso;		// Tiempo empleado hasta el momento.
				comprobando = false; delay(500);
			}
			// Si se ha acccionado algún pulsador después de haber entrado, se borra.
			// De esta forma el jugador tiene que esperar a que termine de sonar una nota antes de pulsar la siguiente.
			Pulsado = false; pulsador = 0;
			// Actualizamos el valor del tiempo para controlar la siguiente pulsación.
			tiempo = millis();
		}
	}
}

// Muestra el valor seleccionado (luz y sonido).
void muestraValor(byte valor) {
	// Luz
	valor--;
	digitalWrite(LedColor[valor], encendido);
	// Sonido
	tone(pinAltavoz, tonos[valor], tiempoMuestra);
	// Pausa mientras suena.
	delay(tiempoMuestra);
	// Apagamos.
	digitalWrite(LedColor[valor], apagado);
}

// Error del usuario. Hacemos parpadear los leds y emitimos sonido.
void Error() {
	// Hacemos parpadear los leds y emitimos sonido.
	tone(pinAltavoz, 1234, 2000);

	for (byte q = 0; q < 8; q++) {
		for (byte w = 0; w < 4; w++) {
			digitalWrite(LedColor[w], q % 2);
		}
		delay(250);
	}
	if (turno == 1)
	{
		player1 = false;
	}
	else
	{
		player2 = false;
	}
}

// Enciende los leds correspondientes a la configuración seleccionada.
void visualizaConfiguracion() {
	// Estado leds "nº de jugadores.
	if (jugadores == 1) {
		digitalWrite(LedColor[0], encendido);
		digitalWrite(LedColor[1], apagado);
		player1 = true;
		player2 = false;
	}
	if (jugadores == 2) {
		digitalWrite(LedColor[0], apagado);
		digitalWrite(LedColor[1], encendido);
		player1 = true;
		player2 = true;
	}
	// Estado led "Modo inverso".
	digitalWrite(LedColor[2], !modoInverso);
	// Estado led "modo alterno".
	digitalWrite(LedColor[3], !modoAlterno);
}

// Animación visual de leds.
void animacionLeds() {
	// Apagamos leds.
	for (byte q = 0; q < 4; q++) {
		digitalWrite(LedColor[q], apagado);
	}
	// Hacemos una secuencia de izquierda a derecha.
	for (byte q = 0; q < 4; q++) {
		digitalWrite(LedColor[q], encendido);
		delay(150);
		digitalWrite(LedColor[q], apagado);
	}

	// Hacemos una secuencia de derecha a izquierda.
	for (byte q = 0; q < 4; q++) {
		digitalWrite(LedColor[3 - q], encendido);
		delay(150);
		digitalWrite(LedColor[3 - q], apagado);
	}
	delay(1500);
}

// Interrupción asociada a la pulsación de uno de los pulsadores. Se activa al pulsar y al soltar.
ISR(PCINT1_vect) {
	// Solo activamos el valor al pulsar. (El pulsador accionado tiene valor 0);
	if ((PINC & intA3) == 0) { Pulsado = true; pulsador = 1; }	// Pulsado el rojo.
	if ((PINC & intA2) == 0) { Pulsado = true; pulsador = 2; }	// Pulsado el amarillo.
	if ((PINC & intA1) == 0) { Pulsado = true; pulsador = 3; }	// Pulsado el verde.
	if ((PINC & intA0) == 0) { Pulsado = true; pulsador = 4; }	// Pulsado el azul.
}
