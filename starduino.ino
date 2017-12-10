/*

 Proyecto Juego de naves
 
 Por Mario Abajo Duran (marioabajo@gmail.com)
 Dedicado a mi amigo Ivan
 
 This example demostrates the use of this library to drive M50530
 chip based Lcd's display, specially the Seiko C338008 and
 Samsung 0282A displays.
 This are 8x24 character displays.
 
 The circuit:
 * LCD ioc1 pin to digital pin 10
 * LCD ioc2 pin to digital pin 11
 * LCD RW pin to digital pin 12
 * LCD Enable pin to digital pin 13
 * LCD D4 pin to digital pin 6
 * LCD D5 pin to digital pin 7
 * LCD D6 pin to digital pin 8
 * LCD D7 pin to digital pin 9
 * 4.7K ohm resistor from 12v to display's pin 14 (contrast)
 * Vcc to display's pin 15
 * Gnd to display's pin 1 and 16
 * 
 
 
 
 This example code is in the public domain.
 
*/

#include <Lcd50530.h>
#include <avr/pgmspace.h>

/*************************************************************/

/* Estructuras de datos */

struct estrella
{
  byte x;
  byte y;
  byte tipo;
  byte vel;
};
struct battleship
{
  byte x;
  byte y;
  byte h;
  byte w;
  short vida;
  unsigned long t_disparo;
  unsigned long t_mov;
  byte IA;
  const byte *imagen;
};
struct pk_battleship
{
  byte num;
  struct battleship *nave;
};
struct municion
{
  byte x;
  byte y;
  char incX;
  char incY;
  byte tipo;
  byte puntos;
  boolean activo;
};
struct pk_municion
{
  byte num;
  struct municion *balas;
};
struct explosion
{
  byte x;
  byte y;
  byte num;
  boolean activo;
};
struct pk_explosion
{
  byte num;
  struct explosion *expl;
};
struct datos_nave_mala
{
  byte id;
  byte h;
  byte w;
  short vida;
  const byte *imagen;
};

/* Inicializacion */

// LCD screen 4bits bus initialization
Lcd50530 lcd(6,7,8,9,13,12,11,10);


/* Definitons */

#define KEY_UP 4
#define KEY_DOWN 2
#define KEY_LEFT A1
#define KEY_RIGHT 5
#define KEY_A A0
#define KEY_B A2
#define SPEAKER 3
#define BATTERY A3

#define maX 24            // Tamaño usable de la pantalla durante el juego, ancho
#define maY 7             // Tamaño usable de la pantalla durante el juego, alto
#define estrellas 10      // Numero de estrellas concurrentes
#define maxBalas 30       // Maximo numero de balas en pantalla
#define maxMalos 5        // Maximo numero de malos en pantalla
#define maxTiposMalos 4   // Maximo numero de tipos de malos posibles


/* Variables */

static byte pantalla[maX][maY];
static byte sound_tone=0;
static int sound_period=0;
static unsigned long sound_tiempo=millis();

/* Bloques logicos para formar los niveles */

#define ESPERA1  1      // Espera 1s
#define ESPERA2  2      // Espera 2s
#define ESPERA5  5      // Espera 5s
#define ESPERA10  10    // Espera 10s
#define ESPERA50  50    // Espera 50s
#define ESPERA100  100  // Espera 100s
#define MALO1  110      // Malo de tipo 1
#define MALO2  111      // Malo de tipo 2
#define MALO3  112      // Malo de tipo 3
#define JEFE1  120      // Jefe de tipo 1
#define END  255        // Fin de nivel
  
const byte nivel1[] = {ESPERA5, MALO1, ESPERA5, MALO1, ESPERA5, MALO1, 
            ESPERA5, MALO1, MALO1, ESPERA10, MALO2, ESPERA10, MALO1, MALO2, 
            ESPERA5, MALO1, ESPERA5, MALO1, MALO1, MALO1, MALO1, ESPERA10,
            MALO2, MALO2, ESPERA10, MALO1, MALO2, MALO1, ESPERA2, MALO2, ESPERA5,
            MALO1, MALO1, MALO1, ESPERA2, MALO2, ESPERA10, JEFE1, END};

