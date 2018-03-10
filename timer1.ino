/***
    timer control

    plusieur alarme en même temps

    Les alarmes sont inscrites dans le eerom donc ils ne sont pas perdus 
    Utilisation d'un RTC style ds3231
    Utilisation d'un écran oled waveshare  
    ajout d'un clavier    
***/

#include <EEPROM.h>
#include "ssd1331.h"
#include "RTClib.h"

RTC_DS3231 rtc;
DateTime now;


// definition de structure

typedef struct{
uint8_t jour,heure,minute,seconde;
uint8_t sortie,ONouOFF;
}alarme_struct;

#define NB_ALARME_MAX 100
//definition des alarmes
alarme_struct mesAlarmes[NB_ALARME_MAX];

// jour de semaine, heure,min, sortie, on/off
//ex:{
//{ 7,18,0,0,10,1}, // ON a 6PM sortie 10
//{ 7, 6,0,0,10,0} // OFF a 6 AM sortie 10
//}; 


uint8_t derniereSeconde;

void annuleAlarme(int alarmeIndex)
{
  // si le jour est 255 alors l'alarme est non fonctionnel
   mesAlarmes[alarmeIndex].jour=255;
}


int8_t setAlarme(int alarmeIndex, alarme_struct * nouvelleAlarme)
{     
      int loop;
      int start = alarmeIndex * sizeof(alarme_struct);
      mesAlarmes[alarmeIndex] = *nouvelleAlarme;
      uint8_t * pointeur = (uint8_t *) nouvelleAlarme;
      // enregistre aussi dans eeprom
      for(loop=0;loop< NB_ALARME_MAX; loop++)
         EEPROM.update(start + loop, *(pointeur++));
}


void lireAlarmeEErom(void)
{
   int8_t loop;
   uint8_t * pointeur= (int8_t *) &mesAlarmes[0];
   
   for(loop=0;loop< NB_ALARME_MAX * sizeof(alarme_struct);loop++)
     pointeur[loop] = EEPROM[loop];
}

void verifieAlarmes(void)
{
   int loop;
   for(loop=0;loop< NB_ALARME_MAX;loop++)
     {

        if(mesAlarmes[loop].jour == 255)
            continue; // ok alarme non utilisée, vérifie la prochaine
        
        if(mesAlarmes[loop].jour !=7)  // si c'est 7 alors c'est tout les jours
          if(mesAlarmes[loop].jour != now.dayOfTheWeek())
             continue; // ok pas le bon jour

        if(mesAlarmes[loop].heure != 0) // si c'est 255 alors c'est toute les heures
          if(mesAlarmes[loop].heure != now.hour())
             continue; // ok pas la bonne heure
        
        if(mesAlarmes[loop].minute != 255) // si c'est 255 alors c'est chaque minute
          if(mesAlarmes[loop].minute != now.minute())
             continue; // ok pas la bonne minute

        if(mesAlarmes[loop].seconde != now.second())
             continue; // ok pas la bonne second        

        // si nous sommes ici alors c'est que nous avons trouvé une alarme
        digitalWrite(mesAlarmes[loop].sortie, mesAlarmes[loop].ONouOFF);
        
     }

  
}

void setup() {
 pinMode(13, OUTPUT);  
// SSD1331_begin();
// SSD1331_clear();
  // maintenant lire le eerom et transférer les alarmes
 lireAlarmeEErom();   //pour l'instant ont assume la table
 derniereSeconde=99;  // force update dans la première loop() 



 // temporairement force effacement
 // et ajout d'un alarme qui utilise la led pour tester
 int loop;
 alarme_struct temp_alarm;
 for(loop=0;loop<NB_ALARME_MAX;loop++)
    annuleAlarme(loop);
 // créons une alarme qui allume la led à la dixième seconde
 temp_alarm={7,255,255,10,13,1};
 setAlarme(0,&temp_alarm);
 // créons une alarme qui éteind la led à la vingtième seconde
 temp_alarm={7,255,255,20,13,0};
 setAlarme(1,&temp_alarm);
 
} //End of setup function.



void loop() {   

  now = rtc.now();

  if(now.second() != derniereSeconde )
   {
     derniereSeconde= now.second();
     // ok une seconde de passé alors vérifions les alarmes
     verifieAlarmes();
   }
    delay(500);  // une précision de .5 seconde 
}
