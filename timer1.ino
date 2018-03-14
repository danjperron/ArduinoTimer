/***
    timer control

    plusieur alarme en même temps

    Les alarmes sont inscrites dans le eerom donc ils ne sont pas perdus 
    Utilisation d'un RTC style ds3231

***/

#include <EEPROM.h>
#include "RTClib.h"


//============== variable declaration

  // creation de l'objet real time clock
     RTC_DS3231 rtc;

  // creation de la classe DateTime
     DateTime now;

  // string contenant le nom des jours et mois pour la conversion numérique
   const char joursSemaine[7][9] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};
   const char mois[13][10]={"mois","Janvier","Février","Mars","Avril","Mai","Juin",\
                   "Juillet","Août","Septembre","Octobre","Novembre","Décembre"};


  // definition des pins de relais
   #define RELAIS_MAX 4
   const uint8_t relais[RELAIS_MAX]={ 3 , 4 , 5 , 6 };


   // definition de la structure contenant les alarmes

    typedef struct{
      uint8_t mois,jour,jourSemaine,heure,minute,seconde;
      uint8_t sortie,ONouOFF;
      }alarme_struct;

   // definition du nombre d'alarme maximum
    #define NB_ALARME_MAX 20

   // creation de la structures des alarmes 
     alarme_struct mesAlarmes[NB_ALARME_MAX];

   // creation de la memoire tammpon (buffer) pour le texte de commande
    #define RCV_BUFFER_MAX 100
    char  rcv_buffer[RCV_BUFFER_MAX];
    uint8_t rcv_pointeur;


   // creation de la table des jetons(token) contenant un pointeur de chaque paramètre.
   // Le jetons sont décodés de la mémoire tampon et leurs pointeurs sont enregistrer dans la table
    #define TOKEN_MAX 10
    char * tokens[TOKEN_MAX];
    uint8_t nbTokens;

   // enregistrement de la dernière seconde
   // de cette façon je ne vérifie qu'au seconde
    uint8_t derniereSeconde;


//========================   Les fonctions ===================

///////////////////// annuleAlarme
//
// Efface l'alarme spécifique de la structure et du EEPROM


void annuleAlarme(int alarmeIndex)
{
  int loop;
  // si le jour est 255 alors l'alarme est non fonctionnel
  int start = alarmeIndex * sizeof(alarme_struct);
  mesAlarmes[alarmeIndex].mois=255;
  for(loop=0;loop<sizeof(alarme_struct);loop++)
     EEPROM.update(start + loop, 255);
}


///////////////////// set Alarme
//
// Enregistre l'alarme spécifique dans la structure et dans le EEPROM

int8_t setAlarme(int alarmeIndex, alarme_struct * nouvelleAlarme)
{     
      int loop;
      int start = alarmeIndex * sizeof(alarme_struct);
      mesAlarmes[alarmeIndex] = *nouvelleAlarme;
      uint8_t * pointeur = (uint8_t *) nouvelleAlarme;
      // enregistre aussi dans eeprom
      for(loop=0;loop< sizeof(alarme_struct); loop++)
         EEPROM.update(start + loop, *(pointeur++));
}


///////////////////// lireAarmeEErom
//
// Lecture de l'EEPROM et transferer le tout dans la structure 'mesAlarmes'

void lireAlarmeEErom(void)
{
   int loop;
   uint8_t * pointeur= (int8_t *) &mesAlarmes[0];
   
   for(loop=0;loop< NB_ALARME_MAX * sizeof(alarme_struct);loop++)
     pointeur[loop] = EEPROM[loop];
}



///////////////////// splitToken
//
//  Creer une table de pointeur de chaque paramètre dans la string séparé par un espace 
//  Retourne le nombre de jeton trouvé

int8_t splitTokens(char *commande)
{
   uint8_t index=0;  // nombre de commande séparée
   uint8_t token=1;  // J'ai un token

   while(index<TOKEN_MAX)
   {

      while((*commande) != 0)
        {
          if(*commande == ' ')
            {
              // ok un espace donc j'ai un token qui suit
              *commande=0;
              token = 1;
            }
           else
            if(token)
             {
              // ok sauvons le debut du prochain token
              tokens[index++]= commande;
              token = 0;
             }
           if(*commande == '\r')
            {
              *commande=0;
             break;
            }
          commande++;
        }
        if(*commande == 0) break;
   }
   return index;
}