const byte nivel2[] = {MALO3, END};

const byte *niveles[] = {nivel1, nivel2};

/* Dibujos y animaciones */

const byte anm_explosion[] = {
  0xA5, 0x9C, 0x9B, 0x23, 0xDB, 0xA5
};

/*
|\____
=_,-'
*/
const byte player[] = {
  '|' , 0x8F, '_', '_', '_',  '_',
  '=' , '_',  ',', '-', '\'', ' '
};

const byte malo1[] = {
  0x8C, 
  0x17, 
  0x8D
};

/*
   /|
 -(_=
*/
const byte malo2[] = {
  ' ', ' ', '/', '|',
  '-', '(', '_', '='
};

/*
  (00=
 -(__/
*/
const byte malo3[] = {
  ' ', '(', '0', '0', '=',
  '-', '(', '_', '_', '/'
};

/*
     -=/ \
      / =/
(}-==( :[|
      \ =\
     -=\ /
*/
const byte jefe1[] = {
' ', ' ', ' ', ' ', ' ', '-', '=', '/', 0x86, 0x8F,
' ', ' ', ' ', ' ', ' ', ' ', '/', 0xA0, '=', '/',
'(', '}', '-', '=', '=', '(', 0xA0, ':', '[', '|',
' ', ' ', ' ', ' ', ' ', ' ', 0x8F, 0xA0, '=', 0x8F,
' ', ' ', ' ', ' ', ' ', '-', '=', 0x8F, '_', '/'
};

/*  Nuevas naves:
Para añadir malos nuevos hay que crear la imagen, 
añadir una linea de datos a la estructura tipos_malos,
aumentar maxTiposMalos, añadir el #define con el nombre
del malo, y revisar la funcion actualiza_nivel
*/

PROGMEM const struct datos_nave_mala tipos_malos[maxTiposMalos] = {
  { MALO1, 3, 1, 3, malo1 },
  { MALO2, 2, 4, 6, malo2 },
  { MALO3, 2, 5, 10, malo3 },
  { JEFE1, 5, 10, 100, jefe1 }
};



/*************************************************************/
/*             FUNCIONES PARA EL FONDO DE PANTALLA           */
/*************************************************************/

void nueva_estrella(struct estrella *a, byte x)
/* Genera una nueva estrella en la parte derecha de la pantalla */
{
  a->x=x;
  a->y=random(maY * 10);
  a->vel=random(2,10);
  switch (random(10))
  {
    case 0:
    case 1:
        a->tipo=0x2A;
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        a->tipo=0x2E;
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        a->tipo=0xA5;
        break;
  }
}

void generar_fondo(struct estrella *star)
/* Genera un fondo incial completamente aleaotiro */
{
  byte i;
  
  for (i=0;i<estrellas;i++)
    nueva_estrella(&star[i], random(maX*10));
}

void fondo(struct estrella *star)
/* Actualiza las estrellas de fondo */
{
  byte i;
  static unsigned long tiempo = millis();
  
  for (i=0;i<estrellas;i++)
    pantalla[star[i].x/10][star[i].y/10]=star[i].tipo;
    
  Serial.println(i);
  if (millis()-tiempo > 100)
  {
    for (i=0;i<estrellas;i++)
    {
      if (star[i].x >= star[i].vel)
        star[i].x -= star[i].vel;
      else
        nueva_estrella(&star[i],maX*10);
    }
    tiempo=millis();
  }
}


/*************************************************************/
/*          FUNCIONES PARA PANTALLA Y MARCADOR               */
/*************************************************************/

void marcador(short puntos, byte vidas, byte nivel)
/* Pone el marcador completo en la pantalla */
{
  byte i;
  
  lcd.setCursor(0,6);
  for (i=0;i<maX;i++)
    lcd.setUnderlineInc();

  lcd.setCursor(0,7);
  lcd.write(0x8B);
  lcd.setCursor(maX-1,7);
  lcd.write(0x8A);
  actua_marcador(puntos, vidas, nivel);
}

