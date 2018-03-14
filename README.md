# ArduinoTimer
timer1.ino - Horloge programmable avec un arduino, quatres relais

L'information des alarmes et la configuration du RTC se font avec le port série à 9600 baud.

Pour changer le RCT il faut envoyer la commande C avec les paramètres.

   C année mois jour heure minute seconde

   ex:   C 2018 3 14 20 00 00 
   correspond a configurer le RTC pour le 14 mars 2018 20:00:00
   
  
Pour Ajouter une alarme if faut envoyer la commande A avec les paramètres.

  A mois jour jourSemaine heure minute seconde relais ON/OFF
  
  ex:  A * * * 18 00 00 0 1
  correspond a une alarme de tout les jours pour 18:00:00 avec activation du relais 0

  ex:  A * * * 19 00 00 0 0
  correspond a une alarme de tout les jours pour 19:00:00 avec déactivation du relais 0
  
  ex:  A 3 14 * 6 0 0 1 1
  correspond a une alarme pour le 14 mars à 6:00:00 avec activation du relais 1
  
  pour les commandes tapez H suivit d'un cariage return
  
  Pour la console arduino il faut configurer le retour du chariot. (En bas du window de la console série).
