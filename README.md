# NodeMCU
NodeMCU data acquisition with ThingSpeak integration IoT. Batch update supported

<img src="https://raw.githubusercontent.com/Marcan21/NodeMCU/master/Projet_Boitier.PNG">

## ThingSpeak Channel
https://thingspeak.com/channels/332349

## Fonctionnement :

• En appuyant sur le bouton de droite, une demande de départ du mode point d'accès
est sollicitée. <br />
  o Une fois rendu en mode AP, l'afficheur indique les lettres 'AP'. <br />
• En appuyant sur le bouton de gauche, le mode d'opération de l'interface est
enclenché. Dès que l'afficheur numérique commence à clignoter;<br />
  o Si aucun bouton n'est appuyé, la fréquence d'acquisition est affichée et le
mode d'opération de l'interface viendra à échéance après quatre secondes.<br />
  o Si le bouton de gauche est appuyé, le mode de sélection de fréquence en
engagé. Le potentiomètre peut donc être utilisé pour faire varier la
fréquence.<br />
▪ Le potentiomètre permet de sélectionner des fréquences
d'acquisition allant de1 à 99 minutes.<br />
▪ Après 8 secondes sans modification de la valeur, le module quitte le
mode de sélection de fréquence.<br />
  o Si les deux boutons sont appuyés simultanément, le mode de réinitialisation
est enclenché. Voir la Figure 25.<br />
▪ En mode de réinitialisation, quatre traits sont affichés et les boutons
sont désactivés.<br />
• En mode de veille profonde, rien n'est affiché à l'écran et les boutons sont
inutilisables<br />

<img src="https://raw.githubusercontent.com/Marcan21/NodeMCU/master/Projet_Interieur.PNG">

<img src="https://raw.githubusercontent.com/Marcan21/NodeMCU/master/Projet_Portail_Captif.PNG">