void actua_marcador(short puntos, byte vidas, byte nivel)
/* Actualiza los campos del marcador*/
{
  lcd.setCursor(1,7);
  lcd.print(" Lev:");
  lcd.print(nivel);
  lcd.print("|");
  lcd.write(0x9D);
  lcd.write('x');
  lcd.print(vidas);
  lcd.print('|');
  if (puntos<100)
    lcd.write(' ');
  if (puntos<10)
    lcd.write(' ');
  lcd.print(puntos);
}

void limpiar_pantalla()
/* Limpia el buffer de pantalla */
{
  memset(pantalla,' ',maY*maX);
}

void copiar_pantalla()
/* Copia el buffer de pantalla al LCD */
{
  byte i,j;
  
  for (j=0;j<maY;j++)
  {
    lcd.setCursor(0,j);
    for (i=0;i<maX;i++)
    {
      lcd.write(pantalla[i][j]);
    }
  }
}



/*************************************************************/
/*          FUNCIONES PARA EL MANEJO DE LAS NAVES            */
/*************************************************************/

void nave(struct battleship a)
/* Pone la nave en el buffer de pantalla */
{
  byte i,j;
  byte x,y;
  const byte *k;

  if (a.vida==0)
    return;
    
  x=a.x/10;
  for (i=0;i<a.h;i++)
  {
    y=i + a.y/32;
    if (y < maY)
    {
      for (j=0;j<a.w;j++)
      {
        if ((j+x < maX) && (a.imagen[i*a.w + j] != 0x20))
          pantalla[j+x][y]=a.imagen[i*a.w + j];
      }
    }
  }
}

void movimiento(struct battleship *a)
/* Actualiza la nave en base al movimiento de los mandos */
{
  short x,y;

  if (millis()-a->t_mov > 10)
  {
    x=digitalRead(KEY_RIGHT)*15 - digitalRead(KEY_LEFT)*15;
    y=digitalRead(KEY_UP)*30 - digitalRead(KEY_DOWN)*30;
    /*x=-(analogRead(A0)-500)/15;
    y=(analogRead(A1)-500)/15;*/
    if (x<1)
    {
      if (a->x<=-x)
        a->x = 0;
      else
        a->x += x;
    }
    else if (x>1)
    {
      if (a->x>((maX-a->w)*10-x))
        a->x = (maX-a->w)*10;
      else
        a->x += x;
    }
    if (y<1)
    {
      if (a->y<=-y)
        a->y = 0;
      else
        a->y += y;
    }
    else if (y>1)
    {
      if (a->y>((maY-a->h)*32-y))
        a->y = (maY-a->h)*32;
      else
        a->y += y;
    }
    
    a->t_mov=millis();
  }
}

void limpiar_naves(struct pk_battleship a)
/* Limpia el array de explosiones */
{
  byte i;

  for (i=0;i<a.num;i++)
  {
    a.nave[i].x=0;
    a.nave[i].y=0;
    a.nave[i].h=0;
    a.nave[i].w=0;
    a.nave[i].vida=0;
    a.nave[i].t_disparo=0;
    a.nave[i].t_mov=0;
    a.nave[i].IA=0;
    a.nave[i].imagen=NULL;
  }
}

void nuevo_malo( struct battleship *malos, byte tipo )
{
  byte i, j;
  
  for (i=0;i<maxMalos;i++)
  {
    if (malos[i].vida)
      continue;
    malos[i].x=240 + random(0,15);
    malos[i].y=random(0,160);
    
    for (j=0;j<maxTiposMalos;j++)
    {
      if (tipos_malos[j].id==tipo)
      {
        malos[i].h=tipos_malos[j].h;
        malos[i].w=tipos_malos[j].w;
        malos[i].vida=tipos_malos[j].vida;
        malos[i].imagen=tipos_malos[j].imagen;
        break;
      }
    }
    
    malos[i].t_disparo=0;
    malos[i].t_mov=0;
    break;
  }
}

byte malos_activos( struct pk_battleship malos )
/* Devuelve el numero de malos que siguen vivos */
{
  byte i,j=0;
  
  for (i=0;i<malos.num;i++)
  {
    if (malos.nave[i].vida)
      j++;
  }
  return j;
}

