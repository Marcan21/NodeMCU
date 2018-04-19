# NodeMCU
NodeMCU data acquisition with ThingSpeak integration IoT. Batch update supported

https://thingspeak.com/channels/332349

Features :

• En appuyant sur le bouton de droite, une demande de départ du mode point d'accès
est sollicitée.
  o Une fois rendu en mode AP, l'afficheur indique les lettres 'AP'.
• En appuyant sur le bouton de gauche, le mode d'opération de l'interface est
enclenché. Dès que l'afficheur numérique commence à clignoter;
  o Si aucun bouton n'est appuyé, la fréquence d'acquisition est affichée et le
mode d'opération de l'interface viendra à échéance après quatre secondes.
  o Si le bouton de gauche est appuyé, le mode de sélection de fréquence en
engagé. Le potentiomètre peut donc être utilisé pour faire varier la
fréquence.
▪ Le potentiomètre permet de sélectionner des fréquences
d'acquisition allant de1 à 99 minutes.
▪ Après 8 secondes sans modification de la valeur, le module quitte le
mode de sélection de fréquence.
  o Si les deux boutons sont appuyés simultanément, le mode de réinitialisation
est enclenché. Voir la Figure 25.
▪ En mode de réinitialisation, quatre traits sont affichés et les boutons
sont désactivés.
• En mode de veille profonde, rien n'est affiché à l'écran et les boutons sont
inutilisables