///////////////////// decimal02
//
//  Converti les valeurs de temps en texte  

void decimal02(uint8_t t)
{
   if(t == '*')
     Serial.print("*");  // ok cela veut dire que la valeur est peut importe
   else
   {
    if(t<10)
     Serial.print("0");  // valeur plus petit que 10 donc il faut ajouter un zéro
    Serial.print(t);
   }
}

///////////////////// syntaxeErreur
//
void syntaxeErreur()
{
  Serial.println(F("Erreur de syntaxe"));
}



///////////////////// isStringNumber
//
//  retourne vrai si la string est seulement des chiffres
//

bool isStringNumber(char * value)
{
   for(;*value!=0;value++)
        if(!isDigit(*value)) return false;
    return true;  
}

//////////////////////////  ValidToken
// retourne la valeur du token dans *valeurOut
// input: 
//     value ->valeur ascii à convertir
//     tout   -> est-ce que '*' est possible
//     pairImpair -> est-ce que 'P' ou 'I' est possible
//  retour:
//  valeur >= 0   retourne valeur
//  sur erreur retourne -1
   
int16_t ValideToken(char * valeurIn, bool tout, bool pairImpair,int16_t * valeurOut)
{
  // verification de '*'
  if(tout)
   {
     if(valeurIn[0]=='*')
     {
        *valeurOut='*';
        return 1;
     }
   }

   // verification de 'I'  
   if(pairImpair)
   {

     if(valeurIn[0]=='I')
       {
         *valeurOut='I';
         return 1;
       }

     if(valeurIn[0]=='P')
      {
        *valeurOut='P';
        return 1;
      }
   }

      // ok si nous sommes la alors ce n'est que des chiffres
    if(!isStringNumber(valeurIn))
       {
        syntaxeErreur();
        return -1;
       }
   
    *valeurOut=atoi(valeurIn);
    return 1;
   
}



///////////////////// setSortie
//
// Envoie la bonne sortie au bon relais

void  setSortie(uint8_t laSortie,uint8_t laValeur)
{
   if(laSortie < RELAIS_MAX)
     {
        digitalWrite(relais[laSortie], laValeur==0 ? LOW : HIGH);
     }

}


///////////////////// verifieAlarmes
//
// Scanner toutes les alarmes et appliquer le changement au sortie
//

void verifieAlarmes(void)
{
   int loop;
   for(loop=0;loop< NB_ALARME_MAX;loop++)
     {
        if(mesAlarmes[loop].mois == 255)
            continue; // ok alarme non utilisée, vérifie la prochaine        

        if(mesAlarmes[loop].mois != '*')
           if(mesAlarmes[loop].mois!= now.month())
            continue; // ok pas le bon mois
            
        if(mesAlarmes[loop].jour == 'I')  // si c'est I alors c'est les jours impairs
         {
          if((mesAlarmes[loop].jour % 2) == 0)
               continue; // ok pas jour impair
         }
        else if(mesAlarmes[loop].jour == 'P')  // si c'est I alors c'est les jours pairs
         {
          if((mesAlarmes[loop].jour % 2) == 1)
              continue; // ok pas jour impair
         }
        else if(mesAlarmes[loop].jour != '*')  // si c'est * alors c'est tout les jours
          if(mesAlarmes[loop].jour != now.day())
             continue; // ok pas le bon jour

  
          if(mesAlarmes[loop].jourSemaine!='*') // si c'est alrs c'est tout les jours de semaine             
          if(mesAlarmes[loop].jourSemaine != now.dayOfTheWeek())
             continue; // ok pas le bon jour


        if(mesAlarmes[loop].heure != '*') // si c'est '*' alors c'est toute les heures
          if(mesAlarmes[loop].heure != now.hour())
             continue; // ok pas la bonne heure
        
        if(mesAlarmes[loop].minute != '*') // si c'est '*'alors c'est chaque minute
          if(mesAlarmes[loop].minute != now.minute())
             continue; // ok pas la bonne minute

        if(mesAlarmes[loop].seconde != now.second())
             continue; // ok pas la bonne second        

        setSortie(mesAlarmes[loop].sortie,mesAlarmes[loop].ONouOFF);
     }

  
}