boolean colision( struct battleship a, struct battleship b)
/* Detecta colision entre 2 naves */
{
  if (a.vida==0 || b.vida==0)
    return LOW;
    
  if ((a.x/10 + a.w > b.x/10) && (a.x/10 < b.x/10 + b.w) && (a.y/32 + a.h > b.y/32) && (a.y/32 < b.y/32 + b.h))
    return HIGH;

  return LOW;
}

void movimiento_IA( struct battleship *a, struct battleship *prota )
/* Mueve las naves controladas por el microcontrolador */
{
  switch(a->IA)
  {
    // kamikaze
    case 0:
          break;

    // Apunta y dispara
    case 1:
          break;

    // borracho
    case 2:
          break;
          
    // 
  }
  
  if (a->x>120)
    a->x-=3;
}



/*************************************************************/
/*                FUNCIONES PARA LOS DISPAROS                */
/*************************************************************/

void disparos(struct battleship *a, struct pk_municion b)
/* Detecta las pulsaciones de los botones y realiza los disparos */
{
  byte i;

  if (millis()-a->t_disparo > 200)
  {
    if (! digitalRead(KEY_A)) // negated because of pullups
    {
      for (i=0;i<b.num;i++)
      {
        if (b.balas[i].activo==0)
        {
          b.balas[i].activo=1;
          b.balas[i].x=a->x + (a->w-1)*10;
          b.balas[i].y=a->y + (a->h-1)*32;
          b.balas[i].incY=0;
          b.balas[i].incX=10;
          b.balas[i].puntos=1;
          b.balas[i].tipo=0x2D;
          break;
        }
      }
      a->t_disparo=millis();
      /*sound_tone=200;
      sound_period=150;
      sound_tiempo=millis();*/
    }
  }
  /*if (millis()-a->t_disparo > 1000)
  {
    if (digitalRead(KEY_B))
    {
      a->t_disparo=millis();
    } 
  }*/
}

void disparos_IA(struct battleship *a, struct pk_municion b)
/* Disparos de las naves controladas por el microcontrolador */
{
  byte i,x,y;
  unsigned long t_disparo;

  /* Si no tiene vida no puede disparar */
  if (a->vida==0)
    return;

  /* Segun el tamño de la nave, el disparo es diferente */
  /* TODO: tal vez haya que cambiar esto y vincular el disparo con el movimiento de la IA */
  if (a->h==5)
  {
    t_disparo=1200;
    i=random(0,3);
    if ((a->h-1)*16*i < maY*32 - a->y)
      y=a->y + (a->h-1)*16*i;
    else
      y=maY*32;
      
    if (i!=1)
      if (a->x<200)
        x=a->x+50;
      else
        x=maX*10;
    else
      x=a->x;
  }
  else if (a->h==2)
  {
    t_disparo=2000;
    y=a->y + 32;
    x=a->x;
  }
  else
  {
    t_disparo=2000;
    y=a->y + (a->h-1)*16;
    x=a->x;
  }
    
  if (millis()-a->t_disparo > t_disparo && x<maX*10 && y<maY*32)
  {
    for (i=0;i<b.num;i++)
    {
      if (b.balas[i].activo==0)
      {
        b.balas[i].activo=1;
        b.balas[i].x=x;
        b.balas[i].y=y;
        b.balas[i].incY=0;
        b.balas[i].incX=-10;
        b.balas[i].puntos=1;
        b.balas[i].tipo=0x9C;
        break;
      }
    }
    a->t_disparo=millis();
  }
}

void limpiar_balas(struct pk_municion a)
/* Limpia el array de balas */
{
  byte i;

  for (i=0;i<a.num;i++)
  {
    a.balas[i].activo=0;
    a.balas[i].x=0;
    a.balas[i].y=0;
    a.balas[i].incY=0;
    a.balas[i].incX=0;
    a.balas[i].puntos=0;
    a.balas[i].tipo=0;
  }
}

