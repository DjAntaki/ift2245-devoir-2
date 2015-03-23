% Rapport du TP2
% Vincent Antaki et Guillaume Poirier-Morency

###Rapport tp2 2



#Critique face au tp :

Nous voulons dénoncer l'encodage horrible avec lequel on nous a demander de transmettre les données. On n'a que des int à transmettre et pourtant on nous demande de transmettre l'encodage est en char.

Calculer le taux de sureprésentation de l'encodage.


Les requêtes sont représentés sur des bytes au lieu d'utiliser un encodage basé
sur une chaine de caractères.

L'identifiant du client est un entier non-signé `unsigned int`

Les demandes relatives ressources sont des entiers signés `int`:

 - une valeur positive représente une libération de ressources
 - une valeur négative une allocation.
 - zéro est neutre

```
bloc:   | client id | ressource 1 | ressource 2 | ... | ressource n |
offset: 0           4             8             12    4n            4n + 1
```

Le serveur renvoit 1 seul byte signé contenant la valeur de retour.


Un seul socket s'occupe de la communication entre les fils d'exécutions clients et serveurs.
En terme d'opéraito
Les clients utilisent `connect` et le