///////////////////// decodeSerie
//
// Cette fonction accumule les caractères série
// sans faire de loupe d'attente.
//
// retourne 1  si elle reçoit un cariage return
// sinon elle retourne 0
// si elle reçoit un "escape" elle reset tout à zéro

uint8_t decodeSerie()
{
   uint8_t rcv_car;

  while(Serial.available() >0)
   {
  
     rcv_car= Serial.read();

     if(rcv_car == '\n')
       return 0 ;  // ok saute le line feed

     if(rcv_car == 27)
      {
       // ok commande escape oublions tout
          rcv_pointeur=0;
          return 0;
      }

       
     rcv_buffer[rcv_pointeur++]=rcv_car;
     if(rcv_pointeur >= RCV_BUFFER_MAX)
       { 
          // trop de caractère effacons tout
          rcv_pointeur=0;
          return 0;
       }
     rcv_buffer[rcv_pointeur]=0;

     if(rcv_car == '\r')
       {
          // ok J'ai une fin de commande
          return 1;
       }
   }
       return 0;
}
 

///////////////////// Commande Aide
//
// Commande d'affichage d'aide lorsque la commande est H ou ?

void   aide()
{
  Serial.println(F("Liste des Commandes\n"\
                   "\tH,? : Affiche l'aide des commandes\n"\
                   "\tT   : Affiche l'heure et la date\n"\
                   "\tR   : Affiche le status des relais\n"\
                   "\tC   : configure RTC\n"\
                   "\t      S YYYY MM JJ hh mm ss\n"\
                   "\tL   : Affiche toute les alarmes\n"\
                   "\tA   : ajoute alarme\n"\
                   "\t      A MM JJ W hh mm ss R E\n"\
                   "\t      MM=mois(1..12,*) JJ=jour(1..31,*) W=Jour semaine(0=dimanche)\n"\
                   "\t      hh=heure(0..23,*) mm=minute(0..59,*) ss=seconde(0..59)\n"\
                   "\t      R=relais sortie (1..4)  E=0/1 OFF/ON\n"\
                   "\tE   : Enlève Alarme\n"\
                   "\t      numero=Numéro Alarme\n"));
}


///////////////////// Commande afficheTemps
//
// Afficher la date et le temps du RTC

void afficheRTC()
{

Serial.print(joursSemaine[now.dayOfTheWeek()]);
Serial.print(" ");
Serial.print(now.day());
Serial.print(" ");
Serial.print(mois[now.month()]);
Serial.print(" ");
Serial.print(now.year());
Serial.print(" ");
decimal02(now.hour());
Serial.print(":");
decimal02(now.minute());
Serial.print(":");
decimal02(now.second());
Serial.println("");
}


///////////////////// configureRTC
//
// Lire la commande de configuration RTC
// et aporter les modifications si tout est OK

void configureRTC()
{
  
  // ok set horloge
  int16_t t_year,t_month,t_day,t_hour,t_min, t_sec;
  
  // avons-nous tout les tokens
    if(nbTokens != 7)
       {
         syntaxeErreur();
         return;
       }
  // année
  if(!ValideToken(tokens[1],false,false,&t_year)) return;
 
  // mois
  if(!ValideToken(tokens[2],false,false,&t_month)) return;
     
  // jour
  if(!ValideToken(tokens[3],false,false,&t_day)) return;
  
  // heure
  if(!ValideToken(tokens[4],false,false,&t_hour)) return;

  // minute
  if(!ValideToken(tokens[5],false,false,&t_min)) return;

  // seconde
  if(!ValideToken(tokens[6],false,false,&t_sec)) return;

  Serial.println(t_year);
  Serial.println(t_month);
  Serial.println(t_day);
  Serial.println(t_hour);
  Serial.println(t_min);
  Serial.println(t_sec);

  rtc.adjust(DateTime(t_year,t_month,t_day,t_hour,t_min,t_sec));
  Serial.println("RTC ajusté");
  afficheRTC();

}