void actuaDisparos(struct pk_municion a)
/* Actualiza los disparos que salen en pantalla */
{
  byte i;

  for (i=0;i<a.num;i++)
  {
    if (!a.balas[i].activo)
      continue;
    if (((short)(a.balas[i].x + a.balas[i].incX) < 0) || ((short)(a.balas[i].y + a.balas[i].incY) < 0) || 
        ((short)(a.balas[i].x + a.balas[i].incX) > maX*10) || ((short)(a.balas[i].y + a.balas[i].incY) > maX*10))
      a.balas[i].activo=0;
    else
    {
      a.balas[i].x += a.balas[i].incX;
      a.balas[i].y += a.balas[i].incY;
    }
    pantalla[a.balas[i].x/10][a.balas[i].y/32]=a.balas[i].tipo;
  }
}

byte acierto( struct battleship a, struct pk_municion b)
/* Detecta un impacto de un conjunto de balas en una nave */
{
  byte i=255;
  short x,y,x0,y0,x1,y1;
  
  if (a.vida==0)
    return 255;
  
  for (i=0;i<b.num;i++)
  {
    /*if ((a.vida > 0) && b.balas[i].activo && 
        (b.balas[i].x/10 >= a.x/10) && (b.balas[i].x/10 < a.x/10+a.w) && 
        (b.balas[i].y/32 >= a.y/32) && (b.balas[i].y/32 < a.y/32+a.h) && 
        (a.imagen[(b.balas[i].y/32 - a.y/32)*a.w + b.balas[i].x/10 - a.x/10]!=0x20))
      return i;*/
      
    if (b.balas[i].activo==0)
      continue;
      
    /* Optimizaciones */
    x0=a.x/10;
    y0=a.y/32;
    x=b.balas[i].x/10 - x0;
    y=b.balas[i].y/32 - y0;
    x1=(b.balas[i].x-b.balas[i].incX)/10 - x0;
    y1=(b.balas[i].y-b.balas[i].incY)/32 - y0;
    
    if (((x>=0 && y>=0 && x<a.w && y<a.h) || (x1>=0 && y1>=0 && x1<a.w && y1<a.h)) && (a.imagen[y*a.w + x]!=0x20))
      return i;
  }
  return 255;
}



/*************************************************************/
/*                FUNCIONES PARA LAS EXPLOSIONES             */
/*************************************************************/

void limpiar_explosiones(struct pk_explosion a)
/* Limpia el array de explosiones */
{
  byte i;

  for (i=0;i<a.num;i++)
  {
    a.expl[i].activo=0;
    a.expl[i].x=0;
    a.expl[i].y=0;
    a.expl[i].num=0;
  }
}

void actuaExplosiones(struct pk_explosion a)
/* Actualiza la animacion de las explosiones en pantalla */
{
  static unsigned long tiempo = millis();
  byte i;
  
  for (i=0;i<a.num;i++)
  {
    if (!a.expl[i].activo)
      continue;
    pantalla[a.expl[i].x][a.expl[i].y]=anm_explosion[a.expl[i].num];
    if (millis()-tiempo > 100)
    {
      a.expl[i].num++;
      if (a.expl[i].num>=6)
        a.expl[i].activo=0;
      /*else
      {
        sound_tone=a.expl[i].num*2+random(0,10);
        sound_period=200;
        sound_tiempo=millis();
      }*/
    }
  }
  if (millis()-tiempo > 100)
    tiempo=millis();
}

void nuevaExplosion(byte x, byte y, struct pk_explosion a)
/* Pone una nueva explosion en pantalla */
{
  byte i;
  
  for (i=0;i<a.num;i++)
  {
    if (a.expl[i].activo)
      continue;
    a.expl[i].x=x;
    a.expl[i].y=y;
    a.expl[i].activo=1;
    a.expl[i].num=0;
    break;
  }
}



/*************************************************************/
/*       FUNCIONES PARA EL MANEJO DE LA PARTIDA              */
/*************************************************************/