///////////////////// ajouteAlarme
//
// Lire la commande d'ajout d'alarme
// et ajouter dans la structures 'mesAlarmes' et EEPROM si c'est ok.
// 

void ajouteAlarme()
{
  alarme_struct dt;
  
  uint8_t itemp;
  int16_t wtemp;
  // numero un cherche un position libre
  for(itemp=0;itemp<NB_ALARME_MAX;itemp++)
    if(mesAlarmes[itemp].mois==255) break;

   if(itemp==NB_ALARME_MAX)
     {
       //toutes les alarmes sont utilisés
       Serial.println(F("Toutes les alarmes sont prises"));
       return;
     }
// ok format Mois jour jourSemaine heure minute seconde relais ON/OFF
// vérifions si nous avons le nombre de token

    if(nbTokens != 9)
       {
         syntaxeErreur();
         return;
       }

  //  mois
 if(!ValideToken(tokens[1],true,false,&wtemp)) return;
 dt.mois=wtemp;
 
  // jour
  if(!ValideToken(tokens[2],true,true,&wtemp)) return;
  dt.jour=wtemp;

  // jour de semaine
  if(!ValideToken(tokens[3],true,false,&wtemp)) return;
  dt.jourSemaine=wtemp;
  
  // heure
  if(!ValideToken(tokens[4],true,false,&wtemp)) return;
  dt.heure=wtemp;

  //minute
  if(!ValideToken(tokens[5],true,false,&wtemp)) return;
  dt.minute=wtemp;

  // seconde
  if(!ValideToken(tokens[6],false,false,&wtemp)) return;
  dt.seconde=wtemp;

  // sortie
  if(!ValideToken(tokens[7],false,false,&wtemp)) return;
  if((wtemp<0) || (wtemp >= RELAIS_MAX))
    {
      syntaxeErreur();
      return;
    }
  dt.sortie=wtemp;

  // ON ou OFF
  if(!ValideToken(tokens[8],false,false,&wtemp)) return;
  dt.ONouOFF=wtemp >0 ? 1 : 0;
  
  // ok faisons le transfer
  setAlarme(itemp,&dt);
  Serial.print(F("Alarme "));
  Serial.print(itemp+1);
  Serial.println(F(" ajoutée"));
  afficheAlarme(itemp);
} 



///////////////////// enleveAlarme
//
// Commande pour enlever un alarme spécifique
void enleveAlarme()
{
  uint8_t itemp;
  if(nbTokens != 2)
    {
      syntaxeErreur();
      return;
    }

    // faut'il enlever toute les alarmes
    if(((String) tokens[1]).length()==1)
       if(tokens[1][0]=='*')
        {
           // ok il faut enlever toutes 
           for(itemp=0;itemp<NB_ALARME_MAX;itemp++)
              annuleAlarme(itemp);
           Serial.println(F("Aucune Alarme restante"));   
           return;
        }
                 

    // est-ce que le paramètre contient uniquement des chiffres
    // ok quel est l'alarme a enlever
    if(!isStringNumber(tokens[1]))
         {
           syntaxeErreur();
           return;
         }


     itemp = atoi(tokens[1]);
     if(itemp == 0)
       {
         syntaxeErreur(); // si atoi ne converti pas alors il retourne 0
         return;
       }
       
     itemp--;  //decrement  parce la table c'est 0..NB_ALARME_MAX  
     if(itemp > NB_ALARME_MAX)
      {
       syntaxeErreur();
       return;
      }

      annuleAlarme(itemp);
      Serial.println(F("Alarme "));
      Serial.print(itemp+1);
      Serial.println(F(" enlevée"));
}



///////////////////// afficheAlarme
//
// Affiche l'alarme spécifique