void game_over()
{
  unsigned long tiempo = millis();
  PROGMEM const char linea1[] = {0x80, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x81};
  PROGMEM const char linea2[] = {0x85, ' ', 'G', 'A', 'M', 'E', ' ', 'O', 'V', 'E', 'R', ' ', 0x84};
  PROGMEM const char linea3[] = {0x82, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x83};
  #define DO 50
  #define RE 75
  #define MI 100
  #define FA 125
  #define SO 150
  #define LA 175
  #define SI 200
  PROGMEM const byte melody_note[] = {MI, MI, SI, SI, DO, DO, SI, LA, LA, SO, SO, FA, FA, MI, 0};
  PROGMEM const byte melody_time[] = {300, 300, 300, 300, 300, 300, 600, 300, 300, 300, 300, 300, 300, 600, 1200};
  PROGMEM const byte melody_max_notes = 14;
  struct estrella star[estrellas];
  byte i,j=0;

  /*sound_period=0;
  generar_fondo(star);  */
  while (millis()-tiempo < 5000)
  {
    limpiar_pantalla();
    fondo(star);
    for (i=0;i<13;i++)
      pantalla[6+i][2]=linea1[i];
    for (i=0;i<13;i++)
      pantalla[6+i][3]=linea2[i];
    for (i=0;i<13;i++)
      pantalla[6+i][4]=linea3[i];
    copiar_pantalla();
    /* musica */
    /*if (millis() - sound_tiempo >= sound_period)
    {
      sound_tone=melody_note[j];
      sound_period=melody_time[j];
      sound_tiempo=millis();
      j++;
      if (j>melody_max_notes)
        j=0;
    }
    actualizar_sonido();*/
  }
  analogWrite(SPEAKER, 0);
}

void ani_ganar()
{
  unsigned long tiempo = millis();
  struct estrella star[estrellas];
  byte i;

  generar_fondo(star);  
  while (millis()-tiempo < 5000)
  {
    limpiar_pantalla();
    fondo(star);


    copiar_pantalla();
  }
}

byte actualiza_nivel( struct pk_battleship malos, byte *paso , const byte *nivel, unsigned long *tiempo)
{
  byte a = pgm_read_byte_near(nivel + *paso);
  
  switch (a)
  {
    case END:
      if (malos_activos(malos)==0)
        return 0;
      break;
    case ESPERA1:
    case ESPERA2:
    case ESPERA5:
    case ESPERA10:
    case ESPERA50:
    case ESPERA100:
      if (*tiempo==0)
        *tiempo=millis();
      else if (millis()-*tiempo >= a*1000)
      {
        *tiempo=0;
        *paso+=1;
      }
      break;
    case MALO1:
    case MALO2:
    case MALO3:
    case JEFE1:
      nuevo_malo(malos.nave, a);
      *paso+=1;
      break;
  }
  return 1;
}

byte ani_muerte_prota( struct battleship *a, struct pk_explosion b )
{
  byte x,y;
  if (a->x<240)
    a->x+=5;
  a->y+=5;

  if (random(0,2))
  {
    x=a->x/10 + random(0,a->w);
    y=a->y/32 + random(0,a->h);
    if (x<maX && y<maY)
      nuevaExplosion(x,y,b);
  }
  
  a->vida=1;
  nave(*a);
  a->vida=0;
    
  if (a->y>=240)
    return 0;
  return 1;
}

/* Taken and modified from http://arduino.cc/playground/Code/AvailableMemory */
unsigned short freeRAM() {
  uint8_t * heapptr, * stackptr;

  stackptr = (uint8_t *)malloc(4);          // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);      // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);           // save value of stack pointer
  
  return stackptr - heapptr;
}

void demo()
{
  unsigned long tiempo = millis();
  struct estrella star[estrellas];
  byte i,j;

  lcd.clear();
  generar_fondo(star);  
  while (1)
  {
    limpiar_pantalla();
    fondo(star);
    copiar_pantalla();
    
    /* Toca cualquier mando para salir de la demo */
    /*i=abs((analogRead(A1)-500)/15);
    j=abs((analogRead(A0)-500)/15);*/
    if ( digitalRead(KEY_UP) && digitalRead(KEY_DOWN) && digitalRead(KEY_LEFT) && digitalRead(KEY_RIGHT) && digitalRead(KEY_A) && digitalRead(KEY_B))  // inverse logic because of pullup's
      return;
  }
}

/*************************************************************/
/*                  FUNCIONES PARA LOS MENUS                 */
/*************************************************************/