void afficheAlarme(uint8_t alarme)
{
    uint8_t utemp;
    if(alarme >= NB_ALARME_MAX)
       return;
    if(mesAlarmes[alarme].mois == 255)
       return;  // alarme inactif
    // print alarme nb
    if(alarme<10)
      Serial.print(" ");
    Serial.print(alarme+1);
    Serial.print("- ");
    // print jour de semaine
    utemp = mesAlarmes[alarme].jourSemaine;
    if(utemp != '*')
     {
      Serial.print(joursSemaine[utemp]);
      Serial.print(" ");
     }
    else
     {
       utemp = mesAlarmes[alarme].jour;
       if(utemp == '*')
         Serial.print("jour=*");
       else if(utemp == 'I')
         Serial.print("jour impair");
       else if(utemp == 'P')
         Serial.print("jour pair");  
       else
         Serial.print(utemp);
       Serial.print(" ");
     }
    // print Mois
    utemp= mesAlarmes[alarme].mois;
    if(utemp == '*')
      Serial.print("mois=*");
    else
      {
       Serial.print(mois[utemp]);
      }
    Serial.print(" ");
    // heure
    decimal02(mesAlarmes[alarme].heure);
    Serial.print(":");
    decimal02(mesAlarmes[alarme].minute);
    Serial.print(":");
    decimal02(mesAlarmes[alarme].seconde);
    Serial.print(F(" relais #"));
    Serial.print(mesAlarmes[alarme].sortie);
    if(mesAlarmes[alarme].ONouOFF == 0)
       Serial.println(F(" OFF"));
    else
       Serial.println(F(" ON"));
}

///////////////////// ajouteAlarme
//
//   Liste toutes les alarmes

void listeAlarmes()
{
  uint8_t loop;
  Serial.println(F("Liste des alarmes:"));
  
  for(loop=0;loop<NB_ALARME_MAX;loop++)
     afficheAlarme(loop);
  Serial.println("");
}




///////////////////// infoRelais
//

void infoRelais()
{
  uint8_t loop;
  Serial.println(F("Status des relais"));
  for(loop=0;loop<RELAIS_MAX;loop++)
    {
        Serial.print(loop);
        Serial.print(":");
        Serial.print(digitalRead(relais[loop]) ? "ON " : "OFF");
        Serial.print("  ");
    }
    Serial.println();
    
}

void setup() {
  int loop;
//  les sorties en OUTPUT et OFF
  for(loop=0;loop<sizeof(relais);loop++)
   {
     pinMode(relais[loop],OUTPUT);
     digitalWrite(relais[loop],LOW);  
   }

  if (! rtc.begin()) {
    Serial.println(F("RTC non disponible"));
    while (1);
  }

 // maintenant lire le eerom et transférer les alarmes
 lireAlarmeEErom();   
 derniereSeconde=99;  // force update dans la première loop() 

 // active la communication série
 Serial.begin(9600);
 Serial.println(F("Timer1. Communication série.\n(c) D.J.Perron Mars 2018.\nTapez H pour de l'aide. N'oubliez pas le retour de chariot"));
 //mesAlarmes[0]={ '*','*','*',20,6,0,0,1};
 //mesAlarmes[1]={ '*','*','*',20,6,30,2,1};
} //End of setup function.



void loop() {   
  uint8_t itemp;
  uint8_t loop;
  char key;
  // Lire la commande
  // un enregistrer la commande de la communication série
  // dans rcv_buffer
  // verifions si nous avons une nouvelle commande
  if(decodeSerie())
   {
      // ok separons la commande en tokens
      // la fonction retourne le nombre de token
      nbTokens= splitTokens(rcv_buffer);
       // J'ai les tokens alors effaçons la commande
       rcv_pointeur=0;
       itemp = ((String)tokens[0]).length();
       
       if(((String)tokens[0]).length() == 1)
       {
          key= toupper(tokens[0][0]);

          switch(key) {
           case 'H': 
           case '?':
                     aide();
                     break;
           case 'T':
                     afficheRTC();
                     break;
           case 'C':
                     configureRTC();
                     break;
           case 'A': 
                     ajouteAlarme();
                     break;
           case 'E':
                     enleveAlarme();
                     break;
           case 'L':
                     listeAlarmes();
                     break;         
           case 'R': 
                     infoRelais();
                     break;  
          }
        
       }
   }   
  now = rtc.now();

  if(now.second() != derniereSeconde )
  {
     derniereSeconde= now.second();
     // ok une seconde de passé alors vérifions les alarmes
     verifieAlarmes();
   }

   delay(100);
 
}