byte menu_principal()
{
  unsigned long tiempo = millis();
  PROGMEM const char titulo[] = "> STAR DUINO <";
  PROGMEM const char inicio[] = {0x80, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x81};
  PROGMEM const char menu1[]  = {0x85, 'J', 'U', 'E', 'G', 'O', ' ', 'N', 'U', 'E', 'V', 'O', ' ', 0x84};
  PROGMEM const char menu2[]  = {0x85, ' ', 'C', 'O', 'N', 'T', 'I', 'N', 'U', 'A', 'R', ' ', ' ', 0x84};
  PROGMEM const char menu3[]  = {0x85, 'P', 'U', 'N', 'T', 'U', 'A', 'C', 'I', 'O', 'N', 'E', 'S', 0x84};
  PROGMEM const char fin[]    = {0x82, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x83};
  struct estrella star[estrellas];
  byte i, salir=0;
  char menu=1;
  short y;

  generar_fondo(star);
  lcd.clear();
  while (millis()-tiempo < 20000 && salir==0)
  {
    if (! digitalRead(KEY_UP))   // negated because of pullup's
      menu--;
    if (! digitalRead(KEY_DOWN)) // negated because of pullup's
      menu++;
    /*y=(analogRead(A1)-500)/15;
    if ( y > 5 )
      menu++;
    else if ( y < -5 )
      menu--;*/
    if (menu<1)
      menu=3;
    else if (menu>3)
      menu=1;
    
    if (! digitalRead(KEY_A))  // negated because of pullup's
      salir=menu;
    
    limpiar_pantalla();
    fondo(star);
    for (i=0;i<14;i++)
      pantalla[6+i][0]=titulo[i];
    for (i=0;i<14;i++)
      pantalla[6+i][2]=inicio[i];
    for (i=0;i<14;i++)
      pantalla[6+i][3]=menu1[i];
    for (i=0;i<14;i++)
      pantalla[6+i][4]=menu2[i];
    for (i=0;i<14;i++)
      pantalla[6+i][5]=menu3[i];
    for (i=0;i<14;i++)
      pantalla[6+i][6]=fin[i];
    
    /* Flecha de menu */
    pantalla[3][2+menu]=0x99;
    pantalla[4][2+menu]='=';
    pantalla[5][2+menu]=0x7E;
      
    copiar_pantalla();
    
    /* Debug */
    lcd.setCursor(0,7);
    lcd.print("Ram libre: ");
    lcd.print(freeRAM());
  }
  
  return salir;
}


/*************************************************************/
/*                     FUNCIONES DE SONIDO                   */
/*************************************************************/

void actualizar_sonido()
/* Actualiza el estado del altavoz */
{
    if (sound_tone>0 && millis()-sound_tiempo<=sound_period)
      analogWrite(SPEAKER, sound_tone);
    else
    {
      sound_tone=0;
      analogWrite(SPEAKER, 0);
    }
}

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(SPEAKER, HIGH);
    delayMicroseconds(tone);
    digitalWrite(SPEAKER, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };

  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

void playMelody(byte *notes, byte *beats, int length, int tempo)
{
  byte i;
  
  for (int i = 0; i < length; i++)
  {
    if (notes[i] == ' ')
    {
      delay(beats[i] * tempo); // rest
    }
    else
    {
      playNote(notes[i], beats[i] * tempo);
    }

    // pause between notes
    delay(tempo / 2); 
  }
}

/*
int length = 15; // the number of notes
char notes[] = "ccggaagffeeddc "; // a space represents a rest
int beats[] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 4 };
int tempo = 300;
*/

/*************************************************************/
/*                    FUNCIONES PRINCIPALES                  */
/*************************************************************/

byte game(byte nivel)
{
  /* Declaracion de variables */
  struct estrella star[estrellas];
  struct municion balasBuenas[10];
  struct pk_municion balasB = { 10, balasBuenas };
  struct municion balasMalas[30];
  struct pk_municion balasM = { 30, balasMalas };
  struct explosion explosiones[15];
  struct pk_explosion explo = { 15, explosiones };
  struct battleship malos[maxMalos];
  struct pk_battleship chicosmalos = {maxMalos, malos};
  struct battleship prota = { 50, 128, 2, 6, 3, 0, 0, 0, player };
  
  struct battleship *malo;
  byte paso=0;
  short puntos=0;
  byte i,aux;
  boolean fin=1;
  unsigned long tiempo=0;

  /* Preparacion del nivel */
  marcador(puntos,prota.vida,nivel);
  generar_fondo(star);
  limpiar_balas(balasB);
  limpiar_balas(balasM);
  limpiar_explosiones(explo);
  limpiar_naves(chicosmalos);
  
  /* Bucle de nivel */
  while (fin)
  {
    fin = actualiza_nivel(chicosmalos, &paso, niveles[nivel], &tiempo);
    
    /* preparar fondo */
    limpiar_pantalla();
    fondo(star);

    /* puesta al dia del protagonista */
    if (prota.vida)
    {
      movimiento(&prota);
      nave(prota);
    }
    disparos(&prota, balasB);
    actuaDisparos(balasB);

    /* Naves enemigas */
    for (i=0;i<maxMalos;i++)
    {
      malo=&malos[i];
      movimiento_IA(malo, &prota);
      nave(*malo);
      disparos_IA(malo, balasM);
      
      /* Detectar colisiones */
      if (prota.vida && colision(prota, *malo))
      {
        puntos+=malo->h*malo->w*malo->vida;
        malo->vida-=3;
        prota.vida--;
        actua_marcador(puntos,prota.vida,nivel);
        nuevaExplosion(malo->x/10 + malo->w/2, malo->y/32 + malo->h/2, explo);
      }
      
      /* Detectar aciertos del protagonista */
      aux=acierto(*malo, balasB);
      if (aux!=255)
      {
        if (malo->vida > balasBuenas[i].puntos)
          malo->vida -= balasBuenas[i].puntos;
        else
          malo->vida=0;
        
        puntos += malo->h*malo->w*balasBuenas->puntos;
        actua_marcador(puntos,prota.vida,nivel);
        balasBuenas[aux].activo=0;
        nuevaExplosion(balasBuenas[aux].x/10, balasBuenas[aux].y/32, explo);
      }
    }
    actuaDisparos(balasM);

    /* Detectar aciertos de las naves enemigas */
    if (prota.vida)
    {
      aux=acierto(prota, balasM);
      if (aux!=255)
      {
        prota.vida--;
        actua_marcador(puntos,prota.vida,nivel);
        balasMalas[aux].activo=0;
        nuevaExplosion(balasMalas[aux].x/10, balasMalas[aux].y/32, explo);
      }
    }

    if (prota.vida==0)
      fin=ani_muerte_prota(&prota,explo);
    
    /* Explosiones */
    actuaExplosiones(explo);
    
    /* Poner todo en el LCD */
    copiar_pantalla();
    
    /* Actualizar sonido */
    //actualizar_sonido();
  }
  
  //analogWrite(speakerPin, 0);
  if (prota.vida)
    ani_ganar();

  return prota.vida;
}

void setup() {
  lcd.begin();
  lcd.noCursor();
  pinMode(KEY_UP, INPUT);
  digitalWrite(KEY_UP, HIGH);    // Pullup resistor
  pinMode(KEY_DOWN, INPUT);
  digitalWrite(KEY_DOWN, HIGH);  // Pullup resistor
  pinMode(KEY_LEFT, INPUT);
  digitalWrite(KEY_LEFT, HIGH);  // Pullup resistor
  pinMode(KEY_RIGHT, INPUT);
  digitalWrite(KEY_RIGHT, HIGH); // Pullup resistor
  pinMode(KEY_A, INPUT);
  digitalWrite(KEY_A, HIGH);     // Pullup resistor
  pinMode(KEY_B, INPUT);
  digitalWrite(KEY_B, HIGH);     // Pullup resistor
  pinMode(SPEAKER, OUTPUT);
  Serial.begin(9600);
}

void loop()
{

  byte resultado;
  static byte nivel=0;
  
  switch (menu_principal())
  {
    case 0:
      demo();
      break;
    case 1:
      resultado=game(nivel);
      /* Si el prota muere */
      if (resultado==0)
      {
        game_over();
        nivel=0;
      }
      else
        nivel++;
      break;
    case 2:
      break;
  }
  
  
}

